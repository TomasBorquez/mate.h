#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(int argc, char **argv) {
  char *cwd = GetCwd();

  LogInfo("Before switching:");
  LogInfo("\tCurrent working directory: %s", cwd);

  if (argc > 1) {
    SetCwd(argv[1]);
    cwd = GetCwd();
    LogInfo("After switching:");
    LogInfo("\tCurrent working directory: %s", cwd);
  }

  return 0;
}