#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Target t = HostTarget();

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

    if (isLinux(t) || isFreeBSD(t)) {
      LinkSystemLibraries(executable, "m");
    }

    InstallExecutable(executable);

    errno_t errExe = RunCommand(executable.outputPath);
    Assert(errExe == SUCCESS, "RunCommand: failed should be SUCCESS");
  }
  EndBuild();
}
