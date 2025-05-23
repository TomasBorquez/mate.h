#define MATE_IMPLEMENTATION // Adds the implementation of functions for mate
#include "../../mate.h"

i32 main(void) {
  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "main",   // output name, in windows this becomes `main.exe` and on linux it stays as `main`
        .flags = "-Wall -g" // adds warnings and debug symbols
    });

    // Files to compile
    AddFile(executable, "./src/main.c");

    // Compiles all files parallely
    InstallExecutable(executable);

    // Runs `./build/main`
    errno_t err = RunCommand(executable.outputPath);
    Assert(err == SUCCESS, "RunCommand: failed, return from exe should always be SUCCESS"); // Assert that we return SUCCESS == 0
  }
  EndBuild();
}
