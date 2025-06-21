#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  String src = S("copy me");

  char buf[32] = {0};

  String dest = {.length = sizeof(buf) - 1, .data = buf};
  StrCopy(dest, src);

  LogInfo("Copied: %.*s", (int)dest.length, dest.data);
  return 0;
}