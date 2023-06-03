#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stdlib.h>

typedef struct SymbolTableEntry {
    char * name;
    struct SymbolTableEntry * next;
} SymbolTableEntry;

typedef struct {
    SymbolTableEntry ** entries;
    size_t size;
} SymbolTable;

SymbolTable * new_symbol_table(size_t size);
SymbolTableEntry * new_symbol_entry(char * name, SymbolTableEntry * next);
SymbolTableEntry * insert_symbol(SymbolTable * table, char * name);
SymbolTableEntry * find_symbol(SymbolTable * table, char * name);
SymbolTableEntry * insert_symbol_if_not_found(SymbolTable * table, char * name);
unsigned int hash_symbol(char * name, size_t modulo);

#endif // SYMBOLS_H
