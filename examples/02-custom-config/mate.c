#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  // You can create a config that allows you to use different compilers as well as build directories
  CreateConfig((MateOptions){
      .scriptCompiler = {.compiler = "clang", .compilerFamily = CLANG},
      .buildDirectory = "./custom-dir",
      .rebuildFlags = "-w", // custom rebuild flags (used when mate rebuilds itself). useful when you want to ignore all warnings from the build script
  });

  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile(executable, "./src/main.c");
    AddFile(executable, "./src/t_math.c");

    // Add Standard C Math Library
    if (GetOS() == OS_LINUX) { // Linux does not have libmath by default
      LinkSystemLibraries(executable, "m");
    }

    InstallExecutable(executable);

    RunCommand(executable.outputPath);
    // Create compile commands for better LSP support
    CreateCompileCommands(executable);
  }
  EndBuild();
}
