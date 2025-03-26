/* Code word manipulation functions */
#ifndef CODE_H
#define CODE_H

#include "globals.h"
#include "symbol_table.h"

/* Maximum operation name length */
#define MAX_OP_LEN 4

/* ARE (Absolute/Relocatable/External) bits */
#define ARE_ABSOLUTE 4    /* Bit 2 (value 4) for Absolute */
#define ARE_RELOCATABLE 2 /* Bit 1 (value 2) for Relocatable */
#define ARE_EXTERNAL 1    /* Bit 0 (value 1) for External */

/* Build a code word from components */
InstructionWord* create_instruction_word(
    OpCode op,           /* Operation code */
    FuncCode func,       /* Function code */
    AddressMode src,     /* Source addressing mode */
    AddressMode dest,    /* Destination addressing mode */
    RegNum src_reg,      /* Source register */
    RegNum dest_reg      /* Destination register */
);

/* Build a data word for immediate/direct/relative addressing */
DataWord* create_data_word(
    unsigned are,        /* ARE bits */
    long value          /* Word value */
);

/* Get addressing mode of an operand */
AddressMode get_addressing_mode(const char *operand);

/* Get operation details */
void get_operation_details(
    const char *op_name,    /* Operation name */
    OpCode *op,            /* Output: operation code */
    FuncCode *func         /* Output: function code */
);

/* Parse operands from a line */
Bool parse_operands(
    SourceLine line,      /* Current line */
    int start_idx,        /* Where to start parsing */
    char *operands[2],    /* Output: operand strings */
    int *count,           /* Output: number of operands */
    const char *op_name   /* Operation name for error messages */
);

#endif /* CODE_H */
