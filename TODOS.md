## Future:
### Tests/Examples
- [x] Show how to use `FlagBuilder`, `AddFiles`, `CreateStaticLib`.
- [x] Add a cross-compile test - check its a windows binary too.
- [x] Remove all examples and create a blog post and guide page.
- [x] Test rebuilding scripts (just a second run).
- [x] Clang `--target` test
- [ ] Test that asserts fail
- [ ] Parallel running tests
- [ ] Create a documentation page

### MSVC
- [ ] Add `msvc_deps_prefix` for MSVC in a different language or VSLANG=1033
- [ ] On MSVC shared lib build, FileStats the expected .lib and check exports
- [ ] `/MD` and `/MDd` should be the default flags for shared libs

### GCC
- [ ] `--start-group` and `--end-group` for transitive deps
- [ ] flags dedupping

## v0.5
- [ ] SubProjects:
    - [ ] Mate cache should save installed targets
    - [ ] Get transitive dependencies through `mate-cache.ini`
    - [ ] `AddSubProject("./shared/raylib")` and a `AddSubProjectEx` variant for args?
    - [ ] `SubProjectGetStaticLib(rl, "raylib");`
    - [ ] `SubProjectGetSharedLib(rl, "raylib");`
    - [ ] Private `LinkSystemLibraries` for transitive deps on shared libs
- [ ] `FindSystemPackage(exe, "sdl3")`
- [ ] `FetchDependency(url, hash)`
- [ ] We should own all `char *` or pointers user gives
- [ ] Make `samurai` windows compatible
- [ ] `mate_program_exists` cache

## v0.4
- [ ] Cleanup:
    - [ ] Assertions should follow some structure, utility functions for repeating asserts
    - [ ] Abstract functions repeated more than 3 times (2 if too long)
    - [ ] Proper implementation/definition group
    - [ ] Proper implementation/definition order
    - [ ] Consistent variable names and follow private/public rules
    - [ ] Flag dedup, if default flag is x, and uses passes x, remove users and warn
    - [ ] Cleaner output on ninja description
    - [ ] `/nologo`, `/showIncludes`, `-fPIC`, ... should be part of ninja variables instead
    - [ ] Consistent formatting and line length, add a `.clang-format`
    - [ ] Minify samurai file
    - [ ] Update CONTRIBUTING.md
- [ ] Custom build step
- [ ] `RunExec` for running executable
- [ ] Move to `exec` && `CreateProcess` for executables

## v0.3
- [x] `LinkStaticLib()` for linking another `StaticLib`
- [x] Shared libraries support
- [x] `NormPathExe/StaticLib/SharedLib` should assert no extension added
- [x] `LinkStaticLib/LinkSharedLib` should assert that both were installed before
- [x] `lib` prefix on output in `CreateStaticLib/CreateSharedLib` if not windows target
- [x] Target field instead of global compiler
- [x] `ar` for cross-compiler
- [x] Full `MSVC` support on static/shared libs
    - [x] GetAr
    - [x] StaticLib
    - [x] SharedLib
    - [x] Raylib example
    - [x] Replace `/Zi` with `/Z7` and warn
    - [x] Warn on `/Zi` and add `/Debug`
- [x] Better compiler naming, code nagivation, move arch to base.h
- [x] On comments add `Implementation/Definition` per feature
- [x] Add cross-compile target for clang with `--target=<triple>`
- [ ] `StrIsEmpty` instead of `StrIsNull`
- [ ] Build output on folder depending on platform/arch
- [ ] Add `args` parser
- [ ] base.h - Generic HashMap

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
- [x] RunCommandF for running commands with formatting
- [x] Add Asan flags
- [x] Look to not create new strings on heap when not necessary
    - [x] StrNew
    - [x] StrNewSize
- [x] Add cleaner build messages in ninja like CMake does
- [x] Avoid using StringVector for simple stuff instead use __VA_ARGS__
    - [x] AddLibraryPaths
    - [x] LinkSystemLibraries
    - [x] LinkFrameworks
    - [x] LinkFrameworksWithOptions
    - [x] AddIncludePaths
    - [x] AddFrameworkPaths
- [x] AddFile/AddFiles should just be one
- [x] Proper cleanup
- [x] Replace all instances of camelCase to snake_case in proper cases
- [x] Path handling is pretty ugly, improve it
- [x] base.h
    - [x] Fix UB error in IniGet/IniSet
    - [x] Move ARR_LEN() to base.h
    - [x] Simplify StringBuilderAppend pattern
    - [x] F() -> StringBuilderAppendF with custom fast formatting
    - [x] SBAddS for automatic String
- [x] Support other compilers like c++

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
