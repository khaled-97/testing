/* Output file writing implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "writefiles.h"
#include "utils.h"

/* Convert machine word to hexadecimal format for output file */
static void encode_number(unsigned long num, char *buf) {
    /* Output in lowercase hexadecimal format, 6 digits (24 bits) */
    sprintf(buf, "%06lx", num & 0xFFFFFF);
}

/* Write object file (.ob) */
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
    
    /* Write code section - this should match the original format */
    for (addr = 0; addr < code_size; addr++) {
        if (code[addr]) {
            unsigned long word = 0;
            
            if (code[addr]->is_instruction) {
                InstructionWord *inst = code[addr]->content.code;
                /* Convert operation code to the original expected format */
                word = (inst->op << 18) | 
                      (inst->src_mode << 16) |
                      (inst->src_reg << 13) |
                      (inst->dest_mode << 11) |
                      (inst->dest_reg << 8) |
                      (inst->func << 3) |
                      inst->are;
            } else {
                DataWord *data = code[addr]->content.data;
                word = (data->value << 3) | data->are;
            }
            
            encode_number(word, encoded);
            fprintf(fp, "%07ld %s\n", addr + START_IC, encoded);
        }
    }
    
    /* Write data section after the code, as in the original implementation */
    for (addr = 0; addr < dc; addr++) {
        encode_number(data[addr] << 3, encoded); /* Shift values to match original format */
        fprintf(fp, "%07ld %s\n", addr + ic, encoded);
    }
    
    fclose(fp);
    return TRUE;
}

/* Write entry file (.ent) */
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

/* Write external file (.ext) */
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
