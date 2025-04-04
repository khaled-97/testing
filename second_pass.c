/* Second pass implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "second_pass.h"
#include "code.h"
#include "utils.h"
#include "instructions.h"
#include "symbol_table.h"

/* Process a single line in the second pass */
Bool process_line_second_pass(SourceLine line, long *ic, MachineWord **code, SymbolTable *symbols) {
    int index = 0;
    char label[MAX_SOURCE_LINE];
    SymbolEntry *entry;
    
    /* Debug print entire line */
    printf("Second pass processing line: %s", line.text);
    
    /* Skip whitespace and comments */
    skip_whitespace(line.text, &index);
    if (!line.text[index] || line.text[index] == ';' || line.text[index] == '\n') 
        return TRUE;
    
    /* Skip label if exists */
    if (str_chr(line.text, ':')) {
        while (line.text[index] && line.text[index] != ':') index++;
        index++;
    }
    
    skip_whitespace(line.text, &index);
    
    /* Handle .entry directive */
    if (line.text[index] == '.') {
        /* Check for .entry */
        if (strncmp(line.text + index, ".entry", 6) == 0) {
            index += 6;
            skip_whitespace(line.text, &index);
            
            /* Get label name */
            if (!line.text[index]) {
                print_error(line, "Missing label name for .entry directive");
                return FALSE;
            }
            
            /* Extract label name */
            {
                int i;
                for (i = 0; line.text[index] && !strchr(" \t\n", line.text[index]); i++, index++) {
                    if (i >= MAX_SOURCE_LINE - 1) {
                        print_error(line, "Label name too long");
                        return FALSE;
                    }
                    label[i] = line.text[index];
                }
                label[i] = '\0';
            }
            
            /* Remove & if relative addressing */
            if (label[0] == '&') {
                int i;
                for (i = 0; label[i + 1]; i++) {
                    label[i] = label[i + 1];
                }
                label[i] = '\0';
            }
            
            /* Check if already marked as entry */
            if (!find_symbol_by_type(symbols, label, SYMBOL_ENTRY)) {
                /* Look for symbol definition */
                entry = find_symbol_by_type(symbols, label, SYMBOL_CODE);
                if (!entry) entry = find_symbol_by_type(symbols, label, SYMBOL_DATA);
                
                if (!entry) {
                    /* Check if external */
                    if (find_symbol_by_type(symbols, label, SYMBOL_EXTERN)) {
                        print_error(line, "Symbol %s cannot be both external and entry", label);
                        return FALSE;
                    }
                    print_error(line, "Undefined symbol %s for .entry", label);
                    return FALSE;
                }
                
                /* Mark symbol as entry */
                entry->type = SYMBOL_ENTRY;
            }
        }
        return TRUE;
    }
    
    /* Handle code line symbols */
    return resolve_symbols(line, ic, code, symbols);
}

/* Get opcode from instruction line */
static OpCode get_opcode_from_line(SourceLine line) {
    int index = 0;
    char op[MAX_OP_LEN + 1];
    int i;
    OpCode opcode;
    FuncCode func;
    
    /* Skip label if exists */
    if (str_chr(line.text, ':')) {
        while (line.text[index] && line.text[index] != ':') index++;
        index++;
    }
    
    skip_whitespace(line.text, &index);
    
    /* Get operation name */
    for (i = 0; i < MAX_OP_LEN && line.text[index] && 
        line.text[index] != ' ' && line.text[index] != '\t' && 
        line.text[index] != '\n'; i++, index++) {
        op[i] = line.text[index];
    }
    op[i] = '\0';
    
    /* Get operation details */
    get_operation_details(op, &opcode, &func);
    return opcode;
}

