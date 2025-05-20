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
      AddFile("./src/build.c");
      AddFile("./src/deps.c");
      AddFile("./src/env.c");
      AddFile("./src/graph.c");
      AddFile("./src/htab.c");
      AddFile("./src/log.c");
      AddFile("./src/parse.c");
      AddFile("./src/samu.c");
      AddFile("./src/scan.c");
      AddFile("./src/tool.c");
      AddFile("./src/tree.c");
      AddFile("./src/util.c");
      AddFile("./src/os-posix.c");
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
