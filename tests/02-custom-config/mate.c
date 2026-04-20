#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  CreateConfig((MateOptions){.buildDirectory = "./custom-dir"});

  StartBuild();
  {
    Executable executable;
    if (isMSVC()) {
      executable = CreateExecutable((ExecutableOptions){.output = "main", .flags = "/W4"});
    } else {
      executable = CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall"});
    }

    AddFile(executable, "./src/main.c");
    AddFile(executable, "./src/t_math.c");

    if (isLinux() || isFreeBSD()) {
      LinkSystemLibraries(executable, "m");
    }

    InstallExecutable(executable);

    errno_t errExe = RunCommand(executable.outputPath);
    errno_t errCompileCmd = CreateCompileCommands(executable);
    Assert(errExe == SUCCESS && errCompileCmd == SUCCESS, "Failed, RunCommand and errCompileCmd should be SUCCESS");
  }
  EndBuild();
}
