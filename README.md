# About
Mate is a build system for C in C and all you need to run it is a C compiler, if you want to learn more about it check out our [examples](./examples) which should
teach you everything you need to know to use it, here is a simple example of a `mate.c`:

```c 
#define MATE_IMPLEMENTATION // Adds function implementations 
#include "mate.h"

i32 main() {
  StartBuild();
  {
    CreateExecutable((ExecutableOptions){
        .output = "main",   // output name, in windows this becomes `main.exe` automatically and on linux it stays as `main`
        .flags = "-Wall -g" // adds warnings and debug symbols
    });

    // Files to compile
    AddFile("./src/main.c");

    // Compiles all files parallely with samurai
    String exePath = InstallExecutable();

    // Runs `./build/main` or `./build/main.exe` depending on the platform
    RunCommand(exePath);
  }
  EndBuild();
}
```

To use it al you need is `mate.h` on the same folder you have the `mate.c` example file and you can run it like `gcc ./mate.c -o ./mate.exe && ./mate.exe`, after running once
it will auto detect if you need to rebuild based on `./build/mate-cache.ini`, so you can just run `./mate` directly.

Under the hood we use [Samurai](https://github.com/michaelforney/samurai), a ninja rewrite in C, we add it to the source code and compile it on the first run, after that
it is cached and thanks to it builds are extremely fast.

## Compatibility
- linux, supported.
- windows, supported. (you'll need to use ninja since samurai is not yet supported, we are [working](https://github.com/TomasBorquez/mate.h/issues/2) on it though)
- macos, somewhat supported. (mate was made to be as compatible as possible but we have limited testing in macos)

- [gcc](https://gcc.gnu.org/), supported. (it was mainly made for it so it has the best support)
- [clang](https://releases.llvm.org/), supported.
- [tinyc](https://bellard.org/tcc/), supported.
- [msvc](https://visualstudio.microsoft.com/vs/features/cplusplus/), unsupported yet (you can use all of the above in MinGW perfectly well)

## Acknowledgments
This project incorporates code from [Samurai](https://github.com/michaelforney/samurai), which is primarily licensed under ISC license by Michael Forney, 
with portions under Apache License 2.0 and MIT licenses. The full license text is available in the [LICENSE-SAMURAI.txt](./LICENSE-SAMURAI.txt) file.

Inspired from `build.zig`, `nob.h`, `CMake` and other unrelated C libraries.
