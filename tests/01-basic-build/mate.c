#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  StartBuild();
  {
    CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile("./src/main.c");

    String exePath = InstallExecutable();

    errno_t result = RunCommand(exePath);
    Assert(result == SUCCESS, "Failed, RunCommand should be SUCCESS");
  }
  EndBuild();
}
