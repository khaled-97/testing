/* Output file writing functions */
#ifndef WRITEFILES_H
#define WRITEFILES_H

#include "globals.h"
#include "symbol_table.h"

/* Write object file (.ob) - machine code in special format */
Bool write_object_file(
    const char *base_name,     /* File name without extension */
    MachineWord **code,        /* Code image array */
    long *data,                /* Data image array */
    long ic,                   /* Final instruction counter */
    long dc                    /* Final data counter */
);

/* Write entry file (.ent) - list of entry symbols */
Bool write_entry_file(
    const char *base_name,     /* File name without extension */
    SymbolTable *symbols       /* Symbol table with entries */
);

/* Write external file (.ext) - list of external references */
Bool write_extern_file(
    const char *base_name,     /* File name without extension */
    SymbolTable *symbols       /* Symbol table with externals */
);

#endif /* WRITEFILES_H */
