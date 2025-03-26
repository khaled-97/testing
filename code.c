/* Code word manipulation implementation */
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "code.h"
#include "utils.h"

/* Debug flag for code operations */
#define DEBUG_CODE 1

/* Create instruction word from components */
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
    
    if (DEBUG_CODE) {
        printf("[DEBUG] Code: Created instruction word - OpCode: %d, Func: %d, Src Mode: %d, Dest Mode: %d\n", 
               op, func, src, dest);
        printf("[DEBUG] Code: Instruction word bit layout:\n");
        printf("[DEBUG] Code:   OpCode (bits 18-23): %d\n", op);
        printf("[DEBUG] Code:   Source addressing mode (bits 16-17): %d\n", src);
        printf("[DEBUG] Code:   Source register (bits 13-15): %d\n", src_reg);
        printf("[DEBUG] Code:   Destination addressing mode (bits 11-12): %d\n", dest);
        printf("[DEBUG] Code:   Destination register (bits 8-10): %d\n", dest_reg);
        printf("[DEBUG] Code:   Function code (bits 3-7): %d\n", func);
        printf("[DEBUG] Code:   ARE (bits 0-2): %d\n", word->are);
        
        /* Calculate the 24-bit word value for debugging
           According to spec:
           - Bits 23-18: Opcode
           - Bits 17-16: Source addressing mode (reset if no source operand)
           - Bits 15-13: Source register (reset if not register operand)
           - Bits 12-11: Destination addressing mode (reset if no destination)
           - Bits 10-8: Destination register (reset if not register operand)
           - Bits 7-3: Function code
           - Bits 2-0: ARE */
        unsigned long word_value = ((unsigned long)op << 18) |         /* Opcode: bits 23-18 */
                                  ((unsigned long)src << 16) |         /* Source mode: bits 17-16 */
                                  ((unsigned long)src_reg << 13) |     /* Source register: bits 15-13 */
                                  ((unsigned long)dest << 11) |        /* Destination mode: bits 12-11 */
                                  ((unsigned long)dest_reg << 8) |     /* Destination register: bits 10-8 */
                                  ((unsigned long)func << 3) |         /* Function: bits 7-3 */
                                  word->are;                           /* ARE: bits 2-0 */
        
        printf("[DEBUG] Code:   24-bit word value: 0x%06lx\n", word_value & 0xFFFFFF);
        
        /* Show binary representation */
        char binary[25];
        unsigned long mask = 1UL << 23;
        int i;
        for (i = 0; i < 24; i++) {
            binary[i] = (word_value & mask) ? '1' : '0';
            mask >>= 1;
        }
        binary[24] = '\0';
        printf("[DEBUG] Code:   Binary representation: %s\n", binary);
    }
    
    return word;
}

/* Create data word */
DataWord* create_data_word(unsigned are, long value) {
    DataWord* word = (DataWord*)safe_malloc(sizeof(DataWord));
    
    word->are = are;
    word->value = value;
    
    if (DEBUG_CODE) {
        printf("[DEBUG] Code: Created data word - ARE: %u, Value: %ld\n", are, value);
        printf("[DEBUG] Code: Data word bit layout:\n");
        printf("[DEBUG] Code:   Value (bits 3-23): %ld\n", value);
        printf("[DEBUG] Code:   ARE (bits 0-2): %u\n", are);
        
        /* Calculate the 24-bit word value for debugging */
        unsigned long word_value = ((unsigned long)value << 3) | are;
        
        printf("[DEBUG] Code:   24-bit word value: 0x%06lx\n", word_value & 0xFFFFFF);
        
        /* Show binary representation */
        char binary[25];
        unsigned long mask = 1UL << 23;
        int i;
        for (i = 0; i < 24; i++) {
            binary[i] = (word_value & mask) ? '1' : '0';
            mask >>= 1;
        }
        binary[24] = '\0';
        printf("[DEBUG] Code:   Binary representation: %s\n", binary);
    }
    
    return word;
}

