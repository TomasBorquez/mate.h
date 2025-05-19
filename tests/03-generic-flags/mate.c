#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  StartBuild();
  {
    CreateExecutable((ExecutableOptions){
        .output = "main",
        .warnings = FLAG_WARNINGS,         // -Wall -Wextra
        .debug = FLAG_DEBUG,               // -g3
        .optimization = FLAG_OPTIMIZATION, // -O2
        .std = FLAG_STD_C23,               // -std=c2x
    });

    AddFile("./src/main.c");

    if (isLinux()) {
      LinkSystemLibraries("m");
    }

    String exePath = InstallExecutable();
    errno_t result = RunCommand(exePath);
    Assert(result == SUCCESS, "Failed, RunCommand should be SUCCESS");
  }
  EndBuild();
}
