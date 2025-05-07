#define MATE_IMPLEMENTATION // Adds the implementation of functions for mate
#include "../../mate.h"

i32 main() {
  errno_t result = 0;

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

    // Runs `./build/main.exe` or whatever your main file is
    result = RunCommand(exePath);
  }
  EndBuild();

  return result; // returns resul of running executable, 0 is success
}
