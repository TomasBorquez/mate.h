#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Arena *arena = ArenaCreate(128);
  String s = S("abcdef");
  String sub = StrSlice(arena, s, 2, 5);
  LogInfo("Slice(2,5): %.*s", (int)sub.length, sub.data);
  ArenaFree(arena);
  return 0;
}