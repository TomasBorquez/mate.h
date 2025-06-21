#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  String a = S("hello");
  String b = S("hello ");
  String c = S("world");

  LogInfo("a = \"%s\"", a.data);
  LogInfo("b = \"%s\"", b.data);
  LogInfo("c = \"%s\"", c.data);
  
  LogInfo("StrEq(a, b): %s", StrEq(a, b) ? "true" : "false");
  LogInfo("StrEq(a, c): %s", StrEq(a, c) ? "true" : "false");
  return 0;
}