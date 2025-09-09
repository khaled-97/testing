/*
 * Machine Code Word Implementation
 *
 * This module handles the creation and manipulation of machine code words:
 * 1. Instruction words (operations with operands)
 * 2. Data words (numeric values and addresses)
 * 3. Operand addressing modes
 * 4. Operation parsing and validation
 *
 * Machine Word Format:
 * - 24 bits total
 * - Instructions: opcode, addressing modes, registers, function, ARE
 * - Data: value (21 bits) + ARE bits (3 bits)
 */
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "binary_machine_code.h"
#include "utils.h"

/*
 * create_instruction_word - Creates a new instruction word
 *
 * Parameters:
 * op: Operation code
 * func: Function code (for specific operation variants)
 * src: Source operand addressing mode
 * dest: Destination operand addressing mode
 * src_reg: Source register number (if register mode)
 * dest_reg: Destination register number (if register mode)
 *
 * Returns:
 * InstructionWord*: Pointer to new instruction word
 *
 * Creates word with absolute addressing by default (ARE = 4)
 */
InstructionWord* create_instruction_word(
    OpCode op, FuncCode func,
    AddressMode src, AddressMode dest,
    RegNum src_reg, RegNum dest_reg) {
    
    InstructionWord* word = (InstructionWord*)safe_malloc(sizeof(InstructionWord));
    
    word->are = ARE_ABSOLUTE;  /* Default to absolute addressing (4 in binary) */
    word->op = op;
    word->func = func;
    word->src_mode = src;
    word->dest_mode = dest;
    word->src_reg = src_reg;
    word->dest_reg = dest_reg;
    return word;
}

/*
 * create_data_word - Creates a new data word
 *
 * Parameters:
 * are: Address Reference type (Absolute/Relocatable/External)
 * value: Numeric value to store (21 bits)
 *
 * Returns:
 * DataWord*: Pointer to new data word
 */
DataWord* create_data_word(unsigned are, long value) {
    DataWord* word = (DataWord*)safe_malloc(sizeof(DataWord));
    
    word->are = are;
    word->value = value;
    
    return word;
}

/*
 * get_addressing_mode - Determines addressing mode of operand
 *
 * Parameters:
 * operand: Operand string to analyze
 *
 * Returns:
 * AddressMode: Type of addressing used
 *   IMMEDIATE (#number)
 *   DIRECT (label)
 *   RELATIVE (&label)
 *   REGISTER_MODE (r0-r7)
 *   NO_ADDRESSING/INVALID_ADDR for errors
 */
AddressMode get_addressing_mode(const char *operand) {
    char *endptr;
    const char *numstr;
    SourceLine temp_line;
    
    if (!operand) return NO_ADDRESSING;
    
    /* Check for immediate addressing (#number) */
    if (operand[0] == '#') {
        numstr = operand + 1;
        
        /* Check if empty after # */
        if (!*numstr) {
            temp_line.num = 0;
            temp_line.filename = "";
            temp_line.text = (char*)operand;
            
            print_error(temp_line, "Missing number after #");
            return NO_ADDRESSING;
        }
        
        /* Attempt to convert to number */
        strtol(numstr, &endptr, 10);
        if (*endptr != '\0') {
            temp_line.num = 0;
            temp_line.filename = "";
            temp_line.text = (char*)operand;
            
            print_error(temp_line, "Invalid immediate value '%s', must be a valid number", numstr);
            return NO_ADDRESSING;
        }
        return IMMEDIATE;
    }
    
    /* Check for relative addressing (&label) */
    if (operand[0] == '&') {
        if (!is_valid_label(operand + 1)) {
            return NO_ADDRESSING;
        }
        return RELATIVE;
    }
    
    /* Check for register (r0-r7) */
    if (operand[0] == 'r') {
        if (strlen(operand) != 2) {
            temp_line.num = 0;
            temp_line.filename = "";
            temp_line.text = (char*)operand;
            
            print_error(temp_line, "Invalid register format '%s', must be r0-r7", operand);
            return INVALID_ADDR;
        }
        if (operand[1] >= '0' && operand[1] <= '7') {
            return REGISTER_MODE;
        }
        /* Invalid register number */
        temp_line.num = 0;
        temp_line.filename = "";
        temp_line.text = (char*)operand;
        
        print_error(temp_line, "Invalid register number '%c', must be between 0-7", operand[1]);
        return INVALID_ADDR;
    }
    
    /* If valid label, assume direct addressing */
    if (is_valid_label(operand)) {
        return DIRECT;
    }
    return NO_ADDRESSING;
}

