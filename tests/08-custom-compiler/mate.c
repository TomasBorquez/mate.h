#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  CreateConfig((MateOptions){.compiler = "g++", .compilerFamily = GCC});

  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall"});

    AddFile(executable, "./src/main.cpp");
    AddFile(executable, "./src/t_math.cpp");

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
