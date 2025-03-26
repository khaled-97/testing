/* First pass implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "first_pass.h"
#include "code.h"
#include "utils.h"
#include "instructions.h"
#include "symbol_table.h"

/* Debug flag for first pass */
#define DEBUG_FIRST_PASS 1

/* Local function declarations */
static Bool process_code_line(SourceLine line, int index, long *ic, MachineWord **code);
static void handle_extra_words(MachineWord **code, long *ic, char *operand, OpCode opcode);

/* Process a single line in the first pass */
Bool process_line_first_pass(SourceLine line, long *ic, long *dc, 
                           MachineWord **code, long *data, SymbolTable *symbols) {
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: Processing line %ld: %s", line.num, line.text);
    }
    /* Note: In the original implementation, all data is placed AFTER code.
       First we process everything, then at the end of the first pass,
       we update data symbol addresses by adding the final IC value */
    int index = 0;
    char symbol[MAX_SOURCE_LINE];
    Directive dir;
    
    /* Skip whitespace */
    skip_whitespace(line.text, &index);
    
    /* Skip empty or comment lines */
    if (!line.text[index] || line.text[index] == '\n' || line.text[index] == ';') {
        if (DEBUG_FIRST_PASS) {
            printf("[DEBUG] First Pass: Skipping empty or comment line\n");
        }
        return TRUE;
    }
    
    /* Check for label */
    if (get_label(line, symbol)) {
        if (DEBUG_FIRST_PASS) {
            printf("[DEBUG] First Pass: Found label: %s\n", symbol);
        }
        
        /* Invalid label name */
        if (!is_valid_label(symbol)) {
            print_error(line, "Invalid label name: %s", symbol);
            return FALSE;
        }
        
        /* Skip label definition in source */
        while (line.text[index] != ':') index++;
        index++;
        skip_whitespace(line.text, &index);
        
        /* Check if label already exists */
        if (find_symbol(symbols, symbol)) {
            print_error(line, "Label %s already defined", symbol);
            return FALSE;
        }
        
        if (DEBUG_FIRST_PASS) {
            printf("[DEBUG] First Pass: Label %s is valid\n", symbol);
        }
    }
    
    /* Empty line after label */
    if (!line.text[index]) return TRUE;
    
    /* Check for directive */
    dir = get_instruction_type(line, &index);
    if (dir == DIR_ERROR) return FALSE;
    
    skip_whitespace(line.text, &index);
    
    if (DEBUG_FIRST_PASS) {
        if (dir != DIR_NONE) {
            printf("[DEBUG] First Pass: Found directive type: %d\n", dir);
        } else {
            printf("[DEBUG] First Pass: No directive found, treating as code line\n");
        }
    }
    
    /* Handle directives */
    if (dir != DIR_NONE) {
        /* Add symbol to table for .data/.string */
        if ((dir == DIR_DATA || dir == DIR_STRING) && symbol[0]) {
            add_symbol(symbols, symbol, *dc, SYMBOL_DATA);
            if (DEBUG_FIRST_PASS) {
                printf("[DEBUG] First Pass: Added data symbol %s at address %ld\n", symbol, *dc);
            }
        }
        
        /* Process each directive type */
        switch (dir) {
            case DIR_STRING:
                return process_string_inst(line, index, data, dc);
            case DIR_DATA:
                return process_data_inst(line, index, data, dc);
            case DIR_EXTERN:
                return process_extern_inst(line, index, symbols);
            case DIR_ENTRY:
                if (symbol[0]) {
                    print_error(line, "Cannot define label for .entry directive");
                    return FALSE;
                }
                break;
            default:
                break;
        }
        return TRUE;
    }
    
    /* Handle code line */
    if (symbol[0]) {
        add_symbol(symbols, symbol, *ic, SYMBOL_CODE);
        if (DEBUG_FIRST_PASS) {
            printf("[DEBUG] First Pass: Added code symbol %s at address %ld\n", symbol, *ic);
        }
    }
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: Processing code line at IC=%ld\n", *ic);
    }
    return process_code_line(line, index, ic, code);
}

