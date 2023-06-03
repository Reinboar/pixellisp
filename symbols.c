#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "./symbols.h"
#include "./helper.h"

SymbolTable * new_symbol_table(size_t size) {
    SymbolTable * new_table = calloc(sizeof(SymbolTable), 1);
    new_table->size = size;
    new_table->entries = calloc(sizeof(SymbolTableEntry *), size);
    return new_table;
}

SymbolTableEntry * new_symbol_entry(char * name, SymbolTableEntry * next) {
    SymbolTableEntry * new_entry = calloc(sizeof(SymbolTableEntry), 1);
    if ( !new_entry ) {
        exit_message("Error while allocating memory for new symbol entry.", -1);
    }
    new_entry->name = name;
    new_entry->next = next;
    return new_entry;
}

// Computes the hash of the given string.
unsigned int hash_symbol(char * name, size_t modulo) {
    unsigned int hash_value = 0;
    for ( int i = 0 ; name[i] != 0 ; i++ ) {
        hash_value += ((i + 1) * 7) * (int)(name[i]);
    }
    return hash_value % modulo;
}

// Inserts a new symbol into the given table. Does not check for duplicates.
SymbolTableEntry * insert_symbol(SymbolTable * table, char * name) {
    size_t entry_index = hash_symbol(name, table->size);
    SymbolTableEntry * current_entry = table->entries[entry_index];
    if ( !current_entry ) {
        table->entries[entry_index] = new_symbol_entry(name, NULL);
        return table->entries[entry_index];
    }
    while ( current_entry->next ) {
        current_entry = current_entry->next;
    }
    current_entry->next = new_symbol_entry(name, NULL);
    return current_entry->next;
}

// Searches for symbol entry with name. Returns NULL if it can't be found.
SymbolTableEntry * find_symbol(SymbolTable * table, char * name) {
    size_t entry_index = hash_symbol(name, table->size);
    SymbolTableEntry * current_entry = table->entries[entry_index];
    while ( current_entry ) {
        if ( strcmp(name, current_entry->name) == 0 )
            return current_entry;
        current_entry = current_entry->next;
    }
    return NULL;
}

// Inserts a new symbol entry with name and returns it if it's not found. Returns the existing entry otherwise.
SymbolTableEntry * insert_symbol_if_not_found(SymbolTable * table, char * name) {
    SymbolTableEntry * entry = find_symbol(table, name);
    if ( entry )
        return entry;
    return insert_symbol(table, name);
}
