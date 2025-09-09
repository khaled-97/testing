/*
 * Output File Writing Implementation
 *
 * This module handles the creation of all output files:
 * 1. Object file (.ob) - Contains the assembled machine code
 * 2. Entry file (.ent) - Lists entry symbols and their addresses
 * 3. External file (.ext) - Lists external symbol references
 *
 * The object file format follows the 24-bit word specification
 * with hexadecimal encoding and proper memory addressing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "writefiles.h"
#include "utils.h"

/*
 * encode_number - Encodes a 24-bit number into hexadecimal format
 *
 * Parameters:
 * num: Number to encode (only lower 24 bits are used)
 * buf: Buffer to store the encoded string
 *
 * Formats the number as a 6-digit lowercase hexadecimal string,
 * ensuring it fits within 24 bits (mask with 0xFFFFFF)
 */
static void encode_number(unsigned long num, char *buf) {
    /* Output in lowercase hexadecimal format, 6 digits (24 bits) */
    sprintf(buf, "%06lx", num & 0xFFFFFF);
}

/*
 * write_object_file - Creates the object file (.ob) containing machine code
 *
 * Parameters:
 * base_name: Base name for the output file
 * code: Array of machine code words
 * data: Array of data values
 * ic: Final instruction counter
 * dc: Final data counter
 *
 * Returns:
 * Bool: TRUE if file written successfully, FALSE if error
 *
 * File Format:
 * - First line: <code_size> <data_size>
 * - Following lines: <address> <encoded_word>
 *   where encoded_word is 6 hex digits representing 24-bit word
 */
Bool write_object_file(const char *base_name, MachineWord **code, long *data,
                      long ic, long dc) {
    char filename[256];
    FILE *fp;
    long addr;
    char encoded[10];
    long code_size = ic - START_IC;
    
    /* Create filename */
    sprintf(filename, "%s.ob", base_name);
    
    /* Open file */
    fp = fopen(filename, "w");
    if (!fp) return FALSE;
    
    /* Write header - code and data sizes */
    fprintf(fp, "%ld %ld\n", code_size, dc);
    
    for (addr = 0; addr < code_size; addr++) {
        if (code[addr]) {
            unsigned long word = 0;
            InstructionWord *inst;
            DataWord *data;
            
            if (code[addr]->is_instruction) {
                inst = code[addr]->content.code;

                /* Convert operation code to the original expected format */
                /* Encode instruction word following the specification exactly */
                /* 
                   - Bits 23-18: Opcode
                   - Bits 17-16: Source addressing mode (reset if no source operand)
                   - Bits 15-13: Source register (reset if not register operand)
                   - Bits 12-11: Destination addressing mode (reset if no destination)
                   - Bits 10-8: Destination register (reset if not register operand)
                   - Bits 7-3: Function code
                   - Bits 2-0: ARE 
                */
                word = (inst->op << 18);                        /* Opcode: bits 23-18 */
                
                /* Source fields (bits 17-13) */
                word |= (inst->src_mode << 16);                 /* Source addressing mode: bits 17-16 */
                word |= (inst->src_reg << 13);                  /* Source register: bits 15-13 */
                
                /* Destination fields (bits 12-8) */
                word |= (inst->dest_mode << 11);                /* Destination addressing mode: bits 12-11 */
                word |= (inst->dest_reg << 8);                  /* Destination register: bits 10-8 */
                
                /* Function and ARE fields (bits 7-0) */
                word |= (inst->func << 3);                      /* Function code: bits 7-3 */
                word |= inst->are;                              /* ARE: bits 2-0 */
                
            } else {
                data = code[addr]->content.data;
                word = (data->value << 3) | data->are;
            }
            encode_number(word, encoded);
            fprintf(fp, "%07ld %s\n", addr + START_IC, encoded);
        }
    }
    
    for (addr = 0; addr < dc; addr++) {
        unsigned long word;
        
        /* Use the data value directly - no ARE bits for data directives */
        word = data[addr] & 0xFFFFFF; /* Ensure it's a 24-bit value */
        
        encode_number(word, encoded);
        fprintf(fp, "%07ld %s\n", addr + ic, encoded);
    }
    
    fclose(fp);
    return TRUE;
}

/*
 * write_entry_file - Creates the entry file (.ent) for entry symbols
 *
 * Parameters:
 * base_name: Base name for the output file
 * symbols: Symbol table containing all symbols
 *
 * Returns:
 * Bool: TRUE if file written successfully or no entries,
 *       FALSE if file creation failed
 *
 * File Format:
 * Each line: <symbol_name> <address>
 * Only writes file if at least one entry symbol exists
 */
Bool write_entry_file(const char *base_name, SymbolTable *symbols) {
    char filename[256];
    FILE *fp;
    SymbolEntry *entry;
    Bool has_entries = FALSE;
    
    /* Check if we have any entries */
    for (entry = symbols->first; entry; entry = entry->next) {
        if (entry->type == SYMBOL_ENTRY) {
            has_entries = TRUE;
            break;
        }
    }
    
    if (!has_entries) return TRUE;  /* No entries to write */
    
    /* Create filename */
    sprintf(filename, "%s.ent", base_name);
    
    /* Open file */
    fp = fopen(filename, "w");
    if (!fp) return FALSE;
    
    /* Write all entry symbols */
    for (entry = symbols->first; entry; entry = entry->next) {
        if (entry->type == SYMBOL_ENTRY) {
            fprintf(fp, "%s %07ld\n", entry->name, entry->address);
        }
    }
    
    fclose(fp);
    return TRUE;
}

/*
 * write_extern_file - Creates the external file (.ext) for external references
 *
 * Parameters:
 * base_name: Base name for the output file
 * symbols: Symbol table containing all symbols
 *
 * Returns:
 * Bool: TRUE if file written successfully or no externals,
 *       FALSE if file creation failed
 *
 * File Format:
 * Each line: <symbol_name> <reference_address>
 * Only includes external symbols that are actually referenced (address != 0)
 */
Bool write_extern_file(const char *base_name, SymbolTable *symbols) {
    char filename[256];
    FILE *fp;
    SymbolEntry *entry;
    Bool has_externals = FALSE;
    
    /* Check if we have any externals */
    for (entry = symbols->first; entry; entry = entry->next) {
        if (entry->type == SYMBOL_EXTERN && entry->address != 0) {
            has_externals = TRUE;
            break;
        }
    }
    
    if (!has_externals) return TRUE;  /* No externals to write */
    
    /* Create filename */
    sprintf(filename, "%s.ext", base_name);
    
    /* Open file */
    fp = fopen(filename, "w");
    if (!fp) return FALSE;
    
    /* Write all external references */
    for (entry = symbols->first; entry; entry = entry->next) {
        /* Only write entries with non-zero addresses (references, not declarations) */
        if (entry->type == SYMBOL_EXTERN && entry->address != 0) {
            fprintf(fp, "%s %07ld\n", 
                    entry->name, 
                    entry->address);
        }
    }
    
    fclose(fp);
    return TRUE;
}
