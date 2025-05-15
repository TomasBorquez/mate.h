#define MATE_IMPLEMENTATION // Adds the implementation of functions for mate
#include "../../mate.h"

i32 main() {
  StartBuild();
  {
    CreateExecutable((ExecutableOptions){
        .output = "main",   // output name, in windows this becomes `main.exe` and on linux it stays as `main`
        .flags = "-Wall -g" // adds warnings and debug symbols
    });

    // Files to compile
    AddFile("./src/main.c");

    // Compiles all files parallely
    String exePath = InstallExecutable();

    // Runs `./build/main`
    errno_t err = RunCommand(exePath);
    LogInfo("err: %d", err);
  }
  EndBuild();
}
