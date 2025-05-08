#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  errno_t result;

  // You can create a config that allows you to use different compilers as well as build directories
  CreateConfig((MateOptions){.compiler = "clang", .buildDirectory = "./custom-dir"});

  StartBuild();
  {
    CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile("./src/main.c");
    AddFile("./src/t_math.c");

    // Add Standard C Math Library
    LinkSystemLibraries("m");

    String exePath = InstallExecutable();
    errno_t errExe = RunCommand(exePath);

    // Create compile commands for better LSP support
    errno_t errCompileCmd = CreateCompileCommands();

    result = (errExe == SUCCESS && errCompileCmd == SUCCESS) ? SUCCESS : 1; // 0 success, >1 err
  }
  EndBuild();

  return result;
}
