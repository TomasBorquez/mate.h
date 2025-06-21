#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  String s1 = S("Hello World!");
  String s2 = S("Hello World!");

  LogInfo("Original: %.*s", (int)s1.length, s1.data);
  StrToLower(s1);
  LogInfo("Lower:    %.*s", (int)s1.length, s1.data);
  StrToUpper(s2);
  LogInfo("Upper:    %.*s", (int)s2.length, s2.data);
  return 0;
}