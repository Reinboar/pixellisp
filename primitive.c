#include "./helper.h"
#include "./primitive.h"
#include "./constructor.h"
#include "./context.h"
#include "./interpreter.h"
#include <math.h>
#include <setjmp.h>

#define for_each_cell(init_cell_name, init_cell) \
    for ( LispCell * init_cell_name = init_cell ; init_cell_name ; init_cell_name = init_cell_name->tail )

void define_primitive(char * name, PrimitiveFunPtr prim, LispContext * ctx) {
    insert_context_entry_by_name(ctx, name, new_lisp_primitive(prim));
}

void define_symbol(char * name, LispValue * value, LispContext * ctx) {
    insert_context_entry_by_name(ctx, name, value);
}

LispValue * add(LispCell * args, LispContext * ctx) {
    int total = 0;
    for_each_cell(current_cell, args) {
        LispNumber * current_number = eval(current_cell->head, ctx);
        if ( current_number->type != kNumberValue )
            exit_message("Attempt to add non-number.", -1);
        total += current_number->value;
    }
    return new_lisp_number(total);
}

LispValue * multiply(LispCell * args, LispContext * ctx) {
    int total = 1;
    for_each_cell(current_cell, args) {
        LispNumber * current_number = eval(current_cell->head, ctx);
        if ( current_number->type != kNumberValue )
            exit_message("Attempt to add non-number.", -1);
        total *= current_number->value;
    }
    return new_lisp_number(total);
}

LispValue * lisp_print(LispCell * args, LispContext * ctx) {
    for_each_cell(current_cell, args) {
        print_value(eval(current_cell->head, ctx));
        printf(" ");
    }
    printf("\n");
    return NULL;
}

LispValue * lisp_define(LispCell * args, LispContext * ctx) {
    LispSymbol * name_val = args->head;
    LispValue * def_val = eval(args->tail->value, ctx);
    if ( name_val->type != kSymbolValue )
        exit_message("Cannot define value of non-symbol.", -1);
    LispContextEntry * found_entry = find_context_entry(ctx, name_val->value);
    if ( found_entry ) {
        found_entry->value = def_val;
    } else {
        insert_context_entry_by_name(ctx, name_val->value, def_val);
    }
    return NULL;
}

LispValue * lisp_set(LispCell * args, LispContext * ctx) {
    LispSymbol * name_val = args->head;
    LispValue * set_val = eval(args->tail->value, ctx);
    if ( name_val->type != kSymbolValue )
        exit_message("Cannot set the value of non-symbol.", -1);
    LispContextEntry * found_entry = find_context_entry_all(ctx, name_val->value);
    if ( found_entry ) {
        found_entry->value = set_val;
    } else {
        exit_message("Cannot set the value of variable that has not been defined.", -1);
    }
    return NULL;
}

LispValue * lisp_eq(LispCell * args, LispContext * ctx) {
    LispValue * a = eval(args->head, ctx);
    LispValue * b = eval(args->tail->value, ctx);
    return valueify_bool(a == b);
}

LispValue * lisp_eqv(LispCell * args, LispContext * ctx) {
    LispValue * a = eval(args->head, ctx);
    LispValue * b = eval(args->tail->value, ctx);
    if ( (!a && b) || (a && !b) )
        return FALSE_VALUE;
    if ( !a && !b )
        return TRUE_VALUE;
    return valueify_bool(a->value == b->value);
}

LispValue * lisp_defun(LispCell * args, LispContext * ctx) {
    LispCell * name_and_params = args->head;
    LispCell * def_body = args->tail;
    LispSymbol * name_val = name_and_params->head;
    LispCell * params = name_and_params->tail;
    if ( name_val->type != kSymbolValue )
        exit_message("Cannot define value of non-symbol.", -1);
    if ( params && params->type != kCellValue )
        exit_message("Invalid parameter list.", -1);
    LispLambda * defun_lambda = new_lisp_lambda(def_body, params, ctx);
    LispContextEntry * found_entry = find_context_entry(ctx, name_val->value);
    if ( found_entry ) {
        found_entry->value = defun_lambda;
    } else {
        insert_context_entry_by_name(ctx, name_val->value, defun_lambda);
    }
    return NULL;
}

