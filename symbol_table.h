/* Symbol table management */
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "globals.h"

/* Symbol types */
typedef enum {
    SYMBOL_CODE,     /* Label for code section */
    SYMBOL_DATA,     /* Label for data section */
    SYMBOL_ENTRY,    /* Entry label */
    SYMBOL_EXTERN    /* External label */
} SymbolType;

/* Symbol table entry */
typedef struct symbol_entry {
    char *name;                 /* Symbol name */
    long address;              /* Symbol address/value */
    SymbolType type;          /* Symbol type */
    struct symbol_entry *next; /* Next in linked list */
} SymbolEntry;

/* Symbol table */
typedef struct symbol_table {
    SymbolEntry *first;
    SymbolEntry *last;
} SymbolTable;

/* Create new symbol table */
SymbolTable* create_symbol_table(void);

/* Add symbol to table */
Bool add_symbol(SymbolTable *table, const char *name, long addr, SymbolType type);

/* Find symbol by name */
SymbolEntry* find_symbol(SymbolTable *table, const char *name);

/* Find symbol by name and type */
SymbolEntry* find_symbol_by_type(SymbolTable *table, const char *name, SymbolType type);

/* Update symbol address */
Bool update_symbol_address(SymbolTable *table, const char *name, long new_addr);

/* Free symbol table memory */
void free_symbol_table(SymbolTable *table);

#endif /* SYMBOL_TABLE_H */
