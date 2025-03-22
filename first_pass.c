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

/* Local function declarations */
static Bool process_code_line(SourceLine line, int index, long *ic, MachineWord **code);
static void handle_extra_words(MachineWord **code, long *ic, char *operand);

/* Process a single line in the first pass */
Bool process_line_first_pass(SourceLine line, long *ic, long *dc, 
                           MachineWord **code, long *data, SymbolTable *symbols) {
    /* Note: In the original implementation, all data is placed AFTER code.
       First we process everything, then at the end of the first pass,
       we update data symbol addresses by adding the final IC value */
    int index = 0;
    char symbol[MAX_SOURCE_LINE];
    Directive dir;
    
    /* Skip whitespace */
    skip_whitespace(line.text, &index);
    
    /* Skip empty or comment lines */
    if (!line.text[index] || line.text[index] == '\n' || line.text[index] == ';')
        return TRUE;
    
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

/* Process a code (instruction) line */
static Bool process_code_line(SourceLine line, int index, long *ic, MachineWord **code) {
    char op[MAX_OP_LEN + 1];
    char *operands[2];
    OpCode opcode;
    FuncCode func;
    InstructionWord *inst;
    int i, op_count;
    MachineWord *word;
    long ic_start;
    AddressMode src_mode;
    AddressMode dest_mode;
    
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
    
    /* Create instruction word */
    /* Get addressing modes and validate */
    src_mode = op_count > 0 ? get_addressing_mode(operands[0]) : NO_ADDRESSING;
    dest_mode = op_count > 1 ? get_addressing_mode(operands[1]) : NO_ADDRESSING;
    
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
                                 op_count > 0 ? R0 : NO_REGISTER,  /* Will be set later if register addressing */
                                 op_count > 1 ? R0 : NO_REGISTER);
    
    /* Store instruction */
    ic_start = *ic;
    word = (MachineWord*)safe_malloc(sizeof(MachineWord));
    word->is_instruction = 1;
    word->content.code = inst;
    code[(*ic)++ - START_IC] = word;
    
    /* Handle additional words for operands */
    if (op_count > 0) {
        handle_extra_words(code, ic, operands[0]);
        free(operands[0]);
        
        if (op_count > 1) {
            handle_extra_words(code, ic, operands[1]);
            free(operands[1]);
        }
    }
    
    /* Set instruction length */
    code[ic_start - START_IC]->is_instruction = (*ic) - ic_start;
    
    return TRUE;
}

/* Handle additional words needed for operands */
static void handle_extra_words(MachineWord **code, long *ic, char *operand) {
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
        } else {
            /* Label - leave space for address to be filled in second pass */
            (*ic)++;
        }
    }
}
