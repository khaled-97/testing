/*
 * First Pass Implementation
 * 
 * This module handles the first pass of the two-pass assembler.
 * During the first pass, the assembler:
 * 1. Builds the symbol table by processing labels
 * 2. Encodes instructions and their operands
 * 3. Processes directives (.data, .string, .extern, .entry)
 * 4. Calculates addresses for code (IC) and data (DC) segments
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "first_pass.h"
#include "binary_machine_code.h"
#include "utils.h"
#include "instructions.h"
#include "symbol_table.h"

/* Forward declarations of internal functions */
static Bool process_code_line(SourceLine line, int index, long *ic, MachineWord **code);
static void handle_extra_words(MachineWord **code, long *ic, char *operand, OpCode opcode);

/*
 * process_line_first_pass - Processes a single line during the first pass
 * 
 * Parameters:
 * line: Source line containing text, line number, and filename
 * ic: Pointer to instruction counter
 * dc: Pointer to data counter
 * code: Array to store machine code instructions
 * data: Array to store data values
 * symbols: Symbol table for storing labels
 * 
 * Returns:
 * Bool: TRUE if line processed successfully, FALSE if error occurred
 * 
 * This function:
 * 1. Processes labels and adds them to symbol table
 * 2. Handles directives (.data, .string, .extern, .entry)
 * 3. Processes and encodes instructions
 * 4. Updates IC and DC counters accordingly
 */
Bool process_line_first_pass(SourceLine line, long *ic, long *dc, 
                           MachineWord **code, long *data, SymbolTable *symbols) {
    int index = 0;
    char symbol[MAX_SOURCE_LINE];
    Directive dir;
    
    /* Skip whitespace */
    skip_whitespace(line.text, &index);
    
    /* Skip empty or comment lines */
    if (!line.text[index] || line.text[index] == '\n' || line.text[index] == ';') {
        return TRUE;
    }
    
    /* Check for label */
    if (get_label(line, symbol)) {
        
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
    }
    
    /* Empty line after label */
    if (!line.text[index]) return TRUE;
    
    /* Check for directive */
    dir = get_instruction_type(line, &index);
    if (dir == DIR_ERROR) return FALSE;
    
    skip_whitespace(line.text, &index);
    
    /* Handle directives */
    if (dir != DIR_NONE) {
        /* Add symbol to table for .data/.string */
        if ((dir == DIR_DATA || dir == DIR_STRING) && symbol[0]) {
            add_symbol(symbols, symbol, *dc, SYMBOL_DATA);
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
    }
    return process_code_line(line, index, ic, code);
}

/*
 * process_code_line - Processes and encodes an instruction line
 * 
 * Parameters:
 * line: Source line to process
 * index: Current position in the line
 * ic: Pointer to instruction counter
 * code: Array to store encoded machine code
 * 
 * Returns:
 * Bool: TRUE if instruction encoded successfully, FALSE if error occurred
 * 
 * This function:
 * 1. Parses the operation and its operands
 * 2. Validates operand count and addressing modes
 * 3. Creates instruction words with appropriate encoding
 * 4. Handles additional words for operands as needed
 */
static Bool process_code_line(SourceLine line, int index, long *ic, MachineWord **code) {
    char op[MAX_OP_LEN + 1];                /* Operation name buffer */
    char *operands[2] = {NULL, NULL};       /* Array to store operand strings */
    OpCode opcode;                          /* Operation code (type of instruction) */
    FuncCode func;                          /* Function code for specific operation */
    InstructionWord *inst;                  /* Encoded instruction word */
    int i, op_count;                        /* Loop counter and operand count */
    MachineWord *word;                      /* Machine word for storing in code array */
    long ic_start;                          /* Starting IC for calculating instruction length */
    AddressMode src_mode = NO_ADDRESSING;   /* Addressing mode of source operand */
    AddressMode dest_mode = NO_ADDRESSING;  /* Addressing mode of destination operand */
    RegNum src_reg = 0;                     /* Register number for source operand (0 default) */
    RegNum dest_reg = 0;                    /* Register number for destination operand (0 default) */
    
    /* Get operation name */
    for (i = 0; i < MAX_OP_LEN && line.text[index] && 
        line.text[index] != ' ' && line.text[index] != '\t' && 
        line.text[index] != '\n'; i++, index++) {
        op[i] = line.text[index];
    }
    op[i] = '\0';
    
    /* Get operation details */
    get_operation_details(op, &opcode, &func);
    
    if (opcode == OP_INVALID) {
        print_error(line, "Invalid operation: %s", op);
        return FALSE;
    }
    
    /* Parse operands */
    if (!parse_operands(line, index, operands, &op_count, op)) {
        return FALSE;
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
    
    /* Check for invalid addressing modes */
    if (src_mode == INVALID_ADDR || dest_mode == INVALID_ADDR) {
        if (op_count > 0) {
            free(operands[0]);
            if (op_count > 1) free(operands[1]);
        }
        return FALSE;
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
    
    /* Handle additional words for operands */
    if (op_count > 0) {
        handle_extra_words(code, ic, operands[0], opcode);
        free(operands[0]);
        
        if (op_count > 1) {
            handle_extra_words(code, ic, operands[1], opcode);
            free(operands[1]);
        }
    }
    
    /* Set instruction length */
    code[ic_start - START_IC]->is_instruction = (*ic) - ic_start;
    return TRUE;
}

/*
 * handle_extra_words - Creates additional words needed for operands
 * 
 * Parameters:
 * code: Array to store machine code
 * ic: Pointer to instruction counter
 * operand: Operand string to process
 * opcode: Operation code for validation
 * 
 * This function:
 * 1. Processes immediate values and encodes them
 * 2. Reserves space for labels (resolved in second pass)
 * 3. Handles relative addressing for jump instructions
 * 4. Updates instruction counter for additional words
 */
static void handle_extra_words(MachineWord **code, long *ic, char *operand, OpCode opcode) {
    AddressMode mode = get_addressing_mode(operand);
    MachineWord *word;
    
    /* Skip invalid addressing modes and registers */
    if (mode == INVALID_ADDR) {
        return;  /* Error already printed in get_addressing_mode */
    }
    
    /* Handle valid addressing modes (except registers which are encoded in instruction) */
    if (mode != NO_ADDRESSING && mode != REGISTER_MODE) {
        if (mode == IMMEDIATE) {
            /* Immediate value - encode now */
            char *ptr;
            long value = strtol(operand + 1, &ptr, 10);
            
            word = (MachineWord*)safe_malloc(sizeof(MachineWord));
            word->is_instruction = 0;
            word->content.data = create_data_word(ARE_ABSOLUTE, value);
            
            code[(*ic)++ - START_IC] = word;
            
        } else if (mode == DIRECT) {
            
            /* Just reserve space for now - will be filled in second pass */
            (*ic)++;
            
        } else if (mode == RELATIVE) {
            
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
        }
    }
}
