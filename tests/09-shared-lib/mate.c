#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Target t = HostTarget();

  StartBuild();
  {
    // shared lib linked by another shared lib:
    SharedLib vecutil = CreateSharedLib((SharedLibOptions){
        .output = "vecutil",
        .flags = "-DVECUTIL_BUILD",
        .debug = FLAG_DEBUG,
    });

    AddFile(vecutil, "./src/vecutil.c");

    InstallSharedLib(vecutil);

    // shared lib:
    SharedLib mathlib = CreateSharedLib((SharedLibOptions){
        .output = "mathlib",
        .flags = "-DMATHLIB_BUILD",
        .debug = FLAG_DEBUG,
    });

    AddFile(mathlib, "./src/mathlib.c");

    LinkSharedLib(mathlib, vecutil);

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
      String mathlibPdb = PathJoin(mathlib.outputDir, S("mathlib.pdb"));
      String mainPdb = PathJoin(exe.outputDir, S("main.pdb"));
      Assert(FileStats(mathlibPdb).error == SUCCESS, "expected mathlib.pdb next to the DLL at %s, /DEBUG pipeline broken", mathlibPdb.data);
      Assert(FileStats(mainPdb).error == SUCCESS, "expected main.pdb next to the exe at %s, /DEBUG pipeline broken", mainPdb.data);
    }

    errno_t errExe = RunCommand(exe.outputPath);
    Assert(errExe == SUCCESS, "Failed, RunCommand should be SUCCESS");
  }
  EndBuild();
}
