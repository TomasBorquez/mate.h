#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "main",
        .warnings = FLAG_WARNINGS,
        .target = {
          .os = OS_WINDOWS,
          .arch = ARCH_X64,
          .compiler = "x86_64-w64-mingw32-gcc",
          .compilerFamily = GCC,
        }
    });

    AddFile(executable, "./src/main.c");

    InstallExecutable(executable);

    errno_t err = RunCommand(S("file ./build/main.exe | grep -q 'PE32+'")); // TODO: change to ./build/windows-x64-gcc/main.exe
    Assert(err == SUCCESS, "cross-compile test: ./build/main.exe is not a Windows executable");
  }
  EndBuild();
}
