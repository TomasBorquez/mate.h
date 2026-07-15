#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Target t = HostTarget();
  CreateConfig((MateOptions){.buildDirectory = "./custom-dir"});

  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "main",
        .flags = !isMSVC(t) ? "-Wall" : "/W4"
    });

    AddFile(executable, "./src/main.c");
    AddFile(executable, "./src/t_math.c");

    if (isLinux(t) || isFreeBSD(t)) {
      LinkSystemLibraries(executable, "m");
    }

    InstallExecutable(executable);

    errno_t errExe = RunCommand(executable.outputPath);
    errno_t errCompileCmd = CreateCompileCommands(executable);
    Assert(errExe == SUCCESS && errCompileCmd == SUCCESS, "Failed, RunCommand and errCompileCmd should be SUCCESS");
  }
  EndBuild();
}
