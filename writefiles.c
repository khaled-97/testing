/* Output file writing implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "writefiles.h"
#include "utils.h"

/* Debug flag for writefiles */
#define DEBUG_WRITEFILES 1

/* Convert machine word to binary format */
static void encode_binary(unsigned long num, char *buf) {
    int i;
    unsigned long mask = 1UL << 23; /* Start from MSB (24-bit number) */
    
    if (DEBUG_WRITEFILES) {
        printf("[DEBUG] WriteFiles: Encoding number %lu to binary\n", num);
    }
    
    for (i = 0; i < 24; i++) {
        buf[i] = (num & mask) ? '1' : '0';
        mask >>= 1;
    }
    buf[24] = '\0';
    
    if (DEBUG_WRITEFILES) {
        printf("[DEBUG] WriteFiles: Binary representation: %s\n", buf);
    }
}

/* Convert machine word to hexadecimal format for output file */
static void encode_number(unsigned long num, char *buf) {
    /* Output in lowercase hexadecimal format, 6 digits (24 bits) */
    if (DEBUG_WRITEFILES) {
        printf("[DEBUG] WriteFiles: Encoding number %lu to hexadecimal\n", num);
    }
    
    sprintf(buf, "%06lx", num & 0xFFFFFF);
    
    if (DEBUG_WRITEFILES) {
        printf("[DEBUG] WriteFiles: Hexadecimal representation: %s\n", buf);
    }
}

/* Write object file (.ob) */
Bool write_object_file(const char *base_name, MachineWord **code, long *data,
                      long ic, long dc) {
    if (DEBUG_WRITEFILES) {
        printf("[DEBUG] WriteFiles: Writing object file for %s\n", base_name);
        printf("[DEBUG] WriteFiles: Code size: %ld, Data size: %ld\n", ic - START_IC, dc);
    }
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
            char binary[25]; /* 24 bits + null terminator */
            InstructionWord *inst;
            DataWord *data;
            
            if (code[addr]->is_instruction) {
                inst = code[addr]->content.code;
                
                if (DEBUG_WRITEFILES) {
                    printf("[DEBUG] WriteFiles: Encoding instruction at address %ld\n", addr + START_IC);
                    printf("[DEBUG] WriteFiles: OpCode: %d, Src Mode: %d, Src Reg: %d, Dest Mode: %d, Dest Reg: %d, Func: %d, ARE: %d\n",
                           inst->op, inst->src_mode, inst->src_reg, inst->dest_mode, inst->dest_reg, inst->func, inst->are);
                }
                
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
                
                if (DEBUG_WRITEFILES) {
                    printf("[DEBUG] WriteFiles: Instruction word bits:\n");
                    printf("[DEBUG] WriteFiles:   OpCode (bits 18-23): %06lx\n", ((unsigned long)inst->op << 18) & 0xFFFFFF);
                    printf("[DEBUG] WriteFiles:   Src Mode (bits 16-17): %06lx\n", ((unsigned long)inst->src_mode << 16) & 0xFFFFFF);
                    printf("[DEBUG] WriteFiles:   Src Reg (bits 13-15): %06lx\n", ((unsigned long)inst->src_reg << 13) & 0xFFFFFF);
                    printf("[DEBUG] WriteFiles:   Dest Mode (bits 11-12): %06lx\n", ((unsigned long)inst->dest_mode << 11) & 0xFFFFFF);
                    printf("[DEBUG] WriteFiles:   Dest Reg (bits 8-10): %06lx\n", ((unsigned long)inst->dest_reg << 8) & 0xFFFFFF);
                    printf("[DEBUG] WriteFiles:   Func (bits 3-7): %06lx\n", ((unsigned long)inst->func << 3) & 0xFFFFFF);
                    printf("[DEBUG] WriteFiles:   ARE (bits 0-2): %06lx\n", (unsigned long)inst->are & 0xFFFFFF);
                    printf("[DEBUG] WriteFiles:   Final word: %06lx\n", word & 0xFFFFFF);
                }
            } else {
                data = code[addr]->content.data;
                
                if (DEBUG_WRITEFILES) {
                    printf("[DEBUG] WriteFiles: Encoding data word at address %ld\n", addr + START_IC);
                    printf("[DEBUG] WriteFiles: Value: %ld, ARE: %d\n", data->value, data->are);
                }
                
                word = (data->value << 3) | data->are;
                
                if (DEBUG_WRITEFILES) {
                    printf("[DEBUG] WriteFiles: Data word bits:\n");
                    printf("[DEBUG] WriteFiles:   Value (bits 3-23): %06lx\n", ((unsigned long)data->value << 3) & 0xFFFFFF);
                    printf("[DEBUG] WriteFiles:   ARE (bits 0-2): %06lx\n", (unsigned long)data->are & 0xFFFFFF);
                    printf("[DEBUG] WriteFiles:   Final word: %06lx\n", word & 0xFFFFFF);
                }
            }
            
            encode_number(word, encoded);
            encode_binary(word, binary);
            fprintf(fp, "%07ld %s %s\n", addr + START_IC, encoded, binary);
        }
    }
    
        /* Write data section after the code, as in the original implementation */
    for (addr = 0; addr < dc; addr++) {
        char binary[25]; /* 24 bits + null terminator */
        unsigned long word;
        
        if (DEBUG_WRITEFILES) {
            printf("[DEBUG] WriteFiles: Encoding data value at address %ld\n", addr + ic);
            printf("[DEBUG] WriteFiles: Value: %ld\n", data[addr]);
        }
        
        /* Use the data value directly - no ARE bits for data directives */
        word = data[addr] & 0xFFFFFF; /* Ensure it's a 24-bit value */
        
        if (DEBUG_WRITEFILES) {
            printf("[DEBUG] WriteFiles: Data value bits:\n");
            printf("[DEBUG] WriteFiles:   Value (bits 0-23): %06lx\n", word);
            printf("[DEBUG] WriteFiles:   Final word: %06lx\n", word);
        }
        
        encode_number(word, encoded);
        encode_binary(word, binary);
        fprintf(fp, "%07ld %s %s\n", addr + ic, encoded, binary);
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
