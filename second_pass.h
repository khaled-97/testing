/* Second pass header file */
#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "globals.h"
#include "symbol_table.h"

/* Process a single line in second pass */
Bool process_line_second_pass(
    SourceLine line,      /* Current line */
    long *ic,            /* Instruction counter pointer */
    MachineWord **code,  /* Code image array */
    SymbolTable *symbols /* Symbol table */
);

/* Add symbols to code words */
Bool resolve_symbols(
    SourceLine line,      /* Current line */
    long *ic,            /* Instruction counter */
    MachineWord **code,  /* Code image */
    SymbolTable *symbols /* Symbol table */
);

/* Process operand in second pass */
Bool process_operand_second_pass(
    SourceLine line,      /* Current line */
    long *curr_ic,       /* Current instruction position */
    long *start_ic,      /* Start of instruction */
    char *operand,       /* Operand to process */
    MachineWord **code,  /* Code image */
    SymbolTable *symbols /* Symbol table */
);

#endif /* SECOND_PASS_H */
