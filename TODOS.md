# [TODOS](https://github.com/TomasBorquez/mate.h/issues/9)

## v0.3
- [ ] Make flags OR values
- [ ] Add ability to compile with g++ and other c/cpp compilers
- [ ] Minify samurai output file
- [ ] Add `args` parser
- [ ] Shared libs
- [ ] Make `samurai` windows compatible
- [ ] Full `MSVC` support on static/shared libs
- [ ] `InstallHeader()`

## v0.2
- [x] Samurai does not generate `compile_commands.json`?
- [x] Move `AddFile/RemoveFile` to use `s()` for the file names
- [x] `CreateStaticLib/CreateExecutable` were using includes instead of libs
- [x] When parsing `ExecutableOptions.output`, make sure it is not a path, just a name with/without .exe
- [x] `clang-formatter` as a github action (when many contributions)
- [x] `AddFiles` for array, and remove macros
- [x] Make all macros safe -> `()`
- [x] Scripts for tests are pretty bad, fix them
- [x] Header Dependency check
- [x] Add `MacOS` support (does work well atm)
- [x] Remove CPP support on mate.h
- [x] Amalgamation should be done on a buffer and then create a file
- [x] Simplify Default flag creation
- [x] snake_case for local names, PascalCase for public funcs, camelCase for public struct members
- [x] Remove unnecessary functions
- [ ] RunCommandF for running commands with formatting
- [ ] Add Asan flags
- [ ] Look to not create new strings on heap when not necessary
- [ ] Simplify StringBuilderAppend pattern
- [ ] Path handling is pretty ugly, improve it
- [ ] `LinkStaticLib()` for linking another `StaticLib`
- [ ] Output should be able to have path and should be parsed out of `ExecutableOptions/StaticLibOptions`

#### Examples
- [ ] Show how to use FS functions `FileWrite`, `FileDelete`, etc.
- [ ] Show how to use String functions from base.h like `F()`, `StrConcat`, `StrEq`, etc.
- [ ] Show how to use `FlagBuilder`, `AddFiles`, `CreateStaticLib`.
- [ ] raylib-source-code
    - [ ] option shared (`-fPIC`, `-DBUILD_LIBTYPE_SHARED`)
    - [ ] option linux_backend (`wayland/x11`)
    - [ ] option optional modules such as `rshapes`, `rtextures`, etc
    - [ ] option platform such as `glfw`, `SDL`, etc
    - [ ] option opengl version
    - [ ] default config from `src/config.h` else `-DEXTERNAL_CONFIG_FLAGS`
    - [ ] ^ for all use as inspiration `build.zig` from raylib

## v0.1
- [x] Create `amalgam-script.c` as well as `pre-commit hook`
- [x] Examples folder that serves as tests
- [x] Full Linux support
- [x] `FLAG_DEBUG` -g3 rather than -g *when supported* since -g3 generates more information
- [x] Add glob pattern on `AddFile` using `ListDirectory`
- [x] Switch to samurai instead of ninja files (for now only bootstrap on linux)
- [x] Actually parse `mate-cache.ini`
- [x] Add `TCC` support
- [x] Add better error messages
    - [x] Remove file errors unless they are critical, and always check errors
    - [x] Instead of assertions add `LogError/Abort` custom function
    - [x] If someone doesnt use `StartBuild` before creating executable
    - [x] Uses `InstallExecutable` before creating executable
- [x] Refactor code
    - [x] Cache states is unclear
    - [x] String function names are unclear for paths
    - [x] Optimize Mate Built-in String functions
    - [x] String functions shouldn't be by reference unless necessary
    - [x] String builder function
    - [x] Use String builder function
    - [x] Remove unnecessary code
    - [x] Add comments on `api.h`
- [x] Semi Support for `MSVC`
- [x] Static libs
- [x] `FlagBuilder` and `AddFlag(builder, ...)`:
    - [x] Similar to `StringBuilder` but it uses `mateState.arena` and `AddFlag` can add many flags plus they have a `" "` by default
    - [x] Use when parsing flags in `CreateStaticLib` and `CreateExecutable`
    - [x] Move to it in flag parsing for `./tests/07-raylib-source-code`
- [x] Move private functions to `__mate_` and snake_case
- [x] Add `FMT_I64` and `FMT_I32` to remove warnings
- [x] Move back to camelCase but mate prefix for private `mateFunction`