/* Process a code (instruction) line */
static Bool process_code_line(SourceLine line, int index, long *ic, MachineWord **code) {
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: process_code_line - Starting at index %d\n", index);
    }
    char op[MAX_OP_LEN + 1];
    char *operands[2] = {NULL, NULL};
    OpCode opcode;
    FuncCode func;
    InstructionWord *inst;
    int i, op_count;
    MachineWord *word;
    long ic_start;
    AddressMode src_mode = NO_ADDRESSING;
    AddressMode dest_mode = NO_ADDRESSING;
    RegNum src_reg = 0; /* Initialize to 0 as default, not NO_REGISTER (-1) */
    RegNum dest_reg = 0; /* Initialize to 0 as default, not NO_REGISTER (-1) */
    
    /* Get operation name */
    for (i = 0; i < MAX_OP_LEN && line.text[index] && 
        line.text[index] != ' ' && line.text[index] != '\t' && 
        line.text[index] != '\n'; i++, index++) {
        op[i] = line.text[index];
    }
    op[i] = '\0';
    
    /* Get operation details */
    get_operation_details(op, &opcode, &func);
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: Operation: %s, OpCode: %d, Func: %d\n", op, opcode, func);
    }
    
    if (opcode == OP_INVALID) {
        print_error(line, "Invalid operation: %s", op);
        return FALSE;
    }
    
    /* Parse operands */
    if (!parse_operands(line, index, operands, &op_count, op)) {
        if (DEBUG_FIRST_PASS) {
            printf("[DEBUG] First Pass: Failed to parse operands\n");
        }
        return FALSE;
    }
    
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: Found %d operands\n", op_count);
        for (i = 0; i < op_count; i++) {
            printf("[DEBUG] First Pass: Operand %d: %s\n", i+1, operands[i]);
        }
    }

    /* Validate operand count for single-operand instructions */
    if (((opcode == OP_SINGLE) || /* CLR/NOT/INC/DEC */
         (opcode == OP_JUMPS) ||  /* JMP/BNE/JSR */
         (opcode == OP_RED) || 
         (opcode == OP_PRN)) && 
        op_count != 1) {
        print_error(line, "Operation '%s' requires exactly one operand, got %d", op, op_count);
        if (op_count > 0) {
            free(operands[0]);
            if (op_count > 1) {
                free(operands[1]);
            }
        }
        return FALSE;
    }
    
    /* Set addressing modes and register values based on operand count */
    if (op_count == 0) {
        /* No operands (RTS, STOP) - all fields are 0 */
        src_mode = 0;
        dest_mode = 0;
        src_reg = 0;
        dest_reg = 0;
    } else if (op_count == 1) {
        /* For most single-operand instructions, the operand is the destination */
        if (opcode == OP_SINGLE || opcode == OP_JUMPS || opcode == OP_RED) {
            /* No source operand */
            src_mode = 0;
            src_reg = 0;
            
            /* Destination is the operand */
            dest_mode = get_addressing_mode(operands[0]);
            
            /* Set destination register if it's a register */
            if (dest_mode == REGISTER_MODE) {
                dest_reg = (RegNum)(operands[0][1] - '0');
            } else {
                dest_reg = 0; /* Explicitly set to 0 if not a register */
            }
        } else if (opcode == OP_PRN) {
            /* For PRN, the operand is the source */
            dest_mode = 0;
            dest_reg = 0;
            
            /* Source is the operand */
            src_mode = get_addressing_mode(operands[0]);
            
            /* Set source register if it's a register */
            if (src_mode == REGISTER_MODE) {
                src_reg = (RegNum)(operands[0][1] - '0');
            } else {
                src_reg = 0; /* Explicitly set to 0 if not a register */
            }
        }
    } else if (op_count == 2) {
        /* Two-operand instruction */
        src_mode = get_addressing_mode(operands[0]);
        dest_mode = get_addressing_mode(operands[1]);
        
        /* Set register values */
        if (src_mode == REGISTER_MODE) {
            src_reg = (RegNum)(operands[0][1] - '0');
        } else {
            src_reg = 0; /* Explicitly set to 0 if not a register */
        }
        
        if (dest_mode == REGISTER_MODE) {
            dest_reg = (RegNum)(operands[1][1] - '0');
        } else {
            dest_reg = 0; /* Explicitly set to 0 if not a register */
        }
    }
    
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: Source addressing mode: %d\n", src_mode);
        printf("[DEBUG] First Pass: Destination addressing mode: %d\n", dest_mode);
        printf("[DEBUG] First Pass: Source register: %d\n", src_reg);
        printf("[DEBUG] First Pass: Destination register: %d\n", dest_reg);
    }
    
    /* Check for invalid addressing modes */
    if (src_mode == INVALID_ADDR || dest_mode == INVALID_ADDR) {
        if (op_count > 0) {
            free(operands[0]);
            if (op_count > 1) free(operands[1]);
        }
        return FALSE;
    }
    
    /* Create instruction word */
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: Creating instruction word with:\n");
        printf("[DEBUG] First Pass:   OpCode: %d\n", opcode);
        printf("[DEBUG] First Pass:   Function: %d\n", func);
        printf("[DEBUG] First Pass:   Source mode: %d\n", src_mode);
        printf("[DEBUG] First Pass:   Destination mode: %d\n", dest_mode);
        printf("[DEBUG] First Pass:   Source register: %d\n", src_reg);
        printf("[DEBUG] First Pass:   Destination register: %d\n", dest_reg);
    }
    
    inst = create_instruction_word(opcode, func, 
                                 src_mode,
                                 dest_mode,
                                 src_reg,
                                 dest_reg);
    
    /* Store instruction */
    ic_start = *ic;
    word = (MachineWord*)safe_malloc(sizeof(MachineWord));
    word->is_instruction = 1;
    word->content.code = inst;
    code[(*ic)++ - START_IC] = word;
    
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: Stored instruction word at IC=%ld\n", ic_start);
    }
    
    /* Handle additional words for operands */
    if (op_count > 0) {
        if (DEBUG_FIRST_PASS) {
            printf("[DEBUG] First Pass: Processing extra words for operand 1: %s\n", operands[0]);
        }
        handle_extra_words(code, ic, operands[0], opcode);
        free(operands[0]);
        
        if (op_count > 1) {
            if (DEBUG_FIRST_PASS) {
                printf("[DEBUG] First Pass: Processing extra words for operand 2: %s\n", operands[1]);
            }
            handle_extra_words(code, ic, operands[1], opcode);
            free(operands[1]);
        }
    }
    
    /* Set instruction length */
    code[ic_start - START_IC]->is_instruction = (*ic) - ic_start;
    
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: Instruction length: %d words\n", (int)((*ic) - ic_start));
    }
    
    return TRUE;
}

