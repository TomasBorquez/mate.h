#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Target t = HostTarget();

  StartBuild();
  {
    // shared lib:
    SharedLib mathlib = CreateSharedLib((SharedLibOptions){
        .output = "mathlib",
        .flags = "-DMATHLIB_BUILD",
        .debug = FLAG_DEBUG,
    });

    AddFile(mathlib, "./src/mathlib.c");

    InstallSharedLib(mathlib);

    // executable:
    Executable exe = CreateExecutable((ExecutableOptions){
        .output = "main",
        .debug = FLAG_DEBUG,
    });

    AddFile(exe, "./src/main.c");

    LinkSharedLib(exe, mathlib);
    if (!isWindows(t)) {
      LinkSystemLibraries(exe, "m"); // fabsf()
    }

    InstallExecutable(exe);

    if (isMSVC(t)) {
      Assert(FileStats(S("./build/mathlib.pdb")).error == SUCCESS, "expected mathlib.pdb next to the DLL, /DEBUG pipeline broken");
      Assert(FileStats(S("./build/main.pdb")).error == SUCCESS, "expected main.pdb next to the exe, /DEBUG pipeline broken");
    }

    errno_t errExe = RunCommand(exe.outputPath);
    Assert(errExe == SUCCESS, "Failed, RunCommand should be SUCCESS");
  }
  EndBuild();
}
