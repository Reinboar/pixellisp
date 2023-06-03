#include "./helper.h"
#include "./tokenizer.h"
#include "./symbols.h"
#include "./constructor.h"

void init_global_symbol_table(size_t size) {
    GLOBAL_SYM_TABLE = new_symbol_table(size);
}

LispValue * new_lisp_value(void * value) {
  LispValue * lisp_val = malloc(sizeof(LispValue));
  if ( !lisp_val ) {
    exit_message("Error while allocating new lisp value.", -1);
  }
  lisp_val->value = value;
  lisp_val->extra_value = NULL;
  lisp_val->type = kUnknownValue;
  lisp_val->refs = 0;
  return lisp_val;
}

#define LispType(name, val_type_enum, return_type, val_type) \
  return_type name (val_type value) { \
    return_type lisp_value = new_lisp_value(value); \
    lisp_value->type = val_type_enum; \
    return lisp_value; \
  }

LispType(new_lisp_number, kNumberValue, LispNumber *, int)
LispType(new_lisp_string, kStringValue, LispString *, char *)
LispType(new_lisp_symbol, kSymbolValue, LispSymbol *, char *)
LispType(new_lisp_primitive, kPrimitiveValue, LispPrimitive *, PrimitiveFunPtr)

LispBool * new_lisp_bool(bool value) {
  LispBool * lisp_value = new_lisp_value((void*)value);
  lisp_value->type = kBoolValue;
  return lisp_value;
}

LambdaInfo * new_lambda_info(LispCell * code, LispCell * params) {
  LambdaInfo * lambda_info = calloc(sizeof(LambdaInfo), 1);
  lambda_info->code = code;
  lambda_info->params = params;
  return lambda_info;
}

LispLambda * new_lisp_lambda(LispCell * code, LispCell * params, struct LispContext * ctx) {
  LispLambda * lisp_lam = new_lisp_value(new_lambda_info(code, params));
  lisp_lam->type = kLambdaValue;
  lisp_lam->ctx = ctx;
  return lisp_lam;
}

MacroInfo * new_macro_info(LispCell * template, LispCell * params) {
  MacroInfo * macro_info = calloc(sizeof(MacroInfo), 1);
  macro_info->template = template;
  macro_info->params = params;
  return macro_info;
}

LispMacro * new_lisp_macro(LispCell * template, LispCell * params, struct LispContext * ctx) {
  LispMacro * lisp_mac = new_lisp_value(new_macro_info(template, params));
  lisp_mac->type = kMacroValue;
  lisp_mac->ctx = ctx;
  return lisp_mac;
}

LispCell * new_lisp_cell(LispValue * head, LispValue * tail) {
  LispCell * lisp_cell = new_lisp_value(head);
  lisp_cell->type = kCellValue;
  lisp_cell->tail = tail;
  return lisp_cell;
}

LispVector * new_lisp_vector(size_t length) {
  LispVector * lisp_vec = new_lisp_value(NULL);
  lisp_vec->value = calloc(sizeof(LispValue), length);
  lisp_vec->length = length;
  lisp_vec->type = kVectorValue;
  return lisp_vec;
}

// Returns true if the token is a left paren, bracket, or brace.
bool is_opener(Token * token) {
  switch (token->type) {
    case kLeftParen:
    case kLeftBracket:
    case kLeftBrace:
    return true;
    default:
    return false;
  }
}

// Returns true if the token is a right paren, bracket, or brace.
bool is_closer(Token * token) {
  switch (token->type) {
    case kRightParen:
    case kRightBracket:
    case kRightBrace:
    return true;
    default:
    return false;
  }
}

// Returns true if token is not cell-based or otherwise a collection.
bool is_atomic_token(Token * token) {
  return token->type == kNumber || token->type == kString || token->type == kIdentifier;
}

bool is_modifier_token(Token * token) {
  return token->type == kSingleQuote ||
    token->type == kBacktick ||
    token->type == kComma ||
    token->type == kHash ||
    token->type == kAtSign;
}

// Sets the passed cell's head to the given head value, then returns a new cell
// after setting the old cell's tail to it.
LispCell * extend_cell(LispCell * current_cell, LispValue * head) {
  current_cell->head = head;
  LispCell * new_cell = new_lisp_cell(NULL, NULL);
  current_cell->tail = new_cell;
  return new_cell;
}

