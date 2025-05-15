#define MATE_IMPLEMENTATION
#include "../../mate.h"

#define SAMU_VERSION_OUTPUT 2

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
    errno_t err = RunCommand(F(state.arena, "%s -version", exePath.data));
    result = err == SAMU_VERSION_OUTPUT ? SUCCESS : 1;
  }
  EndBuild();

  return result;
}
