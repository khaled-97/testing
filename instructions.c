/* Assembly instruction handling implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "instructions.h"
#include "utils.h"
#include "symbol_table.h"

/* Find instruction type from line starting at index */
Directive get_instruction_type(SourceLine line, int *index) {
    struct {
        const char *name;
        Directive type;
    } directives[] = {
        {".data", DIR_DATA},
        {".string", DIR_STRING},
        {".entry", DIR_ENTRY},
        {".extern", DIR_EXTERN},
        {NULL, DIR_NONE}
    };
    int i;
    
    if (line.text[*index] != '.')
        return DIR_NONE;
    
    for (i = 0; directives[i].name; i++) {
        int len = str_len(directives[i].name);
        if (strncmp(line.text + *index, directives[i].name, len) == 0) {
            *index += len;
            return directives[i].type;
        }
    }
    
    return DIR_ERROR;
}

/* Process .data instruction */
Bool process_data_inst(SourceLine line, int start_idx, long *data_img, long *dc) {
    int i = start_idx;
    char num_str[MAX_SOURCE_LINE];
    int num_idx;
    Bool success;
    long value;
    int idx;
    
    skip_whitespace(line.text, &i);
    
    /* Check for empty data directive */
    if (!line.text[i] || line.text[i] == '\n') {
        print_error(line, "Empty .data directive");
        return FALSE;
    }
    
    /* Process each number */
    while (line.text[i] && line.text[i] != '\n') {
        /* Get number string */
        num_idx = 0;
        if (line.text[i] == '+' || line.text[i] == '-') {
            num_str[num_idx++] = line.text[i++];
        }
        
        /* Collect the entire token first */
        while (line.text[i] && line.text[i] != ',' && !isspace(line.text[i]) && num_idx < MAX_SOURCE_LINE - 1) {
            num_str[num_idx++] = line.text[i++];
        }
        num_str[num_idx] = '\0';

        /* If there are any non-digit characters (except leading +/-), show the full invalid token */
        idx = 0;
        if (num_str[0] == '+' || num_str[0] == '-') {
            idx = 1;
        }
        while (num_str[idx]) {
            if (!isdigit(num_str[idx])) {
                print_error(line, "Invalid number '%s' - only digits allowed (with optional +/- prefix)", num_str);
                return FALSE;
            }
            idx++;
        }
        
        /* Check for empty number */
        if (num_str[0] == '\0') {
            print_error(line, "Empty number after comma");
            return FALSE;
        }

        /* Check for sign without number */
        if ((num_str[0] == '+' || num_str[0] == '-') && !num_str[1]) {
            print_error(line, "Sign '%c' without a number", num_str[0]);
            return FALSE;
        }

        /* Convert to value */
        value = get_number(num_str, &success);
        if (!success) {
            print_error(line, "Number conversion failed for '%s'", num_str);
            return FALSE;
        }
        
        /* Store value */
        data_img[*dc] = value;
        (*dc)++;
        
        /* Skip whitespace and check commas */
        skip_whitespace(line.text, &i);
        if (line.text[i] == ',') {
            i++;
            skip_whitespace(line.text, &i);
            
            /* Check for multiple consecutive commas */
            if (line.text[i] == ',') {
                print_error(line, "Multiple consecutive commas found");
                return FALSE;
            }
            
            /* Check for comma at end */
            if (!line.text[i] || line.text[i] == '\n') {
                print_error(line, "Trailing comma with no number");
                return FALSE;
            }
        } else if (line.text[i] && line.text[i] != '\n') {
            print_error(line, "Expected comma between numbers");
            return FALSE;
        }
    }
    
    return TRUE;
}

