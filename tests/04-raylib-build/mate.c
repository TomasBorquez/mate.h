#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  StartBuild();
  {
    if (isMSVC()) {
      CreateExecutable((ExecutableOptions){.output = "main", .flags = "/MD /MACHINE:X64"});
    } else {
      CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});
    }

    AddFile("./src/main.c");

    if (isLinux()) {
      AddIncludePaths("./vendor/raylib/include");
      AddLibraryPaths("./vendor/raylib/lib/linux_amd64");
      LinkSystemLibraries("raylib", "m");
    }

    if (isWindows()) {
      AddIncludePaths("./vendor/raylib/include");

      if (isMSVC()) {
        AddLibraryPaths("./vendor/raylib/lib/win64_msvc");
      } else {
        AddLibraryPaths("./vendor/raylib/lib/win64_mingw");
      }

      LinkSystemLibraries("raylib", "opengl32", "kernel32", "user32", "shell32", "gdi32", "winmm", "msvcrt");
    }

    String exePath = InstallExecutable();
    RunCommand(exePath);
    CreateCompileCommands();
  }
  EndBuild();
}
