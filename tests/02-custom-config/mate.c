#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  errno_t result;

  CreateConfig((MateOptions){.compiler = "gcc", .buildDirectory = "./custom-dir"});

  StartBuild();
  {
    String buildNinjaPath = CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile("./src/main.c");
    AddFile("./src/t_math.c");

    if (isLinux()) {
      LinkSystemLibraries("m");
    }

    String exePath = InstallExecutable();
    errno_t errExe = RunCommand(exePath);

    errno_t errCompileCmd = CreateCompileCommands(buildNinjaPath);
    result = (errExe == SUCCESS && errCompileCmd == SUCCESS) ? SUCCESS : 1;
  }
  EndBuild();

  return result;
}
