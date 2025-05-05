#define MATE_IMPLEMENTATION
#include "mate.h"

i32 main() {
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
    RunCommand(exePath);

    // Create compile commands for better LSP support
    CreateCompileCommands();
  }
  EndBuild();
}