LispValue * lisp_begin(LispCell * args, LispContext * ctx) {
    LispCell * last_expr = NULL;
    for_each_cell(current_cell, args) {
        if ( current_cell->tail ) {
            eval(current_cell->head, ctx);
        } else if ( !current_cell->tail ) {
            last_expr = current_cell;
        }
    }
    LispSymbol * last_lam = eval(last_expr->head->value, ctx);
    if ( last_lam && last_lam == ctx->parent_lambda ) {
        LispValue * jmpval = eval_args(((LispCell *)last_expr->head)->tail, ctx);
        longjmp(ctx->tco_buf, convert_to_jump_val(jmpval));
    }
    return eval(last_expr->head, ctx);
}

LispValue * lisp_if(LispCell * args, LispContext * ctx) {
    LispValue * condition_value = eval(args->head, ctx);
    LispCell * true_cell = args->tail;
    LispLambda * true_cell_lam = NULL;
    if ( true_cell->head->type == kCellValue )
        true_cell_lam = eval(((LispCell * )true_cell->head)->head, ctx);
    LispCell * false_cell = true_cell->tail;
    LispLambda * false_cell_lam = NULL;
    if ( false_cell->head->type == kCellValue )
        false_cell_lam = eval(((LispCell * )false_cell->head)->head, ctx);
    if ( boolify_value(condition_value) ) {
        if ( true_cell_lam && true_cell_lam == ctx->parent_lambda ) {
            LispValue * jmpval = eval_args(((LispCell *)true_cell->head)->tail, ctx);
            longjmp(ctx->tco_buf, convert_to_jump_val(jmpval));
        }
        return eval(true_cell->head, ctx);
    } else {
        if ( false_cell_lam && false_cell_lam == ctx->parent_lambda ) {
            LispValue * jmpval = eval_args(((LispCell *)false_cell->head)->tail, ctx);
            longjmp(ctx->tco_buf, convert_to_jump_val(jmpval));
        }
        return eval(false_cell->head, ctx);
    }
}

LispValue * lisp_quote(LispCell * args, LispContext * ctx) {
    if ( args->tail )
        exit_message("QUOTE only accepts one argument.", -1);
    return args->head;
}

LispCell * eval_unquotes(LispCell * args, LispContext * ctx) {
    LispCell * new_root = new_lisp_cell(NULL, NULL);
    LispCell * new_current_cell = new_root;
    for_each_cell(current_cell, args) {
        LispValue * current_head = current_cell->head;
        if ( current_head->type == kCellValue ) {
            if ( strcmp(((LispCell *)current_head)->head->value, "unquote") == 0 ) {
                new_current_cell->head = eval(((LispCell *)current_head)->tail->value, ctx);
            } else if ( strcmp(((LispCell *)current_head)->head->value, "unquote-flatten") == 0) {
                LispCell * flattened = eval(((LispCell *)current_head)->tail->value, ctx);
                if ( flattened->type != kCellValue )
                    exit_message("Non-list value passed to UNQUOTE-FLATTEN.", -1);
                new_current_cell->head = flattened->head;
                new_current_cell->tail = flattened->tail;
                if ( flattened->tail ) {
                    for_each_cell(current_flat_cell, flattened) {
                        if ( !current_flat_cell->tail )
                            new_current_cell = current_flat_cell;
                    }
                }
            } else {
                new_current_cell->head = eval_unquotes(current_head, ctx);
            }
        } else {
            new_current_cell->head = current_head;
        }
        if ( current_cell->tail ) {
            new_current_cell->tail = new_lisp_cell(NULL, NULL);
            new_current_cell = new_current_cell->tail;
        }
    }
    return new_root;
}

LispValue * lisp_quasiquote(LispCell * args, LispContext * ctx) {
    if ( args->head->type != kCellValue )
        return lisp_quote(args, ctx);
    return eval_unquotes(args->head, ctx);
}

LispValue * lisp_car(LispCell * args, LispContext * ctx) {
    LispCell * cell = eval(args->head, ctx);
    if ( cell->type != kCellValue ) {
        printf("ERROR VALUE: ");
        print_value(cell);
        printf("\n");
        exit_message("Non-list value passed to CAR.", -1);
    }
    return cell->head;
}

