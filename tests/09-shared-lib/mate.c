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

    LinkSystemLibraries(exe, "m"); // fabsf()
    LinkSharedLib(exe, mathlib);

    InstallExecutable(exe);
  }
  EndBuild();
}
