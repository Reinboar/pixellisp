#ifndef CONSTRUCTOR_H
#define CONSTRUCTOR_H

#include "./tokenizer.h"
#include "./symbols.h"

typedef enum ValueType {
  kUnknownValue,
  kNumberValue,
  kStringValue,
  kSymbolValue,
  kCellValue,
  kLambdaValue,
  kPrimitiveValue,
  kMacroValue,
  kBoolValue,
  kVectorValue
} ValueType;

typedef struct LispValue {
  void * value;
  void * extra_value;
  ValueType type;
  unsigned int refs;
} LispValue;

#define LispTypeStruct(name, value_type, value_name, extra_type, extra_name) \
  typedef struct name { \
    value_type value_name; \
    extra_type extra_name; \
    ValueType type; \
    unsigned int refs; \
  } name ;

LispTypeStruct(LispCell, LispValue *, head, LispValue *, tail)
LispTypeStruct(LispNumber, int, value, void *, unused)
LispTypeStruct(LispString, char *, value, void *, unused)
LispTypeStruct(LispSymbol, char *, value, void *, unused)
LispTypeStruct(LispBool, bool, value, void *, unused)
LispTypeStruct(LispVector, LispValue *, value, size_t, length)

struct LispContext;

typedef LispValue *(*PrimitiveFunPtr)(LispCell *, struct LispContext *);
LispTypeStruct(LispPrimitive, PrimitiveFunPtr, value, void *, unused);

LispCell * new_lisp_cell(LispValue * head, LispValue * tail);
LispNumber * new_lisp_number(int value);
LispString * new_lisp_string(char * value);
LispSymbol * new_lisp_symbol(char * value);
LispPrimitive * new_lisp_primitive(PrimitiveFunPtr value);
LispBool * new_lisp_bool(bool value);
LispVector * new_lisp_vector(size_t length);

typedef struct LambdaInfo {
  LispCell * code;
  LispCell * params;
} LambdaInfo;

struct LispContext;

LispTypeStruct(LispLambda, LambdaInfo *, value, struct LispContext *, ctx)
LispLambda * new_lisp_lambda(LispCell * code, LispCell * params, struct LispContext * ctx);

typedef struct MacroInfo {
  LispCell * template;
  LispCell * params;
} MacroInfo;

LispTypeStruct(LispMacro, MacroInfo *, value, struct LispContext *, ctx)
LispMacro * new_lisp_macro(LispCell * template, LispCell * params, struct LispContext * ctx);

void print_value(LispValue * value);
void print_value_raw(LispValue * value);
void print_cell(LispCell * list);
void print_cell_raw(LispCell * list);
LispCell * construct_ast(TokenList * token_list, Token *** current_token);
LispCell * extend_cell(LispCell * cell, LispValue * head);
bool boolify_value(LispValue * value);
size_t cells_length(LispCell * cells);
LispValue ** cells_to_array(LispCell * cells);

SymbolTable * GLOBAL_SYM_TABLE;
void init_global_symbol_table(size_t size);

#endif // CONSTRUCTOR_H