LispValue * lisp_cdr(LispCell * args, LispContext * ctx) {
    LispCell * cell = eval(args->head, ctx);
    if ( cell->type != kCellValue )
        exit_message("Non-list value passed to CDR.", -1);
    return cell->tail;
}

LispValue * lisp_cons(LispCell * args, LispContext * ctx) {
    LispCell * new_cell = new_lisp_cell(NULL, NULL);
    new_cell->head = eval(args->head, ctx);
    new_cell->tail = eval(args->tail->value, ctx);
    return new_cell;
}

LispValue * lisp_set_car(LispCell * args, LispContext * ctx) {
    LispCell * pair = eval(args->head, ctx);
    LispValue * new_car = eval(args->tail->value, ctx);
    if ( pair->type != kCellValue )
        exit_message("Invalid pair or list passed to SET-CAR!", -1);
    pair->head = new_car;
    return NULL;
}

LispValue * lisp_set_cdr(LispCell * args, LispContext * ctx) {
    LispCell * pair = eval(args->head, ctx);
    LispValue * new_cdr = eval(args->tail->value, ctx);
    if ( pair->type != kCellValue )
        exit_message("Invalid pair or list passed to SET-CDR!", -1);
    pair->tail = new_cdr;
    return NULL;
}

LispValue * lisp_lambda_func(LispCell * args, LispContext * ctx) {
    LispCell * lambda_params = args->head;
    LispCell * lambda_code = args->tail;
    return new_lisp_lambda(lambda_code, lambda_params, ctx);
}

LispValue * lisp_defmacro(LispCell * args, LispContext * ctx) {
    LispSymbol * macro_name = args->head->value;
    LispCell * macro_params = ((LispCell *)args->head)->tail;
    LispCell * macro_template = args->tail;
    insert_context_entry(ctx, macro_name->value, new_lisp_macro(macro_template, macro_params, ctx));
    return NULL;
}

// Splits the given list into two lists, one containing keys and the other containing values.
// The returned cell's head contains the key list and the tail contains the value list.
LispCell * split_assoc_list(LispCell * list) {
    LispCell * current_cell = list;
    LispCell * key_root = new_lisp_cell(NULL, NULL);
    LispCell * key_list = key_root;
    LispCell * val_root = new_lisp_cell(NULL, NULL);
    LispCell * val_list = val_root;
    while ( current_cell ) {
        LispCell * current_assoc = current_cell->head;
        if ( !current_assoc || current_assoc->type != kCellValue )
            exit_message("Invalid assoc list.", -1);
        if ( current_cell->tail ) {
            key_list = extend_cell(key_list, current_assoc->head);
            val_list = extend_cell(val_list, current_assoc->tail->value);
        } else {
            key_list->head = current_assoc->head;
            val_list->head = current_assoc->tail->value;
        }
        current_cell = current_cell->tail;
    }
    return new_lisp_cell(key_root, val_root);
}

LispValue * lisp_let(LispCell * args, LispContext * ctx) {
    LispSymbol * let_name = NULL;
    LispCell * arg_list = NULL;
    LispCell * let_body = NULL;
    if ( args->head->type == kSymbolValue ) {
        let_name = args->head;
        arg_list = args->tail->value;
        let_body = ((LispCell*)args->tail)->tail;
        if ( !arg_list || arg_list->type != kCellValue )
            exit_message("Invalid argument list in LET.", -1);
    } else if ( args->head->type == kCellValue ) {
        arg_list = args->head;
        let_body = args->tail;
    } else {
        exit_message("LET takes either a symbol or a list for the first argument.", -1);
    }
    LispCell * pair_list = split_assoc_list(arg_list);
    LispContext * let_ctx = new_context_from_args(eval_args(pair_list->tail, ctx), pair_list->head, ctx, NULL);
    let_ctx->parent_lambda = ctx->parent_lambda;
    if ( let_name ) {
        LispLambda * let_lam = new_lisp_lambda(let_body, pair_list->head, let_ctx);
        insert_context_entry(let_ctx, let_name->value, let_lam);
        let_ctx->parent_lambda = let_lam;
        //return eval_lambda(let_lam, pair_list->tail, let_ctx);
    }
    return eval_seq(let_body, let_ctx);
}

