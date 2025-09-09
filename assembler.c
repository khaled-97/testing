/*
 * Main assembler program - Entry point for the two-pass assembler
 * This module orchestrates the assembly process:
 * 1. Preprocesses the input file to handle macros
 * 2. Performs first pass to build symbol table and encode instructions
 * 3. Performs second pass to resolve symbols and complete encoding
 * 4. Generates output files (.ob, .ent, .ext)
 */
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

/*
 * process_file - Processes a single assembly source file through all assembly stages
 * 
 * Parameters:
 * filename: Name of the assembly source file to process (without extension)
 * 
 * Returns:
 * Bool: TRUE if assembly was successful, FALSE if any errors occurred
 * 
 * The function performs these main steps:
 * 1. Preprocesses the source file to expand macros (.as -> .am)
 * 2. First pass: builds symbol table and encodes instructions
 * 3. Second pass: resolves symbols and completes encoding
 * 4. Generates output files if both passes are successful
 */
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
    
    /* Preprocess the source file to expand macros (.as -> .am) */
    if (!preprocess_file(filename)) {
        fprintf(stderr, "Error: Preprocessing failed for %s\n", filename);
        return FALSE;
    }
    
    /* Allocate memory for preprocessed filename (.am extension) */
    if (!input_filename) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return FALSE;
    }
    
    sprintf(input_filename, "%s.am", filename);
    
    /* Open the preprocessed source file for reading */
    fp = fopen(input_filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_filename);
        free(input_filename);
        return FALSE;
    }
    
    /* Store base filename without extension for output files */
    strcpy(basename, filename);
    
    free(input_filename);
    
    /* Initialize symbol table */
    symbols = create_symbol_table();
    
    /* Initialize line info */
    line.filename = filename;
    
    /* First Pass: Build symbol table and encode instructions */
    while (fgets(line_buf, MAX_SOURCE_LINE, fp)) {
        line.num = line_num++;
        line.text = line_buf;
        
        if (!process_line_first_pass(line, &ic, &dc, code, data, symbols)) {
            success = FALSE;
            break;
        }
    }
    
    /* If first pass successful, update data symbol addresses and perform second pass */
    if (success) {
        /* Add IC to each data symbol address (step 1.18-1.19) */
        SymbolEntry *entry;
        long final_ic = ic;  /* Save the final IC */
        
            /* Update data symbol addresses to follow the code section */
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

/*
 * main - Entry point of the assembler program
 * 
 * Parameters:
 * argc: Number of command line arguments
 * argv: Array of command line argument strings
 * 
 * Returns:
 * int: 0 if all files processed successfully, 1 if any errors occurred
 * 
 * The function processes each input file given as command line arguments.
 * For each file, it calls process_file to perform the complete assembly process.
 */
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
        if (!process_file(argv[i])) {
            success = FALSE;
        }
    }
    
    return success ? 0 : 1;
}
