#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main(void) {
  StartBuild();
  {
    Executable executable;
    if (isMSVC()) {
      executable = CreateExecutable((ExecutableOptions){.output = "main", .flags = "/W4"});
    } else {
      executable = CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall"});
    }

    AddFile(executable, "./src/main.c");

    InstallExecutable(executable);

    errno_t errExe = RunCommand(executable.outputPath);
    Assert(errExe == SUCCESS, "Failed, RunCommand should be SUCCESS");
  }
  EndBuild();
}
