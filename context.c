#include "./constructor.h"
#include "./symbols.h"
#include "./context.h"

LispContext * new_context() {
    LispContext * ctx = calloc(sizeof(LispContext), 1);
    ctx->entries = NULL;
    ctx->last_entry = NULL;
    ctx->next = NULL;
    ctx->parent_lambda = NULL;
    ctx->tco_buf = NULL;
    return ctx;
}

// Creates a new context object and chains it to the given context object. Returns the new context object.
LispContext * extend_context(LispContext * ctx, LispLambda * lam) {
    LispContext * new_ctx = new_context();
    new_ctx->next = ctx;
    new_ctx->parent_lambda = lam;
    return new_ctx;
}

LispContextEntry * find_last_context_entry(LispContext * ctx) {
    LispContextEntry * current_entry = ctx->entries;
    while ( current_entry && current_entry->next ) {
        current_entry = current_entry->next;
    }
    return current_entry;
}

LispContextEntry * copy_context_entries(LispContextEntry * entries) {
    LispContextEntry * current_entry = entries;
    LispContextEntry * current_new_entry = NULL;
    LispContextEntry * new_root_entry = NULL;
    while ( current_entry ) {
        if ( current_new_entry ) {
            current_new_entry->next = new_context_entry(current_entry->interned_name, current_entry->value, NULL);
            current_new_entry = current_new_entry->next;
        } else {
            current_new_entry = new_context_entry(current_entry->interned_name, current_entry->value, NULL);
            new_root_entry = current_new_entry;
        }
        current_entry = current_entry->next;
    }
    return new_root_entry;
}

LispContext * copy_context(LispContext * ctx) {
    LispContext * new_ctx = new_context();
    new_ctx->entries = copy_context_entries(ctx->entries);
    new_ctx->last_entry = find_last_context_entry(new_ctx);
    return new_ctx;
}

// Creates a new context entry object with the given interned symbol and value.
LispContextEntry * new_context_entry(char * interned_name, LispValue * value, LispContextEntry * next) {
    LispContextEntry * new_entry = calloc(sizeof(LispContextEntry), 1);
    new_entry->interned_name = interned_name;
    new_entry->value = value;
    new_entry->next = next;
    return new_entry;
}

// Creates a new context entry object by converting the given name into an interned symbol.
LispContextEntry * new_context_entry_by_name(char * interned_name, LispValue * value, LispContextEntry * next) {
    SymbolTableEntry * sym_name = insert_symbol_if_not_found(GLOBAL_SYM_TABLE, interned_name);
    return new_context_entry(sym_name->name, value, next);
}

// Inserts the given interned symbol and value as a context entry into the given context object.
LispContextEntry * insert_context_entry(LispContext * ctx, char * interned_name, LispValue * value) {
    if ( !ctx->last_entry ) {
        ctx->entries = new_context_entry(interned_name, value, NULL);
        ctx->last_entry = ctx->entries;
        return ctx->entries;
    } else {
        ctx->last_entry->next = new_context_entry(interned_name, value, NULL);
        ctx->last_entry = ctx->last_entry->next;
        return ctx->last_entry;
    }
}

// Converts the given name into an interned symbol and inserts the resulting context entry into the context.
LispContextEntry * insert_context_entry_by_name(LispContext * ctx, char * name, LispValue * value) {
    SymbolTableEntry * sym_name = insert_symbol_if_not_found(GLOBAL_SYM_TABLE, name);
    return insert_context_entry(ctx, sym_name->name, value);
}

// Searches for a context entry with the given interned symbol within the given context.
LispContextEntry * find_context_entry(LispContext * ctx, char * interned_name) {
    LispContextEntry * current_entry = ctx->entries;
    while ( current_entry ) {
        if ( current_entry->interned_name == interned_name )
            return current_entry;
        current_entry = current_entry->next;
    }
    return NULL;
}

// Searches for a context entry with the given interned symbol in the given context or chained contexts.
LispContextEntry * find_context_entry_all(LispContext * ctx, char * interned_name) {
    LispContextEntry * current_entry = ctx->entries;
    LispContext * current_ctx = ctx;
    while ( current_ctx ) {
        LispContextEntry * found_entry = find_context_entry(current_ctx, interned_name);
        if ( found_entry )
            return found_entry;
        current_ctx = current_ctx->next;
    }
    return NULL;
}

void print_context(LispContext * ctx) {
    LispContextEntry * current_entry = ctx->entries;
    while ( current_entry ) {
        printf("%s = ", current_entry->interned_name);
        print_value(current_entry->value);
        printf("\n");
        current_entry = current_entry->next;
    }
}
