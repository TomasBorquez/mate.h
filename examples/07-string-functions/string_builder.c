#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  // StringBuilder is designed for efficient concatenation of many strings.
  // Unlike StrConcat, which allocates a new buffer for each operation,
  // StringBuilder grows its buffer as needed and appends in-place,
  // minimizing allocations and copying when building up a large string.

  Arena *arena = ArenaCreate(128);
  StringBuilderReserve(arena, 32);

  StringBuilder builder = StringBuilderCreate(arena);
  String hello = S("Hello, ");
  String world = S("world!");
  StringBuilderAppend(arena, &builder, &hello);
  StringBuilderAppend(arena, &builder, &world);
  LogInfo("StringBuilder: %.*s", (int)builder.buffer.length, builder.buffer.data);
  ArenaFree(arena);
  return 0;
}