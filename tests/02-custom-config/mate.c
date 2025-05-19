#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  CreateConfig((MateOptions){.compiler = CLANG, .buildDirectory = "./custom-dir"});

  StartBuild();
  {
    String buildNinjaPath;
    if (isMSVC()) {
      buildNinjaPath = CreateExecutable((ExecutableOptions){.output = "main", .flags = "/W4"});
    } else {
      buildNinjaPath = CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall"});
    }

    AddFile("./src/main.c");
    AddFile("./src/t_math.c");

    if (isLinux()) {
      LinkSystemLibraries("m");
    }

    String exePath = InstallExecutable();
    errno_t errExe = RunCommand(exePath);

    errno_t errCompileCmd = CreateCompileCommands(buildNinjaPath);
    Assert(errExe == SUCCESS && errCompileCmd == SUCCESS, "Failed, RunCommand and errCompileCmd should be SUCCESS");
  }
  EndBuild();
}