bool boolify_value(LispValue * value) {
  if ( !value )
    return false;
  if ( value->type == kBoolValue && value->value == false )
    return false;
  return true;
}

LispValue * token_to_value(Token * token) {
  switch (token->type) {
    case kNumber:
    return new_lisp_number(atoi(token->value));
    case kString:
    return new_lisp_string(token->value);
    case kIdentifier:
    return new_lisp_symbol(insert_symbol_if_not_found(GLOBAL_SYM_TABLE, token->value)->name);
    default:
    printf("ERROR: Unknown token with type %d and value '%s'\n", token->type, token->value);
    exit(-1);
  }
}

size_t cells_length(LispCell * cells) {
  LispCell * current_cell = cells;
  size_t current_length = 0;
  while ( current_cell ) {
    current_length++;
    current_cell = current_cell->tail;
  }
  return current_length;
}

LispValue ** cells_to_array(LispCell * cells) {
  size_t array_len = cells_length(cells);
  LispValue ** val_array = calloc(sizeof(LispValue*), array_len);
  LispCell * current_cell = cells;
  for ( unsigned int i = 0 ; current_cell ; i++ ) {
    val_array[i] = current_cell->head;
    current_cell = current_cell->tail;
  }
  return val_array;
}

LispCell * construct_list(TokenList * token_list, Token *** current_token);

LispValue const * END_OF_LIST = -1;

LispCell * quoteify(LispValue * value) {
  LispSymbol * quote_symbol = new_lisp_symbol(find_symbol(GLOBAL_SYM_TABLE, "quote")->name);
  return new_lisp_cell(quote_symbol, new_lisp_cell(value, NULL));
}

LispCell * quasiquoteify(LispValue * value) {
  LispSymbol * qquote_symbol = new_lisp_symbol(find_symbol(GLOBAL_SYM_TABLE, "quasiquote")->name);
  return new_lisp_cell(qquote_symbol, new_lisp_cell(value, NULL));
}

LispCell * unquoteify(LispValue * value) {
  LispSymbol * unquote_symbol = new_lisp_symbol(insert_symbol_if_not_found(GLOBAL_SYM_TABLE, "unquote")->name);
  return new_lisp_cell(unquote_symbol, new_lisp_cell(value, NULL));
}

LispCell * flatten_unquoteify(LispValue * value ) {
  LispSymbol * flatten_symbol = new_lisp_symbol(insert_symbol_if_not_found(GLOBAL_SYM_TABLE, "unquote-flatten")->name);
  return new_lisp_cell(flatten_symbol, new_lisp_cell(value, NULL));
}

LispValue * construct_next_token(TokenList * token_list, Token *** current_token);

LispValue * construct_modifier_token(TokenList * token_list, Token *** current_token) {
  if ( (**current_token)->type == kSingleQuote ) {
    (*current_token)++;
    LispValue * next_value = construct_next_token(token_list, current_token);
    if ( next_value == END_OF_LIST )
      exit_message("Expecting value following quote.", -1);
    return quoteify(next_value);
  } else if ( (**current_token)->type == kBacktick ) {
    (*current_token)++;
    LispValue * next_value = construct_next_token(token_list, current_token);
    if ( next_value == END_OF_LIST )
      exit_message("Expecting value following quasiquote.", -1);
    return quasiquoteify(next_value);
  } else if ( (**current_token)->type == kComma ) {
    (*current_token)++;
    if ( (**current_token)->type == kAtSign ) {
      (*current_token)++;
      LispValue * next_value = construct_next_token(token_list, current_token);
      if ( next_value == END_OF_LIST )
        exit_message("Expecting value following unquote-flatten.", -1);
      return flatten_unquoteify(next_value);
    }
    LispValue * next_value = construct_next_token(token_list, current_token);
    if ( next_value == END_OF_LIST )
      exit_message("Expecting value following unquote.", -1);
    return unquoteify(next_value);
  } else if ( (**current_token)->type == kHash ) {
    exit_message("Vector literals are unimplemented.", -1);
  }
}