/* Add symbols to code words */
Bool resolve_symbols(SourceLine line, long *ic, MachineWord **code, SymbolTable *symbols) {
    char label[MAX_SOURCE_LINE];
    char *operands[2];
    int index = 0, op_count;
    Bool success = TRUE;
    long curr_ic = *ic;
    int inst_len;
    OpCode opcode;
    
    /* Get instruction length */
    inst_len = code[(*ic) - START_IC]->is_instruction;
    printf("Line: '%s', instruction length: %d, ic: %ld\n", line.text, inst_len, *ic);
    
    /* Don't skip operations with length 1 - they may still have operands that need symbol resolution */
    
    /* Skip label if exists */
    if (get_label(line, label)) {
        while (line.text[index] != ':') index++;
        index++;
    }
    
    skip_whitespace(line.text, &index);
    
    /* Get opcode for later validation */
    opcode = get_opcode_from_line(line);
    
    /* Skip operation name */
    while (line.text[index] && !strchr(" \t\n", line.text[index])) index++;
    skip_whitespace(line.text, &index);
    
    /* Parse operands */
    if (!parse_operands(line, index, operands, &op_count, "")) {
        return FALSE;
    }
    
    /* Process operands */
    if (op_count > 0) {
        success = process_operand_second_pass(line, &curr_ic, ic, operands[0], code, symbols, opcode);
        free(operands[0]);
        
        if (success && op_count > 1) {
            success = process_operand_second_pass(line, &curr_ic, ic, operands[1], code, symbols, opcode);
            free(operands[1]);
        }
    }
    
    /* Update IC */
    *ic += inst_len;
    return success;
}

/* Process operand in second pass */
Bool process_operand_second_pass(SourceLine line, long *curr_ic, long *start_ic, 
                               char *operand, MachineWord **code, SymbolTable *symbols,
                               OpCode opcode) {
    AddressMode mode = get_addressing_mode(operand);
    MachineWord *word;
    
    /* Skip immediate addressing (already handled) and registers */
    if (mode == IMMEDIATE || mode == REGISTER_MODE) {
        if (mode == IMMEDIATE) (*curr_ic)++;
        return TRUE;
    }
    
    /* Handle direct and relative addressing */
    if (mode == DIRECT || mode == RELATIVE) {
        long value;
        SymbolEntry *symbol;
        char *sym_name = operand;
        unsigned int are_value;
        
        /* Remove & for relative addressing */
        if (mode == RELATIVE) sym_name++;
        
        /* Debug print */
        printf("Processing operand: %s (mode: %d)\n", operand, mode);
        
        /* Look for symbol */
        symbol = find_symbol(symbols, sym_name);
        if (!symbol) {
            print_error(line, "Undefined symbol: %s", sym_name);
            return FALSE;
        }
        
        /* Debug print */
        printf("Found symbol: %s, type: %d, address: %ld\n", 
               symbol->name, symbol->type, symbol->address);
        
        /* Validate relative addressing usage with jump instructions */
        if (mode == RELATIVE && opcode != OP_JUMPS) {
            print_error(line, "Relative addressing mode (&) can only be used with jump instructions (jmp, bne, jsr)");
            return FALSE;
        }
        
        /* Calculate value based on addressing mode */
        if (mode == DIRECT) {
            /* Direct addressing - use the symbol's address */
            value = symbol->address;
            
            /* Set the A, R, E bits */
            if (symbol->type == SYMBOL_EXTERN) {
                are_value = ARE_EXTERNAL; /* External symbol - set E bit */
            } else {
                are_value = ARE_RELOCATABLE; /* Internal symbol - set R bit */
            }
        } else { /* RELATIVE */
            /* Relative addressing - calculate distance between current instruction and target */
            if (symbol->type != SYMBOL_CODE) {
                print_error(line, "Symbol %s must be a code label for relative addressing", sym_name);
                return FALSE;
            }
            
            /* Calculate distance in memory words */
            value = symbol->address - (*start_ic);
            
            /* For relative addressing, A bit is set, R and E bits are 0 */
            are_value = ARE_ABSOLUTE;
        }
        
        /* Handle external references */
        if (mode == DIRECT && symbol->type == SYMBOL_EXTERN) {
            /* Create a new entry for this reference with the actual address */
            SymbolEntry *new_entry = (SymbolEntry*)safe_malloc(sizeof(SymbolEntry));
            /* Allocate memory for name and copy it (ANSI C compliant) */
            new_entry->name = (char*)safe_malloc(strlen(sym_name) + 1);
            strcpy(new_entry->name, sym_name);
            new_entry->address = (*curr_ic) + 1;
            new_entry->type = SYMBOL_EXTERN;
            new_entry->next = NULL;
            
            /* Add to the end of the symbol table */
            if (!symbols->first) {
                symbols->first = new_entry;
                symbols->last = new_entry;
            } else {
                symbols->last->next = new_entry;
                symbols->last = new_entry;
            }
        }
        
        /* Create data word */
        word = (MachineWord*)safe_malloc(sizeof(MachineWord));
        word->is_instruction = 0;
        word->content.data = create_data_word(are_value, value);
        
        code[(++(*curr_ic)) - START_IC] = word;
    }
    
    return TRUE;
}
