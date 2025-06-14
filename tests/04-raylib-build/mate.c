#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main(void) {
  StartBuild();
  {
    // NOTE: Add .exe to make sure it removes on linux builds
    Executable executable;
    if (isMSVC()) {
      executable = CreateExecutable((ExecutableOptions){.output = "main.exe", .warnings = FLAG_WARNINGS_NONE, .flags = "/MD"});
    } else {
      executable = CreateExecutable((ExecutableOptions){.output = "main.exe", .warnings = FLAG_WARNINGS_NONE, .flags = "-Wall -g"});
    }

    AddFile(executable, "./src/main.c");

    if (isLinux()||isFreeBSD()) {
      AddIncludePaths(executable, "./vendor/raylib/include");
      AddLibraryPaths(executable, "./vendor/raylib/lib/linux_amd64");
      LinkSystemLibraries(executable, "raylib", "m");
    }

    if (isWindows()) {
      AddIncludePaths(executable, "./vendor/raylib/include");

      if (isMSVC()) {
        AddLibraryPaths(executable, "./vendor/raylib/lib/win64_msvc");
        LinkSystemLibraries(executable, "raylib", "opengl32", "kernel32", "user32", "shell32", "gdi32", "winmm", "msvcrt");
      } else {
        AddLibraryPaths(executable, "./vendor/raylib/lib/win64_mingw");
        LinkSystemLibraries(executable, "raylib", "gdi32", "winmm");
      }
    }

    if (isMacOs()) {
      AddIncludePaths(executable, "./vendor/raylib/include");
      AddLibraryPaths(executable, "./vendor/raylib/lib/macos");
      LinkSystemLibraries(executable, "raylib");
    }

    InstallExecutable(executable);
    RunCommand(executable.outputPath);
    CreateCompileCommands(executable);
  }
  EndBuild();
}
