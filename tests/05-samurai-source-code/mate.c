#define MATE_IMPLEMENTATION
#include "../../mate.h"

#define SAMU_NO_SUCH_FILE_OR_DIR 256

i32 main() {
  errno_t result;

  StartBuild();
  {
    CreateExecutable((ExecutableOptions){
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
    AddFile("./src/*.c");

    LinkSystemLibraries("rt");

    String exePath = InstallExecutable();
    errno_t err = RunCommand(exePath);
    result = err == SAMU_NO_SUCH_FILE_OR_DIR ? SUCCESS : 1; // NOTE: "samu: open build.ninja: No such file or directory"
  }
  EndBuild();

  return result;
}
