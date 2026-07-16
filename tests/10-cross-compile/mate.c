#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Target t = {
      .os = OS_WINDOWS,
      .arch = ARCH_X64,
      .compiler = "x86_64-w64-mingw32-gcc",
      .compilerFamily = GCC,
  };

  StartBuild();
  {
    StaticLib mathLib = CreateStaticLib((StaticLibOptions) {
        .output = "math",
        .warnings = FLAG_WARNINGS,
        .target = t,
    });

    AddFile(mathLib, "./src/math.c");

    InstallStaticLib(mathLib);

    String ar = s(mathLib.target.ar);
    Assert(StrEq(ar, S("x86_64-w64-mingw32-ar")), "failed, ar should be x86_64-w64-mingw32-ar, instead is %s", ar.data);

    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "main",
        .warnings = FLAG_WARNINGS,
        .target = t,
    });

    AddFile(executable, "./src/main.c");

    LinkStaticLib(executable, mathLib);
    InstallExecutable(executable);

    errno_t err = RunCommand(S("file ./build/main.exe | grep -q 'PE32+'")); // TODO: change to ./build/windows-x64-gcc/main.exe
    Assert(err == SUCCESS, "cross-compile test: ./build/main.exe is not a Windows executable");
  }
  EndBuild();
}
