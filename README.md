> [!WARNING]
> This is still very experimental

# About:
Inspired from `build.zig`, `nob.h` and `CMake` I created my own build system with the goal to reivent the wheel
and use my own build system for my projects, for now it has limited support multiple platforms, the main one is 
`gcc` in `MINGW` and Windows, but I'm planning on supporting `Clang`, `MSVC` and `TCC`, as well as `Linux`. This
is very much experimental but if you want to use it here is an example for building a `raylib` project:

```c 
#define MATE_IMPLEMENTATION
#include "mate.h"

i32 main() {
  StartBuild();
  {
    // Sets the output name and different flags
    CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall -ggdb"});

    // Files to compile
    AddFile("./src/main.c");

    // Libraries and includes
    AddIncludePaths("C:/raylib/include", "./src");
    AddLibraryPaths("C:/raylib/lib");
    LinkSystemLibraries("raylib", "opengl32", "gdi32", "winmm");

    // Compiles all files parallely with ninja
    String exePath = InstallExecutable();

    // Runs `./build/main.exe` or whatever your main file is
    RunCommand(exePath);

    // Creates a `compile_commands.json`
    CreateCompileCommands();
  }
  EndBuild();
}
```

All you need is `mate.h` on the same folder you have `mate.c` example file and run `gcc ./mate.c -o ./mate.exe && ./mate.exe`, after running once
it will detect if you need to rebuild based on `./build/mate-cache.json`. 

You also need ninja installed and available in your `PATH` since everything is parsed into a `build.ninja` for performance reasons (like incremental builds, parallel compilation, ...) 
and for making it easier to cross platform.
