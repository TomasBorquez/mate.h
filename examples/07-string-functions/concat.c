#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Arena *arena = ArenaCreate(128);
  String a = S("foo");
  String b = S("bar");
  String c = StrConcat(arena, a, b);
  LogInfo("StrConcat: %.*s", (int)c.length, c.data);
  ArenaFree(arena);
  return 0;
}