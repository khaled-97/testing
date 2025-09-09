/*
 * Symbol Table Implementation
 *
 * This module implements a symbol table for the assembler that:
 * 1. Stores all symbols (labels) defined in the source code
 * 2. Tracks symbol types (code, data, entry, external)
 * 3. Maintains symbol addresses
 * 4. Provides efficient symbol lookup and management
 *
 * The symbol table is implemented as a linked list for simplicity
 * and flexibility in adding new symbols during assembly.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"
#include "utils.h"

/*
 * create_symbol_table - Creates and initializes a new symbol table
 *
 * Returns:
 * SymbolTable*: Pointer to newly created empty symbol table
 *
 * Creates a symbol table structure with NULL first and last pointers.
 * Uses safe_malloc to ensure memory allocation succeeds.
 */
SymbolTable* create_symbol_table(void) {
    SymbolTable* table = (SymbolTable*)safe_malloc(sizeof(SymbolTable));
    table->first = NULL;
    table->last = NULL;
    
    return table;
}

/*
 * add_symbol - Adds a new symbol to the symbol table
 *
 * Parameters:
 * table: Symbol table to add to
 * name: Name of the symbol
 * addr: Memory address of the symbol
 * type: Type of symbol (code, data, entry, external)
 *
 * Returns:
 * Bool: TRUE if symbol added successfully, FALSE if error
 *       (e.g., NULL parameters or symbol already exists)
 *
 * Allocates new entry and adds to end of symbol list
 */
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

/*
 * find_symbol - Searches for a symbol by name
 *
 * Parameters:
 * table: Symbol table to search in
 * name: Name of symbol to find
 *
 * Returns:
 * SymbolEntry*: Pointer to found symbol entry, NULL if not found
 *
 * Performs linear search through symbol list
 */
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

/*
 * find_symbol_by_type - Searches for a symbol by name and type
 *
 * Parameters:
 * table: Symbol table to search in
 * name: Name of symbol to find
 * type: Type of symbol to match
 *
 * Returns:
 * SymbolEntry*: Pointer to found symbol entry, NULL if not found
 *
 * Used when type-specific lookup is needed (e.g., entry symbols)
 */
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

/*
 * update_symbol_address - Updates the address of an existing symbol
 *
 * Parameters:
 * table: Symbol table containing the symbol
 * name: Name of symbol to update
 * new_addr: New address value
 *
 * Returns:
 * Bool: TRUE if symbol found and updated, FALSE if not found
 */
Bool update_symbol_address(SymbolTable *table, const char *name, long new_addr) {
    
    SymbolEntry *entry = find_symbol(table, name);
    
    if (!entry) {
        return FALSE;
    }
    
    entry->address = new_addr;
    return TRUE;
}

/*
 * free_symbol_table - Deallocates all memory used by symbol table
 *
 * Parameters:
 * table: Symbol table to free
 *
 * Frees all symbol entries, their names, and the table structure.
 * Handles empty table case safely.
 */
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
