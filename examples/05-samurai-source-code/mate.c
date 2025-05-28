#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main(void) {
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
      AddFile(executable, "./src/htab.c");
      AddFile(executable, "./src/log.c");
      AddFile(executable, "./src/parse.c");
      AddFile(executable, "./src/samu.c");
      AddFile(executable, "./src/scan.c");
      AddFile(executable, "./src/tool.c");
      AddFile(executable, "./src/tree.c");
      AddFile(executable, "./src/util.c");
      AddFile(executable, "./src/os-posix.c");
    */

    // Or this
    AddFile(executable, "./src/*.c");

    if (isLinux()) {
      LinkSystemLibraries(executable, "rt");
    }

    InstallExecutable(executable);
    RunCommand(executable.outputPath);
  }
  EndBuild();
}
