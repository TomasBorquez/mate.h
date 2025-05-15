#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  StartBuild();
  {
    // Add .exe to make sure it removes on linux builds
    String buildNinjaPath;
    if (isMSVC()) {
      buildNinjaPath = CreateExecutable((ExecutableOptions){.output = "main.exe", .flags = "/MD /MACHINE:X64"});
    } else {
      buildNinjaPath = CreateExecutable((ExecutableOptions){.output = "main.exe", .flags = "-Wall -g"});
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
        LinkSystemLibraries("raylib", "opengl32", "kernel32", "user32", "shell32", "gdi32", "winmm", "msvcrt");
      } else {
        AddLibraryPaths("./vendor/raylib/lib/win64_mingw");
        LinkSystemLibraries("raylib", "gdi32", "winmm");
      }
    }

    String exePath = InstallExecutable();
    RunCommand(exePath);
    CreateCompileCommands(buildNinjaPath);
  }
  EndBuild();
}
