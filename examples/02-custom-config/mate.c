#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main(void) {
  // You can create a config that allows you to use different compilers as well as build directories
  CreateConfig((MateOptions){.compiler = CLANG, .buildDirectory = "./custom-dir"});

  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile(executable, "./src/main.c");
    AddFile(executable, "./src/t_math.c");

    // Add Standard C Math Library
    if (isLinux()) {
      LinkSystemLibraries(executable, "m"); // MSVC/MinGW already have math lib by default
    }

    InstallExecutable(executable);

    RunCommand(executable.outputPath);
    // Create compile commands for better LSP support
    CreateCompileCommands(executable.ninjaBuildPath); // executable.ninjaBuildPath is the path where the build.ninja is at, here is where we grab all the commands from
  }
  EndBuild();
}
