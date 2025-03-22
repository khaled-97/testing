/* Assembly instruction handling */
#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "globals.h"
#include "symbol_table.h"

/* Find instruction type from line starting at index */
Directive get_instruction_type(SourceLine line, int *index);

/* Process .data instruction */
Bool process_data_inst(SourceLine line, int start_idx, long *data_img, long *dc);

/* Process .string instruction */
Bool process_string_inst(SourceLine line, int start_idx, long *data_img, long *dc);

/* Process .extern instruction (first pass) */
Bool process_extern_inst(SourceLine, int, SymbolTable*);

/* Process .entry instruction (second pass) */
Bool process_entry_inst(SourceLine, int, SymbolTable*);

/* Validate numeric operand for .data */
Bool is_valid_number(const char *str);

/* Extract numeric value from string */
long get_number(const char *str, Bool *success);

#endif /* INSTRUCTIONS_H */
