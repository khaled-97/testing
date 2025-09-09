/*
 * Assembly Instruction Handling Implementation
 *
 * This module handles all assembly instruction processing including:
 * 1. Processing directives (.data, .string, .entry, .extern)
 * 2. Validating instruction operands
 * 3. Converting numeric values
 * 4. Building the data image
 * 
 * All functions follow strict error checking and validation
 * to ensure proper assembly code processing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "instructions.h"
#include "utils.h"
#include "symbol_table.h"
#include "binary_machine_code.h"  /* For ARE_ABSOLUTE definition */

/*
 * get_instruction_type - Identifies the type of directive in a source line
 *
 * Parameters:
 * line: The source line to examine
 * index: Pointer to current position in line (updated after directive)
 *
 * Returns:
 * Directive: Type of directive found (DIR_NONE if not a directive,
 *           DIR_ERROR if invalid directive)
 *
 * Recognizes: .data, .string, .entry, .extern directives
 */
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
    
    if (line.text[*index] != '.') {
        return DIR_NONE;
    }
    
    for (i = 0; directives[i].name; i++) {
        int len = str_len(directives[i].name);
        if (strncmp(line.text + *index, directives[i].name, len) == 0) {
            *index += len;
            return directives[i].type;
        }
    }
    return DIR_ERROR;
}

/*
 * process_data_inst - Processes a .data directive and builds data image
 *
 * Parameters:
 * line: Source line containing the .data directive
 * start_idx: Starting index after .data directive
 * data_img: Array to store processed data values
 * dc: Pointer to data counter (updated as values are stored)
 *
 * Returns:
 * Bool: TRUE if directive processed successfully, FALSE if error
 *
 * Handles comma-separated list of signed integers
 */
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
        
    /* Store value directly without ARE bits for .data directives */
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

/*
 * process_string_inst - Processes a .string directive and builds data image
 *
 * Parameters:
 * line: Source line containing the .string directive
 * start_idx: Starting index after .string directive
 * data_img: Array to store processed character values
 * dc: Pointer to data counter (updated as characters are stored)
 *
 * Returns:
 * Bool: TRUE if string processed successfully, FALSE if error
 *
 * Processes quoted string and adds null terminator
 */
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
        /* Store character directly without ARE bits */
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
    
    /* Add null terminator without ARE bits */
    data_img[*dc] = 0; /* Zero value */
    (*dc)++;
    
    /* Check for extra content */
    skip_whitespace(line.text, &i);
    if (line.text[i] && line.text[i] != '\n') {
        print_error(line, "Unexpected content after string");
        return FALSE;
    }
    return TRUE;
}

/*
 * process_extern_inst - Processes an .extern directive
 *
 * Parameters:
 * line: Source line containing the .extern directive
 * start_idx: Starting index after .extern directive
 * symbols: Symbol table to store external symbol
 *
 * Returns:
 * Bool: TRUE if external label processed successfully, FALSE if error
 *
 * Adds external symbol to symbol table with type SYMBOL_EXTERN
 */
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

/*
 * process_entry_inst - Processes an .entry directive (during second pass)
 *
 * Parameters:
 * line: Source line containing the .entry directive
 * start_idx: Starting index after .entry directive
 * symbols: Symbol table to verify entry symbol exists
 *
 * Returns:
 * Bool: TRUE if entry label valid and exists, FALSE if error
 *
 * Verifies entry symbol is defined in first pass
 */
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

/*
 * is_valid_number - Validates a string represents a valid signed integer
 *
 * Parameters:
 * str: String to validate
 *
 * Returns:
 * Bool: TRUE if string is valid signed integer, FALSE otherwise
 *
 * Accepts optional +/- prefix followed by digits only
 */
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

/*
 * get_number - Converts a string to a numeric value
 *
 * Parameters:
 * str: String to convert
 * success: Pointer to Bool to indicate conversion success/failure
 *
 * Returns:
 * long: Converted numeric value (0 if conversion fails)
 *
 * Validates input before conversion using is_valid_number
 */
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
