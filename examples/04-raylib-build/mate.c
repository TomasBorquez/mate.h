#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  errno_t result;

  StartBuild();
  {
    CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -g"});

    AddFile("./src/main.c");

#if defined(PLATFORM_LINUX)
    // Includes are usually for vendor `.h` files, in this case `./include`
    AddIncludePaths("./vendor/raylib/linux_amd64/include");
    // Same with libraries, which contain the implementations of the includes
    AddLibraryPaths("./vendor/raylib/linux_amd64/lib");
    // System libraries needed by raylib
    LinkSystemLibraries("raylib", "m");
#elif defined(PLATFORM_WIN)
    // Same thing but for windows
    AddIncludePaths("./vendor/raylib/win64_mingw/include");
    AddLibraryPaths("./vendor/raylib/win64_mingw/lib");
    LinkSystemLibraries("raylib", "gdi32", "winmm", "m");
#endif

    String exePath = InstallExecutable();
    errno_t errExe = RunCommand(exePath);
    errno_t errCompileCmd = CreateCompileCommands();
    result = errExe == 0 && errCompileCmd == 0;
  }
  EndBuild();

  return result;
}
