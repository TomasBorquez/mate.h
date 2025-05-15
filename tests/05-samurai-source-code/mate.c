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

    AddFile("./src/*.c");
    RemoveFile("./src/exclude-me.c");
    RemoveFile("./src/random.c");

    LinkSystemLibraries("rt");

    String exePath = InstallExecutable();
    errno_t err = RunCommand(exePath);
    result = err == SAMU_NO_SUCH_FILE_OR_DIR ? SUCCESS : 1; // NOTE: "samu: open build.ninja: No such file or directory"
  }
  EndBuild();

  return result;
}
