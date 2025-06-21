#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(int argc, char **argv) {
  Arena *arena = ArenaCreate(1024);

  String s = F(arena, "The answer is %d and pi is %.2f", 42, 3.14159);
  LogInfo("F: %.*s", (int)s.length, s.data);

  ArenaFree(arena);
}