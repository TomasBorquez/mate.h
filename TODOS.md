# [TODOS](https://github.com/TomasBorquez/mate.h/issues/9)
- [x] Create `amalgam-script.c` as well as `pre-commit hook`.
- [x] Examples folder that serves as tests.
- [x] Full Linux support.
- [x] `FLAG_DEBUG` -g3 rather than -g *when supported* since -g3 generates more information.
- [x] Add glob pattern on `AddFile` using `ListDirectory`.
- [x] Switch to samurai instead of ninja files (for now only bootstrap on linux).
- [x] Actually parse `mate-cache.ini`.
- [x] Add `TCC` support
- [ ] Different targets apart from `executables` such as static libs, dynamic libs, depending on platform.
- [ ] Full `MSVC` support.
- [ ] `clang-formatter` as a github action.
- [ ] Add better error messages, eg. if someone doesnt use `StartBuild` before creating executable.

### Last stage
- [ ] Add `args` parser.
- [ ] Header tests like on `CMAKE`.
- [ ] Test on `Clang`, `GCC` and `MSVC` on windows.
- [ ] Test on `Clang` and `GCC` on linux.
- [ ] Add `MacOS` support.
- [ ] Optimize String functions and add better names.
- [ ] Properly clean the state and malloc operations.
- [ ] Make `samurai` windows compatible.
