#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "samu",
        .optimization = FLAG_OPTIMIZATION,
        .std = FLAG_STD_C99,
    });

    /* You can do this
      AddFile(executable, "./src/build.c");
      AddFile(executable, "./src/deps.c");
      AddFile(executable, "./src/env.c");
      AddFile(executable, "./src/graph.c");
      ...
    */

    // Or use this wildcard
    AddFile(executable, "./src/*.c");

    if (isLinux()) {
      LinkSystemLibraries(executable, "rt");
    }

    InstallExecutable(executable);
    RunCommand(executable.outputPath);
  }
  EndBuild();
}