LispValue * lisp_eval(LispCell * args, LispContext * ctx) {
    LispCell * eval_code = args->head;
    if ( args->tail )
        exit_message("EVAL takes only one argument.", -1);
    return eval(eval(eval_code, ctx), ctx);
}

LispValue * lisp_is_null(LispCell * args, LispContext * ctx) {
    return valueify_bool(eval(args->head, ctx) == NULL);
}

LispValue * lisp_logical_not(LispCell * args, LispContext * ctx) {
    if ( args->tail )
        exit_message("NOT requires one argument.", -1);
    return valueify_bool(!boolify_value(eval(args->head, ctx)));
}

LispValue * lisp_vector(LispCell * args, LispContext * ctx) {
   size_t vector_len =  cells_length(args);
   LispVector * new_vec = new_lisp_vector(vector_len);
   int i = 0;
   for_each_cell(current_cell, args) {
        LispValue * evaled_arg = eval(current_cell->head, ctx);
        new_vec->value[i].value = evaled_arg->value;
        new_vec->value[i].type = evaled_arg->type;
        new_vec->value[i].extra_value = evaled_arg->extra_value;
        new_vec->value[i].refs = evaled_arg->refs;
        i++;
   }
   return new_vec;
}

LispValue * lisp_vector_ref(LispCell * args, LispContext * ctx) {
    LispVector * vec = eval(args->head, ctx);
    LispNumber * idx = eval(args->tail->value, ctx);
    if ( vec->type != kVectorValue )
        exit_message("Non-vector value passed to VECTOR-REF.", -1);
    if ( idx->type != kNumberValue )
        exit_message("Non-number index passed to VECTOR-REF.", -1);
    if ( idx->value >= vec->length || idx->value < 0 )
        exit_message("Index out of range passed to VECTOR-REF.", -1);
    return &(vec->value[idx->value]);
}

LispValue * lisp_vector_set(LispCell * args, LispContext * ctx) {
    LispVector * vec = eval(args->head, ctx);
    LispNumber * idx = eval(args->tail->value, ctx);
    LispValue * val = eval(((LispCell *)args->tail)->tail->value, ctx);
    if ( vec->type != kVectorValue )
        exit_message("Non-vector value passed to VECTOR-SET.", -1);
    if ( idx->type != kNumberValue )
        exit_message("Non-number index passed to VECTOR-SET.", -1);
    if ( idx->value > vec->length || idx->value < 0 )
        exit_message("Index out of range passed to VECTOR-SET.", -1);
    if ( !val )
        exit_message("Cannot set vector element to NULL.", -1);
    vec->value[idx->value].value = val->value;
    vec->value[idx->value].type = val->type;
    vec->value[idx->value].extra_value = val->extra_value;
    vec->value[idx->value].refs = val->refs;
    return NULL;
}

LispValue * lisp_vector_len(LispCell * args, LispContext * ctx) {
    LispVector * vec = eval(args->head, ctx);
    if ( vec->type != kVectorValue )
        exit_message("Non-vector value passed to VECTOR-LENGTH.", -1);
    return new_lisp_number(vec->length);
}

LispValue * lisp_include_file(LispCell * args, LispContext * ctx) {
    LispString * filename = args->head;
    if ( filename->type != kStringValue )
        exit_message("Filename must be a string.", -1);
    char * filename_str = filename->value;
    FILE * code_file = fopen(filename_str, "r");
    fseek(code_file, 0, SEEK_END);
    long code_size = ftell(code_file);
    fseek(code_file, 0, SEEK_SET);
    char * code_content = malloc(code_size + 1);
    if ( !code_content ) {
        exit_message("Error while allocating memory for code.", -1);
    }
    size_t chars_read = fread(code_content, 1, code_size, code_file);
    if ( chars_read != code_size ) {
        printf("Expected read characters: %d\n", code_size);
        printf("Actual read characters: %d\n", chars_read);
        fclose(code_file);
        exit_message("File read error.", -1);
    }
    fclose(code_file);
    TokenList * code_tokens = tokenize(code_content);
    LispCell * code_ast = construct_ast(code_tokens, NULL);
    return eval_seq(code_ast, ctx);
}

