/* Symbol table implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"
#include "utils.h"

/* Create new symbol table */
SymbolTable* create_symbol_table(void) {
    SymbolTable* table = (SymbolTable*)safe_malloc(sizeof(SymbolTable));
    table->first = NULL;
    table->last = NULL;
    return table;
}

/* Add symbol to table */
Bool add_symbol(SymbolTable *table, const char *name, long addr, SymbolType type) {
    SymbolEntry *entry;
    
    if (!table || !name) return FALSE;
    
    /* Check if symbol already exists */
    if (find_symbol(table, name)) {
        return FALSE;
    }
    
    /* Create new entry */
    entry = (SymbolEntry*)safe_malloc(sizeof(SymbolEntry));
    entry->name = str_copy(name);
    entry->address = addr;
    entry->type = type;
    entry->next = NULL;
    
    /* Add to list */
    if (!table->first) {
        table->first = entry;
        table->last = entry;
    } else {
        table->last->next = entry;
        table->last = entry;
    }
    
    return TRUE;
}

/* Find symbol by name */
SymbolEntry* find_symbol(SymbolTable *table, const char *name) {
    SymbolEntry *current;
    
    if (!table || !name) return NULL;
    
    /* Search list */
    for (current = table->first; current; current = current->next) {
        if (str_cmp(current->name, name) == 0) {
            return current;
        }
    }
    
    return NULL;
}

/* Find symbol by name and type */
SymbolEntry* find_symbol_by_type(SymbolTable *table, const char *name, SymbolType type) {
    SymbolEntry *current;
    
    if (!table || !name) return NULL;
    
    /* Search list for matching name and type */
    for (current = table->first; current; current = current->next) {
        if (str_cmp(current->name, name) == 0 && current->type == type) {
            return current;
        }
    }
    
    return NULL;
}

/* Update symbol address */
Bool update_symbol_address(SymbolTable *table, const char *name, long new_addr) {
    SymbolEntry *entry = find_symbol(table, name);
    
    if (!entry) return FALSE;
    
    entry->address = new_addr;
    return TRUE;
}

/* Free symbol table memory */
void free_symbol_table(SymbolTable *table) {
    SymbolEntry *current, *next;
    
    if (!table) return;
    
    /* Free all entries */
    current = table->first;
    while (current) {
        next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
    
    free(table);
}
