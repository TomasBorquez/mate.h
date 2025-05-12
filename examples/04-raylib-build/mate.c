#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  StartBuild();
  {
    CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile("./src/main.c");

#if defined(PLATFORM_LINUX)
    // `AddIncludePaths` are usually for vendor `.h` files, in this case `./include`
    AddIncludePaths("./vendor/raylib/include");

    // Same with libraries, which contain the implementations of the includes
    AddLibraryPaths("./vendor/raylib/lib/linux_amd64/");

    // System libraries needed by raylib
    LinkSystemLibraries("raylib", "m");
#elif defined(PLATFORM_WIN)
    // Same thing but for windows
    AddIncludePaths("./vendor/raylib/include");
    AddLibraryPaths("./vendor/raylib/lib/win64_mingw");
    LinkSystemLibraries("raylib", "gdi32", "winmm", "m");
#endif

    String exePath = InstallExecutable();
    RunCommand(exePath);
    CreateCompileCommands();
  }
  EndBuild();
}