LispValue * construct_next_token(TokenList * token_list, Token *** current_token) {
  if ( is_atomic_token(**current_token) ) {
    (*current_token)++;
    return token_to_value(*((*current_token)-1));
  } else if ( is_modifier_token(**current_token)) {
    return construct_modifier_token(token_list, current_token);
  } else if ( is_opener(**current_token) ) {
    (*current_token)++;
    return construct_list(token_list, current_token);
  } else if ( is_closer(**current_token) ) {
    (*current_token)++;
    return END_OF_LIST;
  } else {
    printf("ERROR: ");
    print_token(**current_token);
    return NULL;
  }
}

LispCell * construct_list(TokenList * token_list, Token *** current_token) {
  LispCell * list_root = new_lisp_cell(NULL, NULL);
  LispCell * current_cell = list_root;
  LispCell * last_cell = NULL;
  if ( (**current_token)->type == kPeriod )
    exit_message("Dotted list must have car value.", -1);
  LispValue * next_value = construct_next_token(token_list, current_token);
  while ( next_value != END_OF_LIST ) {
    if ( (**current_token)->type == kPeriod ) {
      (*current_token)++;
      LispValue * tail_value = construct_next_token(token_list, current_token);
      current_cell->head = next_value;
      current_cell->tail = tail_value;
      if ( construct_next_token(token_list, current_token) != END_OF_LIST )
        exit_message("Dotted list can't have more than one cdr value.", -1);
      return list_root;
    }
    last_cell = current_cell;
    current_cell = extend_cell(current_cell, next_value);
    next_value = construct_next_token(token_list, current_token);
  }
  if ( last_cell )
    last_cell->tail = NULL;
  return list_root;
}

LispCell * construct_ast(TokenList * token_list, Token *** current_token) {
  LispCell * root_cell = new_lisp_cell(NULL, NULL);
  LispCell * current_cell = root_cell;
  LispCell * last_cell = NULL;
  Token ** stack_token_ptr = token_list->tokens;
  if (!current_token) // use stack allocated token ptr if none is passed
    current_token = &stack_token_ptr;
  while (*current_token < (token_list->current) ) {
    LispValue * next_value = construct_next_token(token_list, current_token);
    last_cell = current_cell;
    current_cell = extend_cell(current_cell, next_value);
  }
  if ( last_cell )
    last_cell->tail = NULL;
  return root_cell;
}

void print_value(LispValue * value) {
  if (!value) {
    printf("() ");
    return;
  }
  switch(value->type) {
    case kNumberValue:
    printf("%d", value->value);
    break;
    case kStringValue:
    printf("%s", value->value);
    break;
    case kSymbolValue:
    printf("%s", value->value);
    break;
    case kLambdaValue:
    printf("<LAMBDA 0x%x>", value->value);
    break;
    case kPrimitiveValue:
    printf("<PRIMITIVE 0x%x>", value->value);
    break;
    case kBoolValue:
    value->value ? printf("true") : printf("false");
    break;
    default:
    case kUnknownValue:
    printf("<UNKNOWN type=%d>", value->type);
    break;
    case kCellValue:
    print_cell(value);
    break;
  }
  printf(" ");
}

void print_value_raw(LispValue * value) {
  if (!value) {
    printf("NULL");
    return;
  }
  printf("<");
  if ( value->type == kCellValue ) {
    print_cell_raw(value);
  } else {
    print_value(value);
  }
  printf(" | type=%d addr=0x%x>", value->type, value);
}

void print_cell(LispCell * list) {
  if (!list || list->type != kCellValue)
    return;
  LispCell * current_cell = list;
  printf("(");
  while ( current_cell ) {
    print_value(current_cell->head);
    current_cell = current_cell->tail;
    if ( current_cell && current_cell->type != kCellValue ) {
      printf(". ");
      print_value(current_cell);
      break;
    }
  }
  printf(")");
}

void print_cell_raw(LispCell * list) {
  if (!list || list->type != kCellValue)
    return;
  LispCell * current_cell = list;
  while ( current_cell ) {
    printf("(");
    print_value_raw(current_cell->head);
    printf(") -> ");
    current_cell = current_cell->tail;
  }
  printf("NULL");
}
