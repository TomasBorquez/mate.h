#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main(void) {
  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "main",
        .warnings = FLAG_WARNINGS,         // -Wall -Wextra
        .debug = FLAG_DEBUG,               // -g3
        .optimization = FLAG_OPTIMIZATION, // -O2
        .std = FLAG_STD_C23,               // -std=c2x
    });

    AddFile(executable, "./src/main.c");

    if (isLinux()) {
      LinkSystemLibraries(executable, "m");
    }

    InstallExecutable(executable);

    errno_t result = RunCommand(executable.outputPath);
    Assert(result == SUCCESS, "RunCommand: failed should be SUCCESS");
  }
  EndBuild();
}
