#define MATE_IMPLEMENTATION
#include "../../mate.h"

static Target GetHostCrossTarget(void) {
  Target host = HostTarget();

  if (host.compilerFamily == COMPILER_CLANG) {
    return (Target){
        .os = OS_WINDOWS,
        .arch = ARCH_X64,
        .compiler = host.compiler,
        .compilerFamily = COMPILER_CLANG,
    };
  }

  return (Target){
      .os = OS_WINDOWS,
      .arch = ARCH_X64,
      .compiler = "x86_64-w64-mingw32-gcc",
      .compilerFamily = COMPILER_GCC,
  };
}

int main(void) {
  Target t = GetHostCrossTarget();
  bool is_clang = t.compilerFamily == COMPILER_CLANG;

  StartBuild();
  {
    StaticLib mathLib = CreateStaticLib((StaticLibOptions){
        .output = "math",
        .warnings = FLAG_WARNINGS,
        .target = t,
    });
    AddFile(mathLib, "./src/math.c");
    InstallStaticLib(mathLib);

    String ar = s(mathLib.target.ar);
    if (is_clang) {
      Assert(StrEq(ar, S("llvm-ar")), "failed, ar should be llvm-ar, instead is %s", ar.data);
    } else {
      Assert(StrEq(ar, S("x86_64-w64-mingw32-ar")), "failed, ar should be x86_64-w64-mingw32-ar, instead is %s", ar.data);
    }

    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "main",
        .warnings = FLAG_WARNINGS,
        .target = t,
    });
    AddFile(executable, "./src/main.c");
    LinkStaticLib(executable, mathLib);
    InstallExecutable(executable);

    if (is_clang) {
      FileStatsResult stats_result = FileStats(executable.ninjaBuildPath);
      Assert(stats_result.error == SUCCESS, "cross-compile test: could not get stats %s", executable.ninjaBuildPath.data);
      FileReadResult read_result = FileRead(mate_state.arena, executable.ninjaBuildPath, stats_result.data.size);
      Assert(read_result.error == SUCCESS, "cross-compile test: could not read %s", executable.ninjaBuildPath.data);
      Assert(StrIncludes(read_result.data, S("cross_target = --target=x86_64-w64-mingw32")),
             "cross-compile test: build.ninja missing derived --target=x86_64-w64-mingw32");
    }

    errno_t err = RunCommand(S("file ./build/main.exe | grep -q 'PE32+'")); // TODO: change to ./build/windows-x64-gcc/main.exe
    Assert(err == SUCCESS, "cross-compile test: ./build/main.exe is not a Windows executable");
  }
  EndBuild();
}
