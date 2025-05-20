#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main(void) {
  CreateConfig((MateOptions){.compiler = CLANG, .buildDirectory = "./custom-dir"});

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

    if (isLinux()) {
      LinkSystemLibraries(executable, "m");
    }

    InstallExecutable(executable);

    errno_t errExe = RunCommand(executable.outputPath);
    errno_t errCompileCmd = CreateCompileCommands(executable);
    Assert(errExe == SUCCESS && errCompileCmd == SUCCESS, "Failed, RunCommand and errCompileCmd should be SUCCESS");
  }
  EndBuild();
}
