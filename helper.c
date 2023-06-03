#include "./helper.h"

void exit_message(char * msg, int code) {
  printf("ERROR: %s\n", msg);
  exit(code);
}