/* Get addressing mode of operand */
AddressMode get_addressing_mode(const char *operand) {
    if (DEBUG_CODE) {
        printf("[DEBUG] Code: Getting addressing mode for operand: %s\n", operand ? operand : "NULL");
    }
    char *endptr;
    const char *numstr;
    SourceLine temp_line;
    
    if (!operand) return NO_ADDRESSING;
    
    /* Check for immediate addressing (#number) */
    if (operand[0] == '#') {
        if (DEBUG_CODE) {
            printf("[DEBUG] Code: Detected immediate addressing mode\n");
        }
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
        if (DEBUG_CODE) {
            printf("[DEBUG] Code: Valid immediate value: %s\n", numstr);
        }
        return IMMEDIATE;
    }
    
    /* Check for relative addressing (&label) */
    if (operand[0] == '&') {
        if (DEBUG_CODE) {
            printf("[DEBUG] Code: Detected relative addressing mode\n");
        }
        if (!is_valid_label(operand + 1)) {
            if (DEBUG_CODE) {
                printf("[DEBUG] Code: Invalid label for relative addressing: %s\n", operand + 1);
            }
            return NO_ADDRESSING;
        }
        if (DEBUG_CODE) {
            printf("[DEBUG] Code: Valid relative addressing with label: %s\n", operand + 1);
        }
        return RELATIVE;
    }
    
    /* Check for register (r0-r7) */
    if (operand[0] == 'r') {
        if (DEBUG_CODE) {
            printf("[DEBUG] Code: Detected possible register addressing mode\n");
        }
        if (strlen(operand) != 2) {
            temp_line.num = 0;
            temp_line.filename = "";
            temp_line.text = (char*)operand;
            
            print_error(temp_line, "Invalid register format '%s', must be r0-r7", operand);
            return INVALID_ADDR;
        }
        if (operand[1] >= '0' && operand[1] <= '7') {
            if (DEBUG_CODE) {
                printf("[DEBUG] Code: Valid register: r%c\n", operand[1]);
            }
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
        if (DEBUG_CODE) {
            printf("[DEBUG] Code: Valid label for direct addressing: %s\n", operand);
        }
        return DIRECT;
    }
    
    if (DEBUG_CODE) {
        printf("[DEBUG] Code: No valid addressing mode found for: %s\n", operand);
    }
    return NO_ADDRESSING;
}

/* Get operation details */
void get_operation_details(const char *op_name, OpCode *op, FuncCode *func) {
    if (DEBUG_CODE) {
        printf("[DEBUG] Code: Getting operation details for: %s\n", op_name ? op_name : "NULL");
    }
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
            if (DEBUG_CODE) {
                printf("[DEBUG] Code: Found operation: %s, OpCode: %d, Func: %d\n", 
                       ops[i].name, ops[i].op, ops[i].func);
            }
            return;
        }
    }
    
    if (DEBUG_CODE) {
        printf("[DEBUG] Code: Invalid operation: %s\n", op_name ? op_name : "NULL");
    }
}

/* Parse operands from a line */
Bool parse_operands(SourceLine line, int start_idx, char *operands[2], 
                   int *count, const char *op_name) {
    if (DEBUG_CODE) {
        printf("[DEBUG] Code: Parsing operands starting at index %d\n", start_idx);
    }
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
        if (DEBUG_CODE) {
            printf("[DEBUG] Code: Parsing operand %d\n", *count + 1);
        }
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
        if (DEBUG_CODE) {
            printf("[DEBUG] Code: Found operand %d: %s\n", *count + 1, buffer);
        }
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
            if (DEBUG_CODE) {
                printf("[DEBUG] Code: Too many operands for %s\n", op_name);
            }
        }
        /* Free allocated memory */
        if (operands[0]) {
            free(operands[0]);
            if (operands[1]) free(operands[1]);
        }
        return FALSE;
    }
    
    if (DEBUG_CODE) {
        printf("[DEBUG] Code: Successfully parsed %d operands\n", *count);
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
