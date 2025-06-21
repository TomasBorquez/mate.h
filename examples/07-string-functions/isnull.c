#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  String a = {0};
  String b = S("not null");
  LogInfo("StrIsNull(a): %s", StrIsNull(a) ? "true" : "false");
  LogInfo("StrIsNull(b): %s", StrIsNull(b) ? "true" : "false");
  return 0;
}