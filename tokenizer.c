#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "./tokenizer.h"
#include "./helper.h"

Token * new_token(char * value, TokenType type) {
  Token * new_token = malloc(sizeof(Token));
  if ( !new_token )
    exit_message("Error while allocating memory for new token.", -1);
  new_token->value = malloc(strlen(value) + 1);
  strcpy(new_token->value, value);
  new_token->type = type;
  return new_token;
}

TokenList * new_token_list(size_t init_size) {
  TokenList * new_list = malloc(sizeof(TokenList));
  new_list->tokens = malloc(sizeof(Token *) * init_size);
  new_list->current = new_list->tokens;
  new_list->size = init_size;
  return new_list;
}

TokenList * adjust_token_list(TokenList * list) {
  if ( list->current - list->tokens >= list->size ) {
    size_t current_position = list->current - list->tokens;
    list->tokens = realloc(list->tokens, sizeof(Token *) * (list->size + 256));
    if ( !list->tokens )
      exit_message("Error encountered while expanding token list.", -1);
    list->size += 256;
    list->current = list->tokens + current_position; // adjust current pointer in case list pointer changes
  }
  return list;
}

TokenList * add_token(TokenList * list, Token * token) {
  list = adjust_token_list(list);
  (*list->current) = token;
  list->current++;
  return list;
}

void print_token(Token * token) {
  switch(token->type) {
    case kNumber:
    printf("NUMBER: %s\n", token->value);
    break;
    case kString:
    printf("STRING: %s\n", token->value);
    break;
    case kIdentifier:
    printf("IDENTIFIER: %s\n", token->value);
    break;
    case kLeftParen:
    printf("LEFT PAREN\n");
    break;
    case kLeftBracket:
    printf("LEFT BRACKET\n");
    break;
    case kLeftBrace:
    printf("LEFT BRACE\n");
    break;
    case kHash:
    printf("HASH\n");
    break;
    case kSingleQuote:
    printf("SINGLE QUOTE\n");
    break;
    case kBacktick:
    printf("BACKTICK\n");
    break;
    case kComma:
    printf("COMMA\n");
    break;
    case kAtSign:
    printf("AT SIGN\n");
    break;
    case kPeriod:
    printf("PERIOD\n");
    break;
    case kRightParen:
    printf("RIGHT PAREN\n");
    break;
    case kRightBracket:
    printf("RIGHT BRACKET\n");
    break;
    case kRightBrace:
    printf("RIGHT BRACE\n");
    break;
    case kUnknown:
    printf("UNKNOWN\n");
    break;
    default:
    exit_message("Unknown token type", -1);
    break;
  }
}

bool is_alpha(char c) {
  if ( c >= 'A' && c <= 'Z' )
    return true;
  if ( c >= 'a' && c <= 'z' )
    return true;
  return false;
}

bool is_numeric(char c) {
  if ( c >= '0' && c <= '9' )
    return true;
  return false;
}

bool is_special(char c) {
  switch (c) {
    case '(':
    case '[':
    case '{':
    case ')':
    case ']':
    case '}':
    case '"':
    case ' ':
    case '#':
    case '.':
    case '`':
    case ',':
    case '@':
    case '\'':
    case '\n':
    case '\t':
      return true;
    default:
      return false;
  }
}

bool is_number_token(const char * token_val) {
  const size_t token_size = strlen(token_val);
  if (strspn(token_val, "0123456789") == token_size)
    return true;
  return false;
}

TokenType identify_non_special(const char * token_val) {
  if (is_number_token(token_val)) {
    return kNumber;
  } else {
    return kIdentifier;
  }
}

TokenList * tokenize_non_special(char ** cur_char, TokenList * token_list) {
  char * token_val = NULL;
  const char * begin_char = *cur_char;
  if (is_special(**cur_char))
    exit_message("Attempt to tokenize non special token with opening special character.", -1);
  while (!is_special(**cur_char)) {
    if (**cur_char == '\0')
      break;
    (*cur_char)++;
  }
  const size_t token_size = *cur_char - begin_char;
  token_val = malloc(token_size + 1);
  if ( !token_val ) {
    exit_message("Error while allocating memory for non-special token.", -1);
  }
  strncpy(token_val, begin_char, token_size);
  token_val[token_size] = 0;
  token_list = add_token(token_list, new_token(token_val, identify_non_special(token_val)));
  return token_list;
}

TokenList * tokenize_string(char ** cur_char, TokenList * token_list) {
  char * string_val = NULL;
  const char * begin_char = *cur_char;
  if (**cur_char != '"')
    exit_message("Attempt to tokenize string without opening quote.", -1);
  (*cur_char)++;
  while (**cur_char != '"') {
    if (**cur_char == '\0')
      exit_message("Reached end of code while tokenizing string.", -1);
    (*cur_char)++;
  }
  const size_t string_size = *cur_char - begin_char - 1;
  string_val = malloc(string_size + 1);
  if ( !string_val )
    exit_message("Error while allocating memory for string token.", -1);
  strncpy(string_val, begin_char + 1, string_size);
  string_val[string_size] = 0;
  token_list = add_token(token_list, new_token(string_val, kString));
  (*cur_char)++;
  return token_list;
}

TokenList * tokenize(char * code) {
  TokenList * token_list = new_token_list(256);
  char * cur_char = code;
  while (*cur_char) {
    switch (*cur_char) {
      case ' ':
      case '\n':
      case '\t':
      case '\r':
        cur_char++;
        break;
      case '(':
        add_token(token_list, new_token("(", kLeftParen));
        cur_char++;
        break;
      case '[':
        add_token(token_list, new_token("[", kLeftBracket));
        cur_char++;
        break;
      case '{':
        add_token(token_list, new_token("{", kLeftBrace));
        cur_char++;
        break;
      case ')':
        add_token(token_list, new_token(")", kRightParen));
        cur_char++;
        break;
      case ']':
        add_token(token_list, new_token("]", kRightBracket));
        cur_char++;
        break;
      case '}':
        add_token(token_list, new_token("}", kRightBrace));
        cur_char++;
        break;
      case '#':
        add_token(token_list, new_token("#", kHash));
        cur_char++;
        break;
      case '\'':
        add_token(token_list, new_token("'", kSingleQuote));
        cur_char++;
        break;
      case '`':
        add_token(token_list, new_token("`", kBacktick));
        cur_char++;
        break;
      case ',':
        add_token(token_list, new_token("`", kComma));
        cur_char++;
        break;
      case '.':
        add_token(token_list, new_token(".", kPeriod));
        cur_char++;
        break;
      case '@':
        add_token(token_list, new_token("@", kAtSign));
        cur_char++;
        break;
      case ';':
        while ( *cur_char != '\n' )
          cur_char++;
        break;
      case '"':
        token_list = tokenize_string(&cur_char, token_list);
        break;
      default:
        token_list = tokenize_non_special(&cur_char, token_list);
        break;
    }
  }
  return token_list;
}
