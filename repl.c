#include "./tokenizer.h"
#include "./constructor.h"
#include "./context.h"
#include "./primitive.h"
#include "./interpreter.h"
#include <stdio.h>

LispValue * run_file(char * filename, LispContext * ctx) {
  FILE * code_file = fopen(filename, "r");
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

int main (int argc, char ** argv) {
  init_global_symbol_table(200);
  LispContext * ctx = new_context();
  init_primitive_defs(ctx);
  char in_buf[65535];
  while (true) {
    printf("$ ");
    char * console_code = fgets(in_buf, sizeof(in_buf), stdin);
    if ( !console_code ) {
      printf("\n");
      exit(-1);
    }
    LispValue * result = eval_seq(construct_ast(tokenize(console_code), NULL), ctx);
    printf("=> ");
    print_value(result);
    printf("\n\n");
  }
  return 0;
}
