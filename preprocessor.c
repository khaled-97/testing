/* Preprocessor implementation for handling macros */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preprocessor.h"
#include "utils.h"
#include "instructions.h"

#define MAX_MACRO_NAME 32
#define MAX_MACRO_LINES 100
#define MAX_MACROS 50

/* Structure to store macro information */
typedef struct {
    char name[MAX_MACRO_NAME];
    char *lines[MAX_MACRO_LINES];
    int line_count;
} Macro;

/* Array to store all macros */
static Macro macros[MAX_MACROS];
static int macro_count = 0;

/* Check if a name is a valid macro name */
static Bool is_valid_macro_name(const char *name) {
    int i;
    
    /* Must start with letter */
    if (!name || !isalpha(name[0]))
        return FALSE;
    
    /* Check remaining characters */
    for (i = 1; name[i]; i++) {
        if (!isalnum(name[i]) && name[i] != '_')
            return FALSE;
    }
    
    /* Check if name is a reserved word or instruction */
    if (strcmp(name, "mcro") == 0 || strcmp(name, "mcroend") == 0)
        return FALSE;
        
    /* Check if name is a directive */
    if (strcmp(name, ".data") == 0 || strcmp(name, ".string") == 0 ||
        strcmp(name, ".entry") == 0 || strcmp(name, ".extern") == 0)
        return FALSE;
        
    /* Check if name is an instruction */
    if (strcmp(name, "mov") == 0 || strcmp(name, "cmp") == 0 ||
        strcmp(name, "add") == 0 || strcmp(name, "sub") == 0 ||
        strcmp(name, "lea") == 0 || strcmp(name, "clr") == 0 ||
        strcmp(name, "not") == 0 || strcmp(name, "inc") == 0 ||
        strcmp(name, "dec") == 0 || strcmp(name, "jmp") == 0 ||
        strcmp(name, "bne") == 0 || strcmp(name, "jsr") == 0 ||
        strcmp(name, "red") == 0 || strcmp(name, "prn") == 0 ||
        strcmp(name, "rts") == 0 || strcmp(name, "stop") == 0)
        return FALSE;
    
    return TRUE;
}

/* Find a macro by name */
static Macro* find_macro(const char *name) {
    int i;
    for (i = 0; i < macro_count; i++) {
        if (strcmp(macros[i].name, name) == 0) {
            return &macros[i];
        }
    }
    return NULL;
}

/* Add a new macro */
static Bool add_macro(const char *name) {
    if (macro_count >= MAX_MACROS) {
        fprintf(stderr, "Error: Too many macros defined\n");
        return FALSE;
    }
    
    if (!is_valid_macro_name(name)) {
        fprintf(stderr, "Error: Invalid macro name '%s'\n", name);
        return FALSE;
    }
    
    if (find_macro(name)) {
        fprintf(stderr, "Error: Macro '%s' already defined\n", name);
        return FALSE;
    }
    
    strcpy(macros[macro_count].name, name);
    macros[macro_count].line_count = 0;
    macro_count++;
    
    return TRUE;
}

/* Add a line to the current macro */
static Bool add_line_to_macro(const char *line) {
    Macro *current_macro;
    
    if (macro_count <= 0) {
        fprintf(stderr, "Error: No macro currently being defined\n");
        return FALSE;
    }
    
    current_macro = &macros[macro_count - 1];
    
    if (current_macro->line_count >= MAX_MACRO_LINES) {
        fprintf(stderr, "Error: Too many lines in macro '%s'\n", current_macro->name);
        return FALSE;
    }
    
    current_macro->lines[current_macro->line_count] = str_copy(line);
    current_macro->line_count++;
    
    return TRUE;
}

/* Free all allocated memory for macros */
static void free_macros() {
    int i, j;
    
    for (i = 0; i < macro_count; i++) {
        for (j = 0; j < macros[i].line_count; j++) {
            free(macros[i].lines[j]);
        }
    }
    
    macro_count = 0;
}

