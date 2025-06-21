#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(int argc, char **argv) {
  // The S macro is for when you have a string literal.

  String a = S("Hello, world!");
  LogInfo("a = %s", a.data);

  // If you have an already existing string, S won't work, because S is
  // a macro. To solve this, you need the `s` function. For this, we're
  // going to showcase this by printing all arguments:

  String programName = s(*argv);
  LogInfo("The program name is: %s", programName.data);
  argv++;
  argc--;

  int argIndex = 1;
  while (argc-- > 0) {
    String arg = s(*argv++);
    LogInfo("argv[%d] = %s", argIndex++, arg.data);
  }

  // StrNewSize is like s(), except it uses an arena.
  Arena *arena = ArenaCreate(128);
  char *hello = "Hello, mate.h!";
  String s = StrNewSize(arena, hello, strlen(hello));
  LogInfo("StrNew s: %.*s", (int)s.length, s.data);
}