LispValue * lisp_string_conc(LispCell * args, LispContext * ctx) {
    char * str_buf = NULL;
    size_t str_buf_len = 0;
    for_each_cell(current_cell, args) {
        LispString * current_head = eval(current_cell->head, ctx);
        if ( current_head->type != kStringValue )
            exit_message("Non-string value(s) passed to STRING-APPEND.", -1);
        size_t current_head_len = strlen(current_head->value);
        if ( !str_buf ) {
            str_buf_len = current_head_len;
            str_buf = calloc(current_head_len, 1);
            memcpy(str_buf, current_head->value, current_head_len);
        } else {
            size_t old_str_buf_len = str_buf_len;
            str_buf_len += current_head_len;
            str_buf = realloc(str_buf, str_buf_len);
            memcpy(str_buf+old_str_buf_len, current_head->value, current_head_len);
        }
    }
    str_buf = realloc(str_buf, str_buf_len+1);
    str_buf[str_buf_len] = 0;
    return new_lisp_string(str_buf);
}

LispValue * lisp_string_ref(LispCell * args, LispContext * ctx) {
    LispString * str = eval(args->head, ctx);
    LispNumber * idx = eval(args->tail->value, ctx);
    if ( str->type != kStringValue )
        exit_message("Non-string value passed to STRING-REF.", -1);
    if ( idx->type != kNumberValue )
        exit_message("Non-number value passed as index to STRING-REF.", -1);
    return new_lisp_number(str->value[idx->value]);
}

LispValue * lisp_string_len(LispCell * args, LispContext * ctx) {
    LispString * str = eval(args->head, ctx);
    if ( str->type != kStringValue )
        exit_message("Non-string value passed to STRING-LENGTH.", -1);
    return new_lisp_number(strlen(str->value));
}

LispValue * lisp_sym_to_str(LispCell * args, LispContext * ctx) {
    LispSymbol * sym = eval(args->head, ctx);
    if ( sym->type != kSymbolValue )
        exit_message("Non-symbol value passed to SYMBOL->STRING.", -1);
    char * new_str_value = malloc(strlen(sym->value)+1);
    strcpy(new_str_value, sym->value);
    return new_lisp_string(new_str_value);
}

LispValue * lisp_str_to_sym(LispCell * args, LispContext * ctx) {
    LispString * str = eval(args->head, ctx);
    if ( str->type != kStringValue )
        exit_message("Non-string value passed to STRING->SYMBOL.", -1);
    SymbolTableEntry * new_sym_entry = insert_symbol_if_not_found(GLOBAL_SYM_TABLE, str->value);
    return new_lisp_symbol(new_sym_entry->name);
}

PRIMITIVE_COMPARISON_OPERATOR(lisp_greater_than, >)
PRIMITIVE_COMPARISON_OPERATOR(lisp_less_than, <)
PRIMITIVE_COMPARISON_OPERATOR(lisp_equal, ==)
PRIMITIVE_COMPARISON_OPERATOR(lisp_not_equal, !=)
PRIMITIVE_COMPARISON_OPERATOR(lisp_greater_or_equal, >=)
PRIMITIVE_COMPARISON_OPERATOR(lisp_less_or_equal, <=)

PRIMITIVE_ARITHMETIC_OPERATOR(lisp_bitwise_or, |)
PRIMITIVE_ARITHMETIC_OPERATOR(lisp_bitwise_and, &)
PRIMITIVE_ARITHMETIC_OPERATOR(lisp_bitwise_xor, ^)
PRIMITIVE_ARITHMETIC_OPERATOR(lisp_divide, /)
PRIMITIVE_ARITHMETIC_OPERATOR(lisp_subtract, -)
PRIMITIVE_ARITHMETIC_OPERATOR(lisp_modulo, %)

PRIMITIVE_BOOL_OPERATOR(lisp_logical_or, ||)
PRIMITIVE_BOOL_OPERATOR(lisp_logical_and, &&)

