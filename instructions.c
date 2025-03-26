/* Assembly instruction handling implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "instructions.h"
#include "utils.h"
#include "symbol_table.h"
#include "code.h"  /* For ARE_ABSOLUTE definition */

/* Debug flag for instructions */
#define DEBUG_INSTRUCTIONS 1

/* Find instruction type from line starting at index */
Directive get_instruction_type(SourceLine line, int *index) {
    if (DEBUG_INSTRUCTIONS) {
        printf("[DEBUG] Instructions: Checking for directive at index %d: %s\n", *index, line.text + *index);
    }
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
    
    if (line.text[*index] != '.') {
        if (DEBUG_INSTRUCTIONS) {
            printf("[DEBUG] Instructions: No directive found (not starting with '.')\n");
        }
        return DIR_NONE;
    }
    
    for (i = 0; directives[i].name; i++) {
        int len = str_len(directives[i].name);
        if (strncmp(line.text + *index, directives[i].name, len) == 0) {
            *index += len;
            if (DEBUG_INSTRUCTIONS) {
                printf("[DEBUG] Instructions: Found directive: %s\n", directives[i].name);
            }
            return directives[i].type;
        }
    }
    
    if (DEBUG_INSTRUCTIONS) {
        printf("[DEBUG] Instructions: Invalid directive\n");
    }
    return DIR_ERROR;
}

/* Process .data instruction */
Bool process_data_inst(SourceLine line, int start_idx, long *data_img, long *dc) {
    if (DEBUG_INSTRUCTIONS) {
        printf("[DEBUG] Instructions: Processing .data directive at index %d\n", start_idx);
    }
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
        if (DEBUG_INSTRUCTIONS) {
            printf("[DEBUG] Instructions: Empty .data directive\n");
        }
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
            if (DEBUG_INSTRUCTIONS) {
                printf("[DEBUG] Instructions: Number conversion failed for '%s'\n", num_str);
            }
            return FALSE;
        }
        
    /* Store value directly without ARE bits for .data directives */
    data_img[*dc] = value;
        if (DEBUG_INSTRUCTIONS) {
            printf("[DEBUG] Instructions: Stored data value %ld at DC=%ld\n", value, *dc);
            printf("[DEBUG] Instructions: This will be encoded as a 24-bit word in the object file\n");
            printf("[DEBUG] Instructions: Data word structure:\n");
            printf("[DEBUG] Instructions:   Value (bits 3-23): %ld\n", value);
            printf("[DEBUG] Instructions:   ARE (bits 0-2): 0 (absolute)\n");
            
            /* Calculate the 24-bit word value for debugging */
            unsigned long word_value = ((unsigned long)value << 3);
            printf("[DEBUG] Instructions:   24-bit word value: 0x%06lx\n", word_value & 0xFFFFFF);
            
            /* Show binary representation */
            char binary[25];
            unsigned long mask = 1UL << 23;
            int i;
            for (i = 0; i < 24; i++) {
                binary[i] = (word_value & mask) ? '1' : '0';
                mask >>= 1;
            }
            binary[24] = '\0';
            printf("[DEBUG] Instructions:   Binary representation: %s\n", binary);
        }
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
    
    if (DEBUG_INSTRUCTIONS) {
        printf("[DEBUG] Instructions: Successfully processed .data directive\n");
    }
    return TRUE;
}

