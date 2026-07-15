#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  StartBuild();
  {
    // shared lib:
    SharedLib mathlib = CreateSharedLib((SharedLibOptions){
        .output = "mathlib",
    });

    AddFile(mathlib, "./src/mathlib.c");

    InstallSharedLib(mathlib);

    // executable:
    Executable exe = CreateExecutable((ExecutableOptions){
        .output = "main",
    });

    AddFile(exe, "./src/main.c");

    LinkSharedLib(exe, mathlib);
    LinkSystemLibraries(exe, "m"); // fabsf()

    InstallExecutable(exe);
    errno_t errExe = RunCommand(exe.outputPath);
    Assert(errExe == SUCCESS, "Failed, RunCommand should be SUCCESS");
  }
  EndBuild();
}
