#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  errno_t result;

  StartBuild();
  {
    // No matter the compiler (MSVC/GCC/CLANG), this would give "equivalent" flags
    CreateExecutable((ExecutableOptions){
        .output = "main",
        .warnings = FLAG_WARNINGS,         // -Wall -Wextra
        .debug = FLAG_DEBUG,               // -g3
        .optimization = FLAG_OPTIMIZATION, // -O2
        .std = FLAG_STD_C23,               // -std=c2x
    });

    AddFile("./src/main.c");

    LinkSystemLibraries("m");

    String exePath = InstallExecutable();
    result = RunCommand(exePath);
  }
  EndBuild();

  return result;
}
