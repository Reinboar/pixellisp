#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "./constructor.h"
#include "./context.h"

LispValue * convert_to_jump_val(LispValue * val);
LispValue * convert_from_jump_val(LispValue * val);

LispContext * new_context_from_args(LispCell * args, LispCell * params, LispContext * parent_ctx, LispLambda * lam);
LispValue * eval_cell(LispCell * cell, LispContext * ctx);
LispValue * eval_seq(LispCell * cell, LispContext * ctx);
LispValue * eval_lambda(LispLambda * lambda, LispCell * args, LispContext * ctx);
LispValue * eval_args(LispCell * args, LispContext * ctx);
LispValue * eval(LispValue * value, LispContext * ctx);

#endif // INTERPRETER_H