/* Process a .as file and create a .am file with expanded macros */
Bool preprocess_file(const char *filename) {
    FILE *input_fp, *output_fp;
    char line_buf[MAX_SOURCE_LINE];
    char macro_name[MAX_MACRO_NAME];
    char input_filename[256], output_filename[256];
    Bool in_macro = FALSE;
    Bool success = TRUE;
    int line_num = 1;
    
    /* Create input filename with .as extension */
    sprintf(input_filename, "%s.as", filename);
    
    /* Create output filename with .am extension */
    sprintf(output_filename, "%s.am", filename);
    
    /* Open input file */
    input_fp = fopen(input_filename, "r");
    if (!input_fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_filename);
        return FALSE;
    }
    
    /* Open output file */
    output_fp = fopen(output_filename, "w");
    if (!output_fp) {
        fprintf(stderr, "Error: Cannot create file %s\n", output_filename);
        fclose(input_fp);
        return FALSE;
    }
    
    /* Reset macro count */
    macro_count = 0;
    
    /* Process each line */
    while (fgets(line_buf, MAX_SOURCE_LINE, input_fp)) {
        char trimmed_line[MAX_SOURCE_LINE];
        int i = 0;
        
        /* Copy line and trim whitespace */
        strcpy(trimmed_line, line_buf);
        str_trim(trimmed_line);
        
        /* Skip empty lines and comments */
        if (trimmed_line[0] == '\0' || trimmed_line[0] == ';') {
            fprintf(output_fp, "%s", line_buf); /* Preserve original line */
            line_num++;
            continue;
        }
        
        /* Skip whitespace */
        skip_whitespace(trimmed_line, &i);
        
        /* Check for macro definition start */
        if (strncmp(trimmed_line + i, "mcro", 4) == 0 && 
        (isspace(trimmed_line[i+4]) || trimmed_line[i+4] == '\0')) {
            int name_start;
            if (in_macro) {
                fprintf(stderr, "Error in line %d: Nested macro definition not allowed\n", line_num);
                success = FALSE;
                break;
            }
            
            i += 4; /* Skip "mcro" */
            skip_whitespace(trimmed_line, &i);
            
            /* Extract macro name */
            name_start = i;
            while (trimmed_line[i] && !isspace(trimmed_line[i])) {
                i++;
            }
            
            if (i == name_start) {
                fprintf(stderr, "Error in line %d: Missing macro name\n", line_num);
                success = FALSE;
                break;
            }
            
            /* Copy macro name */
            strncpy(macro_name, trimmed_line + name_start, i - name_start);
            macro_name[i - name_start] = '\0';
            
            /* Check there's nothing else on the line after the name */
            skip_whitespace(trimmed_line, &i);
            if (trimmed_line[i] != '\0') {
                fprintf(stderr, "Error in line %d: Extra content after macro name not allowed\n", line_num);
                success = FALSE;
                break;
            }
            
            /* Add macro */
            if (!add_macro(macro_name)) {
                success = FALSE;
                break;
            }
            
            in_macro = TRUE;
        }
        /* Check for macro definition end */
        else if (strncmp(trimmed_line + i, "mcroend", 7) == 0 &&
                 (isspace(trimmed_line[i+7]) || trimmed_line[i+7] == '\0')) {
            
            if (!in_macro) {
                fprintf(stderr, "Error in line %d: 'mcroend' without matching 'mcro'\n", line_num);
                success = FALSE;
                break;
            }
            
            /* Check there's nothing else on the line after mcroend */
            i += 7;  /* Skip "mcroend" */
            skip_whitespace(trimmed_line, &i);
            if (trimmed_line[i] != '\0') {
                fprintf(stderr, "Error in line %d: Extra content after mcroend not allowed\n", line_num);
                success = FALSE;
                break;
            }
            
            in_macro = FALSE;
        }
        /* Inside macro definition */
        else if (in_macro) {
            if (!add_line_to_macro(line_buf)) {
                success = FALSE;
                break;
            }
        }
        /* Check for macro usage */
        else {
            Macro *macro = find_macro(trimmed_line + i);
            
            if (macro) {
                /* Expand macro */
                int j;
                for (j = 0; j < macro->line_count; j++) {
                    fprintf(output_fp, "%s", macro->lines[j]);
                }
            } else {
                /* Regular line, copy to output */
                fprintf(output_fp, "%s", line_buf);
            }
        }
        
        line_num++;
    }
    
    /* Check if we're still in a macro definition */
    if (in_macro) {
        fprintf(stderr, "Error: Unclosed macro definition at end of file\n");
        success = FALSE;
    }
    
    /* Cleanup */
    fclose(input_fp);
    fclose(output_fp);
    free_macros();
    
    return success;
}
