#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include "./helper.h"
#include "./primitive.h"
#include "./constructor.h"
#include "./context.h"

LispBool * TRUE_VALUE;
LispBool * FALSE_VALUE;
LispValue * valueify_bool(bool b);

#define PRIMITIVE_TYPE_PREDICATE(name, lisp_type) \
    LispValue * name(LispCell * args, LispContext * ctx) { \
        return valueify_bool(eval(args->head, ctx)->type == lisp_type); \
    }


#define PRIMITIVE_COMPARISON_OPERATOR(name, op) \
    LispValue * name(LispCell * args, LispContext * ctx) { \
        LispNumber * a = eval(args->head, ctx); \
        LispNumber * b = eval(args->tail->value, ctx); \
        if ( a->type != kNumberValue || b->type != kNumberValue ) \
            exit_message("Operator arguments must be numbers.", -1); \
        return valueify_bool(a->value op b->value); \
    }

#define PRIMITIVE_ARITHMETIC_OPERATOR(name, op) \
    LispValue * name(LispCell * args, LispContext * ctx) { \
        LispNumber * a = eval(args->head, ctx); \
        LispNumber * b = eval(args->tail->value, ctx); \
        if ( a->type != kNumberValue || b->type != kNumberValue ) \
            exit_message("Operator arguments must be numbers.", -1); \
        return new_lisp_number(a->value op b->value); \
    }

#define PRIMITIVE_BOOL_OPERATOR(name, op) \
    LispValue * name(LispCell * args, LispContext * ctx) { \
        LispValue * a = eval(args->head, ctx); \
        LispValue * b = eval(args->tail->value, ctx); \
        return valueify_bool(boolify_value(a) op boolify_value(b)); \
    }

void define_primitive(char * name, PrimitiveFunPtr prim, LispContext * ctx);
void init_primitive_defs(LispContext * ctx);

#endif // PRIMITIVE_H
