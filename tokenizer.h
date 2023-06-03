#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
  kUnknown,
  kLeftParen,
  kRightParen,
  kLeftBracket,
  kRightBracket,
  kLeftBrace,
  kRightBrace,
  kHash,
  kSingleQuote,
  kPeriod,
  kBacktick,
  kComma,
  kAtSign,
  kIdentifier,
  kString,
  kNumber
} TokenType;

typedef struct {
  char * value;
  TokenType type;
} Token;

typedef struct {
  Token ** tokens;
  Token ** current;
  size_t size;
} TokenList;

TokenList * tokenize(char * code);
void print_token(Token * token);

#endif // TOKENIZER_H
