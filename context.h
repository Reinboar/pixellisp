#ifndef CONTEXT_H
#define CONTEXT_H

#include "./constructor.h"
#include <setjmp.h>

typedef struct LispContextEntry {
    char * interned_name;
    LispValue * value;
    struct LispContextEntry * next;
} LispContextEntry;

typedef struct LispContext {
    LispContextEntry * entries;
    LispContextEntry * last_entry;
    LispLambda * parent_lambda;
    jmp_buf * tco_buf;
    struct LispContext * next;
} LispContext;

void init_global_symbol_table(size_t size);

LispContext * new_context();
LispContext * extend_context(LispContext * ctx, LispLambda * lam);
LispContextEntry * new_context_entry(char * interned_name, LispValue * value, LispContextEntry * next);
LispContextEntry * new_context_entry_by_name(char * interned_name, LispValue * value, LispContextEntry * next);
LispContextEntry * insert_context_entry(LispContext * ctx, char * interned_name, LispValue * value);
LispContextEntry * insert_context_entry_by_name(LispContext * ctx, char * name, LispValue * value);
LispContextEntry * find_context_entry(LispContext * ctx, char * interned_name);
LispContextEntry * find_context_entry_all(LispContext * ctx, char * interned_name);
void print_context(LispContext * ctx);

#endif // CONTEXT_H
