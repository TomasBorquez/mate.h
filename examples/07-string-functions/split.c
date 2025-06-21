#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Arena *arena = ArenaCreate(128);
  String s = S("a,b,c");
  String delim = S(",");
  LogInfo("s = %.*s", (int)s.length, s.data);

  StringVector v = StrSplit(arena, s, delim);
  for (size_t i = 0; i < v.length; ++i) {
    LogInfo("Split[%zu]: %.*s", i, (int)v.data[i].length, v.data[i].data);
  }

  String s2 = S("line1\nline2\nline3");
  LogInfo("s2 = %.*s", (int)s2.length, s2.data);
  StringVector v2 = StrSplitNewLine(arena, s2);
  for (size_t i = 0; i < v2.length; ++i) {
    LogInfo("SplitNewLine[%zu]: %.*s", i, (int)v2.data[i].length, v2.data[i].data);
  }
  ArenaFree(arena);
  return 0;
}