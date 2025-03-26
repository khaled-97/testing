/* Symbol table implementation */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"
#include "utils.h"

/* Debug flag for symbol table */
#define DEBUG_SYMBOL_TABLE 1

/* Create new symbol table */
SymbolTable* create_symbol_table(void) {
    SymbolTable* table = (SymbolTable*)safe_malloc(sizeof(SymbolTable));
    table->first = NULL;
    table->last = NULL;
    
    if (DEBUG_SYMBOL_TABLE) {
        printf("[DEBUG] Symbol Table: Created new symbol table\n");
    }
    
    return table;
}

/* Add symbol to table */
Bool add_symbol(SymbolTable *table, const char *name, long addr, SymbolType type) {
    if (DEBUG_SYMBOL_TABLE) {
        printf("[DEBUG] Symbol Table: Adding symbol '%s' with address %ld and type %d\n", 
               name, addr, type);
    }
    SymbolEntry *entry;
    
    if (!table || !name) return FALSE;
    
    /* Check if symbol already exists */
    if (find_symbol(table, name)) {
        if (DEBUG_SYMBOL_TABLE) {
            printf("[DEBUG] Symbol Table: Symbol '%s' already exists\n", name);
        }
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
    
    if (DEBUG_SYMBOL_TABLE) {
        printf("[DEBUG] Symbol Table: Successfully added symbol '%s'\n", name);
    }
    
    return TRUE;
}

/* Find symbol by name */
SymbolEntry* find_symbol(SymbolTable *table, const char *name) {
    if (DEBUG_SYMBOL_TABLE) {
        printf("[DEBUG] Symbol Table: Searching for symbol '%s'\n", name);
    }
    SymbolEntry *current;
    
    if (!table || !name) return NULL;
    
    /* Search list */
    for (current = table->first; current; current = current->next) {
        if (str_cmp(current->name, name) == 0) {
            if (DEBUG_SYMBOL_TABLE) {
                printf("[DEBUG] Symbol Table: Found symbol '%s' with address %ld and type %d\n", 
                       name, current->address, current->type);
            }
            return current;
        }
    }
    
    if (DEBUG_SYMBOL_TABLE) {
        printf("[DEBUG] Symbol Table: Symbol '%s' not found\n", name);
    }
    
    return NULL;
}

/* Find symbol by name and type */
SymbolEntry* find_symbol_by_type(SymbolTable *table, const char *name, SymbolType type) {
    if (DEBUG_SYMBOL_TABLE) {
        printf("[DEBUG] Symbol Table: Searching for symbol '%s' with type %d\n", name, type);
    }
    SymbolEntry *current;
    
    if (!table || !name) return NULL;
    
    /* Search list for matching name and type */
    for (current = table->first; current; current = current->next) {
        if (str_cmp(current->name, name) == 0 && current->type == type) {
            if (DEBUG_SYMBOL_TABLE) {
                printf("[DEBUG] Symbol Table: Found symbol '%s' with type %d and address %ld\n", 
                       name, type, current->address);
            }
            return current;
        }
    }
    
    if (DEBUG_SYMBOL_TABLE) {
        printf("[DEBUG] Symbol Table: Symbol '%s' with type %d not found\n", name, type);
    }
    
    return NULL;
}

/* Update symbol address */
Bool update_symbol_address(SymbolTable *table, const char *name, long new_addr) {
    if (DEBUG_SYMBOL_TABLE) {
        printf("[DEBUG] Symbol Table: Updating address for symbol '%s' to %ld\n", name, new_addr);
    }
    
    SymbolEntry *entry = find_symbol(table, name);
    
    if (!entry) {
        if (DEBUG_SYMBOL_TABLE) {
            printf("[DEBUG] Symbol Table: Symbol '%s' not found for address update\n", name);
        }
        return FALSE;
    }
    
    if (DEBUG_SYMBOL_TABLE) {
        printf("[DEBUG] Symbol Table: Updated symbol '%s' address from %ld to %ld\n", 
               name, entry->address, new_addr);
    }
    
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