/*
 * get_operation_details - Gets operation code and function for instruction
 *
 * Parameters:
 * op_name: Name of operation
 * op: Pointer to store operation code
 * func: Pointer to store function code
 *
 * Maps operation names to their codes and specific functions.
 * Sets OP_INVALID if operation not recognized.
 */
void get_operation_details(const char *op_name, OpCode *op, FuncCode *func) {
    struct {
        const char *name;
        OpCode op;
        FuncCode func;
    } ops[] = {
        {"mov", OP_MOV, F_NONE},
        {"cmp", OP_CMP, F_NONE},
        {"add", OP_MATH, F_ADD},
        {"sub", OP_MATH, F_SUB},
        {"lea", OP_LEA, F_NONE},
        {"clr", OP_SINGLE, F_CLR},
        {"not", OP_SINGLE, F_NOT},
        {"inc", OP_SINGLE, F_INC},
        {"dec", OP_SINGLE, F_DEC},
        {"jmp", OP_JUMPS, F_JMP},
        {"bne", OP_JUMPS, F_BNE},
        {"jsr", OP_JUMPS, F_JSR},
        {"red", OP_RED, F_NONE},
        {"prn", OP_PRN, F_NONE},
        {"rts", OP_RTS, F_NONE},
        {"stop", OP_HALT, F_NONE},
        {NULL, OP_INVALID, F_NONE}
    };
    int i;
    
    /* Initialize to invalid */
    *op = OP_INVALID;
    *func = F_NONE;
    
    if (!op_name) return;
    
    /* Find matching operation */
    for (i = 0; ops[i].name; i++) {
        if (str_cmp(op_name, ops[i].name) == 0) {
            *op = ops[i].op;
            *func = ops[i].func;
            return;
        }
    }
}

/*
 * parse_operands - Extracts operands from instruction line
 *
 * Parameters:
 * line: Source line to parse
 * start_idx: Starting position in line
 * operands: Array to store extracted operand strings
 * count: Pointer to store number of operands found
 * op_name: Operation name (for error messages)
 *
 * Returns:
 * Bool: TRUE if operands parsed successfully, FALSE if error
 *
 * Validates operand count against operation requirements
 */
Bool parse_operands(SourceLine line, int start_idx, char *operands[2], 
                   int *count, const char *op_name) {
    int i = start_idx;
    char buffer[MAX_SOURCE_LINE];
    int buf_idx;
    OpCode op;
    FuncCode func;
    
    *count = 0;
    operands[0] = operands[1] = NULL;
    
    skip_whitespace(line.text, &i);
    
    /* Parse up to 2 operands */
    while (line.text[i] && line.text[i] != '\n' && *count < 2) {
        buf_idx = 0;
        
        /* Get operand */
        while (line.text[i] && !strchr(" ,\t\n", line.text[i]) &&
               buf_idx < MAX_SOURCE_LINE - 1) {
            buffer[buf_idx++] = line.text[i++];
        }
        buffer[buf_idx] = '\0';
        
        if (buf_idx == 0) break;
        
        /* Store operand */
        operands[*count] = str_copy(buffer);
        (*count)++;
        
        /* Skip whitespace and comma */
        skip_whitespace(line.text, &i);
        if (line.text[i] == ',') {
            i++;
            skip_whitespace(line.text, &i);
        }
    }
    
    /* Validate number of operands */
    if (line.text[i] && line.text[i] != '\n') {
        if (op_name) {
            print_error(line, "Too many operands for %s", op_name);
        }
        /* Free allocated memory */
        if (operands[0]) {
            free(operands[0]);
            if (operands[1]) free(operands[1]);
        }
        return FALSE;
    }
    
    /* Check for valid number of operands based on operation type */
    get_operation_details(op_name, &op, &func);
    
    /* Validate zero-operand instructions */
    if ((op == OP_RTS || op == OP_HALT) && *count != 0) {
        print_error(line, "Operation '%s' does not accept any operands", op_name);
        if (operands[0]) {
            free(operands[0]);
            if (operands[1]) free(operands[1]);
        }
        return FALSE;
    }
    
    /* Validate two-operand instructions */
    if ((op == OP_MOV || op == OP_CMP || op == OP_MATH || op == OP_LEA) && *count != 2) {
        print_error(line, "Operation '%s' requires exactly two operands, got %d", op_name, *count);
        if (operands[0]) {
            free(operands[0]);
            if (operands[1]) free(operands[1]);
        }
        return FALSE;
    }
    
    return TRUE;
}
