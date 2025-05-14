#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  errno_t result = SUCCESS;

  StartBuild();
  {
    CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile("./src/main.c");

    String exePath = InstallExecutable();

    result = RunCommand(exePath);
  }
  EndBuild();

  return result;
}
