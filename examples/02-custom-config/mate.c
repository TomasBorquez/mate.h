#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  // You can create a config that allows you to use different compilers as well as build directories
  CreateConfig((MateOptions){.compiler = CLANG, .buildDirectory = "./custom-dir"});

  StartBuild();
  {
    String ninjaPath = CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile("./src/main.c");
    AddFile("./src/t_math.c");

    // Add Standard C Math Library
    if (isLinux()) {
      LinkSystemLibraries("m"); // MSVC/MinGW already have math lib by default
    }

    String exePath = InstallExecutable();
    RunCommand(exePath);

    // Create compile commands for better LSP support
    CreateCompileCommands(ninjaPath); // ninjaPath is the path where the build.ninja is at, here is where we grab all the commands from
  }
  EndBuild();
}
