/* First pass header file */
#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "globals.h"
#include "symbol_table.h"

/* Process a single line in the first pass */
Bool process_line_first_pass(
    SourceLine line,      /* Current line being processed */
    long *ic,            /* Instruction counter pointer */
    long *dc,            /* Data counter pointer */
    MachineWord **code,  /* Code image array */
    long *data,          /* Data image array */
    SymbolTable *symbols /* Symbol table */
);

#endif /* FIRST_PASS_H */
