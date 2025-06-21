#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  // STRING_LENGTH exists to extract the length out of a #define'd char literal.
  // ENSURE_STRING_LITERAL turns such a literal into a valid char*.

#define MYSTR "abcdef"
  LogInfo("STRING_LENGTH(MYSTR): %zu", STRING_LENGTH(MYSTR));
  LogInfo("ENSURE_STRING_LITERAL(MYSTR): %s", ENSURE_STRING_LITERAL(MYSTR));
  return 0;
}