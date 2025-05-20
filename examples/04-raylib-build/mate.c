#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main(void) {
  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile(executable, "./src/main.c");

    if (isLinux()) {
      // `AddIncludePaths` are usually for vendor `.h` files, in this case `./include`
      AddIncludePaths(executable, "./vendor/raylib/include");

      // Same with libraries, which contain the implementations of the includes
      AddLibraryPaths(executable, "./vendor/raylib/lib/linux_amd64/");

      // System libraries needed by raylib
      LinkSystemLibraries(executable, "raylib", "m");
    } else {
      // Same thing but for windows
      AddIncludePaths(executable, "./vendor/raylib/include");
      AddLibraryPaths(executable, "./vendor/raylib/lib/win64_mingw/");
      LinkSystemLibraries(executable, "raylib", "gdi32", "winmm", "m");
    }

    InstallExecutable(executable);
    RunCommand(executable.outputPath);
  }
  EndBuild();
}
