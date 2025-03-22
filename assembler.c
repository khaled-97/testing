/* Main assembler program */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "first_pass.h"
#include "second_pass.h"
#include "utils.h"
#include "symbol_table.h"
#include "writefiles.h"
#include "preprocessor.h"

#define MAX_FILENAME 256

/* Process a single assembly source file */
static Bool process_file(const char *filename) {
    FILE *fp;
    char line_buf[MAX_SOURCE_LINE];
    SourceLine line;
    MachineWord *code[MAX_CODE_SIZE] = {NULL};
    long data[MAX_CODE_SIZE] = {0};
    long ic = START_IC, dc = 0;
    long line_num = 1;
    Bool success = TRUE;
    char basename[MAX_FILENAME];
    SymbolTable *symbols;
    char *input_filename = malloc(strlen(filename) + 4); /* +4 for .am and null terminator */
    
    /* Preprocess the file to expand macros */
    printf("Preprocessing %s...\n", filename);
    if (!preprocess_file(filename)) {
        fprintf(stderr, "Error: Preprocessing failed for %s\n", filename);
        return FALSE;
    }
    
    /* Create filename with .am extension (preprocessed file) */
    if (!input_filename) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return FALSE;
    }
    
    sprintf(input_filename, "%s.am", filename);
    
    /* Open preprocessed source file */
    fp = fopen(input_filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_filename);
        free(input_filename);
        return FALSE;
    }
    
    /* Store base filename (without extension) */
    strcpy(basename, filename);
    
    free(input_filename);
    
    /* Initialize symbol table */
    symbols = create_symbol_table();
    
    /* Initialize line info */
    line.filename = filename;
    
    /* First Pass */
    while (fgets(line_buf, MAX_SOURCE_LINE, fp)) {
        line.num = line_num++;
        line.text = line_buf;
        
        if (!process_line_first_pass(line, &ic, &dc, code, data, symbols)) {
            success = FALSE;
            break;
        }
    }
    
    /* If first pass successful, update data symbols and do second pass */
    if (success) {
        /* Add IC to each data symbol address (step 1.18-1.19) */
        SymbolEntry *entry;
        long final_ic = ic;  /* Save the final IC */
        
        /* Update all data symbols to have addresses after code */
        for (entry = symbols->first; entry; entry = entry->next) {
            if (entry->type == SYMBOL_DATA) {
                entry->address += final_ic;
            }
        }
        
        /* Reset file and line counter */
        rewind(fp);
        line_num = 1;
        ic = START_IC;
        
        /* Second Pass */
        while (fgets(line_buf, MAX_SOURCE_LINE, fp)) {
            line.num = line_num++;
            line.text = line_buf;
            
            if (!process_line_second_pass(line, &ic, code, symbols)) {
                success = FALSE;
                break;
            }
        }
        
        /* If both passes successful, write output files */
        if (success) {
            /* Debug: Print all symbols */
            {
                SymbolEntry *entry;
                printf("Symbol Table contents:\n");
                for (entry = symbols->first; entry; entry = entry->next) {
                    printf("Symbol: %s, Type: %d, Address: %ld\n", 
                           entry->name, entry->type, entry->address);
                }
            }
            
            success = write_object_file(basename, code, data, ic, dc) &&
                     write_entry_file(basename, symbols) &&
                     write_extern_file(basename, symbols);
        }
    }
    
    /* Cleanup */
    fclose(fp);
    
    /* Free code words */
    {
        int i;
        for (i = 0; i < ic - START_IC; i++) {
            if (code[i]) {
                if (code[i]->is_instruction)
                    free(code[i]->content.code);
                else
                    free(code[i]->content.data);
                free(code[i]);
            }
        }
    }
    
    /* Free symbol table */
    free_symbol_table(symbols);
    
    return success;
}

int main(int argc, char *argv[]) {
    int i;
    Bool success = TRUE;
    
    /* Check arguments */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1.as [file2.as ...]\n", argv[0]);
        return 1;
    }
    
    /* Process each input file */
    for (i = 1; i < argc; i++) {
        printf("Processing %s:\n", argv[i]);
        if (!process_file(argv[i])) {
            success = FALSE;
            printf("Failed to assemble %s\n", argv[i]);
        } else {
            printf("Successfully assembled %s\n", argv[i]);
        }
    }
    
    return success ? 0 : 1;
}
