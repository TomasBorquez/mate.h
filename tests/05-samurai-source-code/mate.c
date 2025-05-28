#define MATE_IMPLEMENTATION
#include "../../mate.h"

#define SAMU_VERSION_OUTPUT 2

i32 main(void) {
  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "samu",
        .optimization = FLAG_OPTIMIZATION,
        .std = FLAG_STD_C99,
        .warnings = FLAG_WARNINGS_NONE,
    });

    AddFile(executable, "./src/*.c");
    RemoveFile(executable, "./src/exclude-me.c");
    RemoveFile(executable, "./src/random.c");

    if (!isMacOs()) {
      LinkSystemLibraries(executable, "rt");
    }

    InstallExecutable(executable);

    errno_t err = RunCommand(F(mateState.arena, "%s -version", executable.outputPath.data));
    Assert(err == SAMU_VERSION_OUTPUT, "RunCommand: failed output did not equal SAMU_VERSION_OUTPUT");
  }
  EndBuild();
}