PRIMITIVE_TYPE_PREDICATE(lisp_list_p, kCellValue)
PRIMITIVE_TYPE_PREDICATE(lisp_symbol_p, kSymbolValue)
PRIMITIVE_TYPE_PREDICATE(lisp_string_p, kStringValue)
PRIMITIVE_TYPE_PREDICATE(lisp_number_p, kNumberValue)
PRIMITIVE_TYPE_PREDICATE(lisp_lambda_p, kLambdaValue)
PRIMITIVE_TYPE_PREDICATE(lisp_primitive_p, kPrimitiveValue)
PRIMITIVE_TYPE_PREDICATE(lisp_macro_p, kMacroValue)
PRIMITIVE_TYPE_PREDICATE(lisp_vector_p, kVectorValue);

LispValue * valueify_bool(bool b) {
  if ( b ) {
    return TRUE_VALUE;
  } else {
    return FALSE_VALUE;
  }
}

void init_primitive_defs(LispContext * ctx) {
    TRUE_VALUE = new_lisp_bool(true);
    FALSE_VALUE = new_lisp_bool(false);
    define_symbol("true", TRUE_VALUE, ctx);
    define_symbol("false", FALSE_VALUE, ctx);
    define_primitive("or", lisp_logical_or, ctx);
    define_primitive("and", lisp_logical_and, ctx);
    define_primitive("not", lisp_logical_not, ctx);
    define_symbol("null", NULL, ctx);
    define_primitive("+", add, ctx);
    define_primitive("-", lisp_subtract, ctx);
    define_primitive("*", multiply, ctx);
    define_primitive("print", lisp_print, ctx);
    define_primitive("define", lisp_define, ctx);
    define_primitive("set!", lisp_set, ctx);
    define_primitive("defmacro", lisp_defmacro, ctx);
    define_primitive("defun", lisp_defun, ctx);
    define_primitive("begin", lisp_begin, ctx);
    define_primitive("if", lisp_if, ctx);
    define_primitive("quote", lisp_quote, ctx);
    define_primitive("quasiquote", lisp_quasiquote, ctx);
    define_primitive("car", lisp_car, ctx);
    define_primitive("cdr", lisp_cdr, ctx);
    define_primitive("set-car!", lisp_set_car, ctx);
    define_primitive("set-cdr!", lisp_set_cdr, ctx);
    define_primitive("cons", lisp_cons, ctx);
    define_primitive(">", lisp_greater_than, ctx);
    define_primitive("<", lisp_less_than, ctx);
    define_primitive("=", lisp_equal, ctx);
    define_primitive("!=", lisp_not_equal, ctx);
    define_primitive(">=", lisp_greater_or_equal, ctx);
    define_primitive("<=", lisp_less_or_equal, ctx);
    define_primitive("/", lisp_divide, ctx);
    define_primitive("%", lisp_modulo, ctx);
    define_primitive("|", lisp_bitwise_or, ctx);
    define_primitive("&", lisp_bitwise_and, ctx);
    define_primitive("^", lisp_bitwise_xor, ctx);
    define_primitive("lambda", lisp_lambda_func, ctx);
    define_primitive("eval", lisp_eval, ctx);
    define_primitive("null?", lisp_is_null, ctx);
    define_primitive("number?", lisp_number_p, ctx);
    define_primitive("string?", lisp_string_p, ctx);
    define_primitive("symbol?", lisp_symbol_p, ctx);
    define_primitive("lambda?", lisp_lambda_p, ctx);
    define_primitive("primitive?", lisp_primitive_p, ctx);
    define_primitive("macro?", lisp_macro_p, ctx);
    define_primitive("vector?", lisp_vector_p, ctx);
    define_primitive("list?", lisp_list_p, ctx);
    define_primitive("eq?", lisp_eq, ctx);
    define_primitive("eqv?", lisp_eqv, ctx);
    define_primitive("let", lisp_let, ctx);
    define_primitive("include", lisp_include_file, ctx);
    define_primitive("vector", lisp_vector, ctx);
    define_primitive("vector-ref", lisp_vector_ref, ctx);
    define_primitive("vector-set!", lisp_vector_set, ctx);
    define_primitive("vector-length", lisp_vector_len, ctx);
    define_primitive("conc", lisp_string_conc, ctx);
    define_primitive("string-ref", lisp_string_ref, ctx);
    define_primitive("string-length", lisp_string_len, ctx);
    define_primitive("string->symbol", lisp_str_to_sym, ctx);
    define_primitive("symbol->string", lisp_sym_to_str, ctx);
}
