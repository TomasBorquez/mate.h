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

# TODO:
- [x] Return `errno_t` and `LogError` rather than asserting
- [x] Create `AddLibrary`, `AddInclude` and `linkerFlags`
- [x] Create the Strings under the hood with macros or `s()`
- [x] Clearer comments of when implementation of stuff happens
- [x] Create a new repo for `base.h`
- [ ] Create enums of errors:
    - [x] `GeneralError`
    - [x] `FileStatsError`
    - [x] `FileReadError`
    - [ ] `FileWriteError`
    - [ ] `FileDeleteError`
    - [ ] `FileRenameError`
    - [ ] `CreateCompileCommands`
- [ ] Add equivalent FileSystem implementation for linux
- [ ] Assert `Malloc` and `ArenaAlloc`, use `Malloc` and `Free`
- [ ] Add version at the top of the file and add proper license
- [ ] Make GNU extensions work in both MSVC and Clang (ex: Vectors)
- [ ] Test on `Clang`, `GCC` and `MSVC` on windows
- [ ] Test on `Clang` and `GCC` on linux
- [ ] Add `TCC` support
- [ ] Optimize String functions and add better names
- [ ] Actually parse `mate-cache.json` lmao
- [ ] Properly clean the state and malloc operations
- [ ] Add better error messages, eg. if someone doesnt use `StartBuild` before creating executable.
