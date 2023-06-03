#include "./helper.h"
#include "./constructor.h"
#include "./context.h"
#include "./interpreter.h"
#include <setjmp.h>

// Both the following functions are incredibly platform-specific and dirty and stinky.
// Any potential ports will have to fit these to the memory layout of their target platform.
// NOTE: program will crash in gdb unless you set disable-randomization to off because of these functions.
// Like I said, dirty and stinky.
LispValue * convert_to_jump_val(LispValue * val) {
    if ( val == NULL )
        return 2;
    return val;
}

LispValue * convert_from_jump_val(LispValue * val) {
    if ( val == 2 )
        return NULL;
    return 0x7FFF00000000 | (0xFFFFFFFF & (int)val); // <-- the dirty and stinky culprit
}

LispContext * new_context_from_args(LispCell * args, LispCell * params, LispContext * parent_ctx, LispLambda * parent_lam) {
    LispCell * current_arg = args;
    LispCell * current_param = params;
    LispContext * new_ctx = new_context();
    new_ctx->parent_lambda = parent_lam;
    new_ctx->next = parent_ctx;
    while ( current_param ) {
        if ( current_param->head->type != kSymbolValue )
            exit_message("Encountered non-symbol value in parameter list.", -1);
        LispSymbol * param_sym = current_param->head;
        if ( current_arg ) {
            insert_context_entry(new_ctx, param_sym->value, current_arg->head);
            current_arg = current_arg->tail;
        } else {
            insert_context_entry(new_ctx, param_sym->value, NULL);
        }
        current_param = current_param->tail;
        if ( current_param && current_param->type != kCellValue ) {
            param_sym = current_param;
            insert_context_entry(new_ctx, param_sym->value, current_arg);
            return new_ctx;
        }
    }
    if ( current_arg )
        exit_message("Too many arguments passed to lambda.", -1);
    return new_ctx;
}

LispValue * eval_args(LispCell * args, LispContext * ctx) {
    LispCell * new_root_args = new_lisp_cell(NULL, NULL);
    LispCell * new_current_arg = new_root_args;
    LispCell * new_last_arg = NULL;
    LispCell * current_arg = args;
    while ( current_arg ) {
        new_last_arg = new_current_arg;
        new_current_arg = extend_cell(new_current_arg, eval(current_arg->head, ctx));
        current_arg = current_arg->tail;
    }
    if ( new_root_args->head ) {
        new_last_arg->tail = NULL;
        return new_root_args;
    } else {
        return NULL;
    }
}

LispValue * eval_lambda(LispLambda * lambda, LispCell * args, LispContext * ctx) {
    LispContext * lambda_ctx = new_context_from_args(eval_args(args, ctx), lambda->value->params, lambda->ctx, lambda);
    return eval_seq(lambda->value->code, lambda_ctx);
}

LispValue * eval_macro(LispMacro * macro, LispCell * args, LispContext * ctx) {
    LispContext * macro_ctx = new_context_from_args(args, macro->value->params, ctx, NULL);
    return eval(eval_seq(macro->value->template, macro_ctx), ctx);
}

LispValue * eval_cell(LispCell * cell, LispContext * ctx) {
    if ( !cell )
        return NULL;
    LispValue * first_val = eval(cell->head, ctx);
    LispValue * other_vals = cell->tail;
    if ( first_val->type == kLambdaValue ) {
        return eval_lambda(first_val, other_vals, ctx);
    } else if ( first_val->type == kMacroValue ) {
        return eval_macro(first_val, other_vals, ctx);
    } else if ( first_val->type == kPrimitiveValue ) {
        PrimitiveFunPtr prim_ptr = first_val->value;
        return (*prim_ptr)(other_vals, ctx);
    }
    printf("HEAD OF LIST: ");
    print_value(eval(cell->head, ctx)->value);
    printf("\n");
    exit_message("Encountered value other than lambda, macro, or primitive at head of list.", -1);
}

LispValue * eval_seq(LispCell * cell, LispContext * ctx) {
    ctx->tco_buf = calloc(sizeof(jmp_buf), 1);
    int return_val = setjmp(ctx->tco_buf);
    //printf("TCO ENV HAS BEEN SET: 0x%x\n", ctx->tco_buf);
    //printf("SETJMP RETURN VALUE: 0x%x\n\n", return_val);
    if ( return_val ) {
        //printf("NESTED TAIL CALL!!!\n");
        //print_value(return_val);
        LispContext * new_ctx = new_context_from_args(convert_from_jump_val(return_val), ctx->parent_lambda->value->params, ctx->parent_lambda->ctx, ctx->parent_lambda);
        new_ctx->next = ctx;
        new_ctx->tco_buf = ctx->tco_buf;
        ctx = new_ctx;
        //printf("\n");
    }
    //printf("EVAL SEQ\n");
    //printf("TCO ENV: 0x%x\n", ctx->tco_buf);
    //printf("PARENT LAMBDA: ");
    //print_value_raw(ctx->parent_lambda);
    //printf("\n");
    tail_call_goto:;
    LispCell * current_cell = cell;
    LispValue * last_value = NULL;
    while ( current_cell ) {
        LispLambda * current_cell_head = eval(current_cell->head->value, ctx);
        if ( !current_cell->tail && current_cell_head == ctx->parent_lambda ) {
            //printf("TAIL CALL!\n");
            LispContext * new_ctx = new_context_from_args(eval_args(((LispCell *)current_cell->head)->tail, ctx), ctx->parent_lambda->value->params, ctx->parent_lambda->ctx, ctx->parent_lambda);
            current_cell = cell;
            new_ctx->next = ctx;
            ctx = new_ctx;
            goto tail_call_goto;
        }
        last_value = eval(current_cell->head, ctx);
        current_cell = current_cell->tail;
    }
    return last_value;
}

LispValue * eval(LispValue * value, LispContext * ctx) {
    if (!value)
        return NULL;
    LispContextEntry * found_entry = NULL;
    switch (value->type) {
        case kCellValue:
            return eval_cell(value, ctx);
        case kSymbolValue:
            found_entry = find_context_entry_all(ctx, value->value);
            if ( !found_entry ) {
                print_context(ctx);
                printf("UNFOUND: %s\n", value->value);
                exit_message("Undefined symbol.", -1);
            }
            return found_entry->value;
        default:
            return value;
    }
}
