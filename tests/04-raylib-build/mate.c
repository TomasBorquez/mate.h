#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Target t = HostTarget();

  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "main",
        .warnings = FLAG_WARNINGS_NONE,
        .flags = !isMSVC(t) ? "-Wall -g": "/MD"
    });

    AddFile(executable, "./src/main.c");

    if (isLinux(t) || isFreeBSD(t)) {
      AddIncludePaths(executable, "./vendor/raylib/include");
      AddLibraryPaths(executable, "./vendor/raylib/lib/linux_amd64");
      LinkSystemLibraries(executable, "raylib", "m");
    }

    if (isWindows(t)) {
      AddIncludePaths(executable, "./vendor/raylib/include");

      if (isMSVC(t)) {
        AddLibraryPaths(executable, "./vendor/raylib/lib/win64_msvc");
        LinkSystemLibraries(executable, "raylib", "opengl32", "kernel32", "user32", "shell32", "gdi32", "winmm", "msvcrt");
      } else {
        AddLibraryPaths(executable, "./vendor/raylib/lib/win64_mingw");
        LinkSystemLibraries(executable, "raylib", "gdi32", "winmm");
      }
    }

    if (isMacOS(t)) {
      AddIncludePaths(executable, "./vendor/raylib/include");
      AddLibraryPaths(executable, "./vendor/raylib/lib/macos");
      LinkSystemLibraries(executable, "raylib");
    }

    InstallExecutable(executable);

    errno_t errExe = RunCommand(executable.outputPath);
    if (errExe != SUCCESS) {
      LogError("Running the executable failed with err: %d", errExe);
    }
    errno_t errCompileCmd = CreateCompileCommands(executable);
    Assert(errCompileCmd == SUCCESS, "Failed errCompileCmd should be SUCCESS");
  }
  EndBuild();
}
