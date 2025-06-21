# [TODOS](https://github.com/TomasBorquez/mate.h/issues/9)

## v0.2

#### Bugs
- [x] Samurai does not generate `compile_commands.json`?
- [x] Move `AddFile/RemoveFile` to use `s()` for the file names
- [x] `CreateStaticLib/CreateExecutable` were using includes instead of libs

#### Features
- [x] When parsing `ExecutableOptions.output`, make sure it is not a path, just a name with/without .exe
- [x] `clang-formatter` as a github action (when many contributions)
- [x] `AddFiles` for array, and remove macros
- [x] Make all macros `()`
- [ ] Amalgam workflow on PRs
- [ ] Add `args` parser
- [ ] Shared libs
- [ ] `LinkStaticLib()` for linking another `StaticLib`
- [ ] Full `MSVC` support on static/shared libs
- [ ] library builder functions
    - [ ] `InstallHeader()`
    - [ ] Amalgam functions
    - [ ] ...
- [ ] Header tests like on `CMAKE`
- [ ] Add `MacOS` support (find someone who is willing to help)
- [ ] Make `samurai` windows compatible
- [ ] Properly clean the state of arenas and vectors
- [ ] Parallel Dependency check

#### Examples
- [X] Show how to use FS functions `FileWrite`, `FileDelete`, etc.
- [X] Show how to use String functions from base.h like `F()`, `StrConcat`, `StrEq`, etc.
- [ ] Show how to use `FlagBuilder`, `AddFiles`, `CreateStaticLib`.

#### Tests
- [ ] test mate functions and that the output is correct
    - [ ] `AddFiles` and `AddFile`
    - [ ] check `CreateCompileCommands` works correctly
- [ ] test that checks rebuild works correctly
- [ ] test that checks illegal stuff doesn't pass without an assertion
    - [ ] like `InstallExecutable` without `CreateExecutable`
    - [ ] like `CreateExecutable` without `StartBuild`
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