/* Handle additional words needed for operands */
static void handle_extra_words(MachineWord **code, long *ic, char *operand, OpCode opcode) {
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: handle_extra_words for operand: %s\n", operand);
    }
    AddressMode mode = get_addressing_mode(operand);
    MachineWord *word;
    
    /* Skip invalid addressing modes and registers */
    if (mode == INVALID_ADDR) {
        if (DEBUG_FIRST_PASS) {
            printf("[DEBUG] First Pass: Invalid addressing mode, skipping extra words\n");
        }
        return;  /* Error already printed in get_addressing_mode */
    }
    
    if (DEBUG_FIRST_PASS) {
        printf("[DEBUG] First Pass: Operand addressing mode: %d\n", mode);
    }
    
    /* Handle valid addressing modes (except registers which are encoded in instruction) */
    if (mode != NO_ADDRESSING && mode != REGISTER_MODE) {
        if (mode == IMMEDIATE) {
            /* Immediate value - encode now */
            char *ptr;
            long value = strtol(operand + 1, &ptr, 10);
            
            if (DEBUG_FIRST_PASS) {
                printf("[DEBUG] First Pass: Creating data word for immediate value %ld\n", value);
                printf("[DEBUG] First Pass: Using ARE_ABSOLUTE (4) for immediate value\n");
            }
            
            word = (MachineWord*)safe_malloc(sizeof(MachineWord));
            word->is_instruction = 0;
            word->content.data = create_data_word(ARE_ABSOLUTE, value);
            
            code[(*ic)++ - START_IC] = word;
            
            if (DEBUG_FIRST_PASS) {
                printf("[DEBUG] First Pass: Added immediate value %ld at IC=%ld\n", value, (*ic)-1);
                printf("[DEBUG] First Pass: 24-bit word structure:\n");
                printf("[DEBUG] First Pass:   Value (bits 3-23): %ld\n", value);
                printf("[DEBUG] First Pass:   ARE (bits 0-2): %d (absolute)\n", ARE_ABSOLUTE);
                
                /* Calculate the 24-bit word value for debugging */
                unsigned long word_value = ((unsigned long)value << 3) | ARE_ABSOLUTE;
                printf("[DEBUG] First Pass:   24-bit word value: 0x%06lx\n", word_value & 0xFFFFFF);
                
                /* Show binary representation */
                char binary[25];
                unsigned long mask = 1UL << 23;
                int i;
                for (i = 0; i < 24; i++) {
                    binary[i] = (word_value & mask) ? '1' : '0';
                    mask >>= 1;
                }
                binary[24] = '\0';
                printf("[DEBUG] First Pass:   Binary representation: %s\n", binary);
            }
        } else if (mode == DIRECT) {
            /* Label - leave space for address to be filled in second pass */
            if (DEBUG_FIRST_PASS) {
                printf("[DEBUG] First Pass: Direct addressing with label: %s\n", operand);
                printf("[DEBUG] First Pass: Reserving space for label address to be filled in second pass\n");
            }
            
            /* Just reserve space for now - will be filled in second pass */
            (*ic)++;
            
            if (DEBUG_FIRST_PASS) {
                printf("[DEBUG] First Pass: Reserved space for label address at IC=%ld\n", (*ic)-1);
            }
        } else if (mode == RELATIVE) {
            /* Relative label - special handling for jump instructions */
            if (DEBUG_FIRST_PASS) {
                printf("[DEBUG] First Pass: Relative addressing with label: %s\n", operand + 1);
                printf("[DEBUG] First Pass: Reserving space for relative distance to be filled in second pass\n");
            }
            
            /* Verify that relative addressing is only used with jump instructions */
            if (opcode != OP_JUMPS) {
                /* Create a temporary SourceLine for error message */
                SourceLine temp;
                temp.num = 0;
                temp.filename = "";
                temp.text = operand;
                
                print_error(temp, "Relative addressing mode can only be used with jump instructions (jmp, bne, jsr)");
                return;
            }
            
            /* Reserve space for the relative address (distance) */
            (*ic)++;
            
            if (DEBUG_FIRST_PASS) {
                printf("[DEBUG] First Pass: Reserved space for relative address at IC=%ld\n", (*ic)-1);
            }
        }
    } else if (mode == REGISTER_MODE) {
        if (DEBUG_FIRST_PASS) {
            printf("[DEBUG] First Pass: Register mode - no extra words needed (encoded in instruction word)\n");
        }
    }
}
