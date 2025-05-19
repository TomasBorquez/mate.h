#define MATE_IMPLEMENTATION
#include "../../mate.h"

#define SAMU_VERSION_OUTPUT 2

i32 main() {
  StartBuild();
  {
    CreateExecutable((ExecutableOptions){
        .output = "samu",
        .optimization = FLAG_OPTIMIZATION,
        .std = FLAG_STD_C99,
        .warnings = FLAG_WARNINGS_NONE,
    });

    AddFile("./src/*.c");
    RemoveFile("./src/exclude-me.c");
    RemoveFile("./src/random.c");

    LinkSystemLibraries("rt");

    String exePath = InstallExecutable();
    errno_t err = RunCommand(F(mateState.arena, "%s -version", exePath.data));
    Assert(err == SAMU_VERSION_OUTPUT, "Failed, output did not equal SAMU_VERSION_OUTPUT");
  }
  EndBuild();
}
