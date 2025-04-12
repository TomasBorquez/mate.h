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
    CreateExecutable((Executable){.output = S("main"), .flags = S("-Wall -ggdb")});

    // Files to compile
    AddFile(S("./src/main.c"));

    // Libraries and includes
    AddIncludePaths(S("C:/raylib/include"), S("./src"));
    AddLibraryPaths(S("C:/raylib/lib"));
    LinkSystemLibraries(S("raylib"), S("opengl32"), S("gdi32"), S("winmm"));

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

# TODO:
- [x] Return `errno_t` and `LogError` rather than asserting
- [x] Create `AddLibrary`, `AddInclude` and `linkerFlags`
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
- [ ] Create a new repo for `base.h`
- [ ] Make GNU extensions work in both MSVC and Clang (ex: Vectors)
- [ ] Test on `Clang`, `GCC` and `MSVC` on windows
- [ ] Test on `Clang` and `GCC` on linux
- [ ] Add `TCC` support
- [ ] Optimize String functions and add better names
- [ ] Actually parse `mate-cache.json` lmao
