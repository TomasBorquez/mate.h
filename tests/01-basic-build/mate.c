#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Target t = HostTarget();

  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "main",
        .flags = !isMSVC(t) ? "-Wall" : "/W4"
    });

    AddFile(executable, "./src/main.c");

    InstallExecutable(executable);

    errno_t errExe = RunCommand(executable.outputPath);
    Assert(errExe == SUCCESS, "Failed, RunCommand should be SUCCESS");
  }
  EndBuild();
}