/* Process .string instruction */
Bool process_string_inst(SourceLine line, int start_idx, long *data_img, long *dc) {
    if (DEBUG_INSTRUCTIONS) {
        printf("[DEBUG] Instructions: Processing .string directive at index %d\n", start_idx);
    }
    int i = start_idx;
    
    skip_whitespace(line.text, &i);
    
    /* String must start with quote */
    if (line.text[i] != '"') {
        print_error(line, "String must begin with quote");
        if (DEBUG_INSTRUCTIONS) {
            printf("[DEBUG] Instructions: String must begin with quote\n");
        }
        return FALSE;
    }
    i++;
    
    /* Process string characters */
    while (line.text[i] && line.text[i] != '"') {
        if (line.text[i] == '\n') {
            print_error(line, "Unterminated string");
            if (DEBUG_INSTRUCTIONS) {
                printf("[DEBUG] Instructions: Unterminated string\n");
            }
            return FALSE;
        }
        /* Store character directly without ARE bits */
        data_img[*dc] = line.text[i];
        if (DEBUG_INSTRUCTIONS) {
            printf("[DEBUG] Instructions: Stored character '%c' (%d) at DC=%ld\n", 
                   (char)line.text[i], (int)line.text[i], *dc);
            printf("[DEBUG] Instructions: This will be encoded as a 24-bit word in the object file\n");
            printf("[DEBUG] Instructions: Character word structure:\n");
            printf("[DEBUG] Instructions:   ASCII value (bits 3-23): %d\n", (int)line.text[i]);
            printf("[DEBUG] Instructions:   ARE (bits 0-2): 0 (absolute)\n");
            
            /* Calculate the 24-bit word value for debugging */
            unsigned long word_value = ((unsigned long)line.text[i] << 3);
            printf("[DEBUG] Instructions:   24-bit word value: 0x%06lx\n", word_value & 0xFFFFFF);
            
            /* Show binary representation */
            char binary[25];
            unsigned long mask = 1UL << 23;
            int j;
            for (j = 0; j < 24; j++) {
                binary[j] = (word_value & mask) ? '1' : '0';
                mask >>= 1;
            }
            binary[24] = '\0';
            printf("[DEBUG] Instructions:   Binary representation: %s\n", binary);
        }
        (*dc)++;
        i++;
    }
    
    /* String must end with quote */
    if (line.text[i] != '"') {
        print_error(line, "String must end with quote");
        if (DEBUG_INSTRUCTIONS) {
            printf("[DEBUG] Instructions: String must end with quote\n");
        }
        return FALSE;
    }
    i++;
    
    /* Add null terminator without ARE bits */
    data_img[*dc] = 0; /* Zero value */
    if (DEBUG_INSTRUCTIONS) {
        printf("[DEBUG] Instructions: Added null terminator at DC=%ld\n", *dc);
        printf("[DEBUG] Instructions: This will be encoded as a 24-bit word in the object file\n");
        printf("[DEBUG] Instructions: Null terminator word structure:\n");
        printf("[DEBUG] Instructions:   Value (bits 3-23): 0\n");
        printf("[DEBUG] Instructions:   ARE (bits 0-2): 0 (absolute)\n");
        
        /* Calculate the 24-bit word value for debugging */
        unsigned long word_value = 0;
        printf("[DEBUG] Instructions:   24-bit word value: 0x%06lx\n", word_value & 0xFFFFFF);
        
        /* Show binary representation */
        char binary[25];
        unsigned long mask = 1UL << 23;
        int i;
        for (i = 0; i < 24; i++) {
            binary[i] = '0';  /* All zeros for null terminator */
        }
        binary[24] = '\0';
        printf("[DEBUG] Instructions:   Binary representation: %s\n", binary);
    }
    (*dc)++;
    
    /* Check for extra content */
    skip_whitespace(line.text, &i);
    if (line.text[i] && line.text[i] != '\n') {
        print_error(line, "Unexpected content after string");
        return FALSE;
    }
    
    if (DEBUG_INSTRUCTIONS) {
        printf("[DEBUG] Instructions: Successfully processed .string directive\n");
    }
    return TRUE;
}

/* Process .extern instruction */
Bool process_extern_inst(SourceLine line, int start_idx, SymbolTable* symbols) {
    if (DEBUG_INSTRUCTIONS) {
        printf("[DEBUG] Instructions: Processing .extern directive at index %d\n", start_idx);
    }
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
        if (DEBUG_INSTRUCTIONS) {
            printf("[DEBUG] Instructions: Invalid external label: %s\n", label);
        }
        return FALSE;
    }
    
    /* Add to symbol table */
    add_symbol(symbols, label, 0, SYMBOL_EXTERN);
    if (DEBUG_INSTRUCTIONS) {
        printf("[DEBUG] Instructions: Added external symbol: %s\n", label);
    }
    
    /* Check for extra content */
    skip_whitespace(line.text, &i);
    if (line.text[i] && line.text[i] != '\n') {
        print_error(line, "Unexpected content after external label");
        return FALSE;
    }
    
    if (DEBUG_INSTRUCTIONS) {
        printf("[DEBUG] Instructions: Successfully processed .extern directive\n");
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
