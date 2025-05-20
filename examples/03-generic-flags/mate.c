#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main(void) {
  StartBuild();
  {
    // No matter the compiler (MSVC/GCC/CLANG), this would give "equivalent" flags
    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "main",
        .warnings = FLAG_WARNINGS,         // -Wall -Wextra
        .debug = FLAG_DEBUG,               // -g3
        .optimization = FLAG_OPTIMIZATION, // -O2
        .std = FLAG_STD_C23,               // -std=c2x
    });

    AddFile(executable, "./src/main.c");

    if (isLinux()) {
      LinkSystemLibraries(executable, "m"); // Add math only if on linux since MSVC includes this on STD
    }

    InstallExecutable(executable);
    RunCommand(executable.outputPath);
  }
  EndBuild();
}