/* Process .string instruction */
Bool process_string_inst(SourceLine line, int start_idx, long *data_img, long *dc) {
    int i = start_idx;
    
    skip_whitespace(line.text, &i);
    
    /* String must start with quote */
    if (line.text[i] != '"') {
        print_error(line, "String must begin with quote");
        return FALSE;
    }
    i++;
    
    /* Process string characters */
    while (line.text[i] && line.text[i] != '"') {
        if (line.text[i] == '\n') {
            print_error(line, "Unterminated string");
            return FALSE;
        }
        data_img[*dc] = line.text[i];
        (*dc)++;
        i++;
    }
    
    /* String must end with quote */
    if (line.text[i] != '"') {
        print_error(line, "String must end with quote");
        return FALSE;
    }
    i++;
    
    /* Add null terminator */
    data_img[*dc] = 0;
    (*dc)++;
    
    /* Check for extra content */
    skip_whitespace(line.text, &i);
    if (line.text[i] && line.text[i] != '\n') {
        print_error(line, "Unexpected content after string");
        return FALSE;
    }
    
    return TRUE;
}

/* Process .extern instruction */
Bool process_extern_inst(SourceLine line, int start_idx, SymbolTable* symbols) {
    int i = start_idx;
    char label[MAX_SOURCE_LINE];
    int label_idx = 0;
    
    skip_whitespace(line.text, &i);
    
    /* Get label name */
    while (line.text[i] && !isspace(line.text[i]) && label_idx < MAX_SOURCE_LINE - 1) {
        label[label_idx++] = line.text[i++];
    }
    label[label_idx] = '\0';
    
    /* Validate label */
    if (!is_valid_label(label)) {
        print_error(line, "Invalid external label: %s", label);
        return FALSE;
    }
    
    /* Add to symbol table */
    add_symbol(symbols, label, 0, SYMBOL_EXTERN);
    
    /* Check for extra content */
    skip_whitespace(line.text, &i);
    if (line.text[i] && line.text[i] != '\n') {
        print_error(line, "Unexpected content after external label");
        return FALSE;
    }
    
    return TRUE;
}

/* Process .entry instruction (second pass) */
Bool process_entry_inst(SourceLine line, int start_idx, SymbolTable* symbols) {
    int i = start_idx;
    char label[MAX_SOURCE_LINE];
    int label_idx = 0;
    
    skip_whitespace(line.text, &i);
    
    /* Get label name */
    while (line.text[i] && !isspace(line.text[i]) && label_idx < MAX_SOURCE_LINE - 1) {
        label[label_idx++] = line.text[i++];
    }
    label[label_idx] = '\0';
    
    /* Validate label */
    if (!is_valid_label(label)) {
        print_error(line, "Invalid entry label: %s", label);
        return FALSE;
    }
    
    /* Find symbol (must be defined) */
    if (!find_symbol(symbols, label)) {
        print_error(line, "Entry label undefined: %s", label);
        return FALSE;
    }
    
    /* Check for extra content */
    skip_whitespace(line.text, &i);
    if (line.text[i] && line.text[i] != '\n') {
        print_error(line, "Unexpected content after entry label");
        return FALSE;
    }
    
    return TRUE;
}

/* Validate numeric operand */
Bool is_valid_number(const char *str) {
    int i = 0;
    
    if (!str || !*str) return FALSE;
    
    /* Check sign */
    if (str[0] == '+' || str[0] == '-') i++;
    
    /* Must have at least one digit */
    if (!str[i]) return FALSE;
    
    /* Check remaining characters */
    while (str[i]) {
        if (!isdigit(str[i])) return FALSE;
        i++;
    }
    
    return TRUE;
}

/* Extract numeric value */
long get_number(const char *str, Bool *success) {
    char *end;
    long value;
    
    if (!str || !success) {
        if (success) *success = FALSE;
        return 0;
    }
    
    *success = TRUE;
    
    if (!is_valid_number(str)) {
        *success = FALSE;
        return 0;
    }
    
    value = strtol(str, &end, 10);
    if (*end != '\0') {
        *success = FALSE;
        return 0;
    }
    
    return value;
}
