#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  StartBuild();
  {
    CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile("./src/main.c");

    if (isLinux()) {
      // `AddIncludePaths` are usually for vendor `.h` files, in this case `./include`
      AddIncludePaths("./vendor/raylib/include");

      // Same with libraries, which contain the implementations of the includes
      AddLibraryPaths("./vendor/raylib/lib/linux_amd64/");

      // System libraries needed by raylib
      LinkSystemLibraries("raylib", "m");
    } else {
      // Same thing but for windows
      AddIncludePaths("./vendor/raylib/include");
      AddLibraryPaths("./vendor/raylib/lib/win64_mingw/");
      LinkSystemLibraries("raylib", "gdi32", "winmm", "m");
    }

    String exePath = InstallExecutable();
    RunCommand(exePath);
  }
  EndBuild();
}
