#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  String str = S("   hello world   ");
  LogInfo("Before trim: \"%s\"", str.data);
  StrTrim(&str);
  LogInfo("After trim: \"%s\"", str.data);
  return 0;
}