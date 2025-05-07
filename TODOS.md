# [TODOS](https://github.com/TomasBorquez/mate.h/issues/9)
- [x] Create `amalgam-script.c` as well as `pre-commit hook`
- [x] Examples folder that serves as tests
- [x] Full Linux support
- [x] `FLAG_DEBUG` -g3 rather than -g *when supported* since -g3 generates more information.
- [ ] Different targets apart from `executables` such as static libs, dynamic libs, depending on platform.
- [ ] Switch to samurai instead of ninja files (for now only bootstrap on linux)
- [ ] Full MSVC support
- [ ] Header tests like on CMAKE 
- [ ] Add `TCC` support
- [ ] Add `args` parser
- [ ] Add `MacOS` support 
- [ ] Optimize String functions and add better names
- [ ] Actually parse `mate-cache.ini`
- [ ] `clang-formatter` as a github action
- [ ] Properly clean the state and malloc operations
- [ ] Add better error messages, eg. if someone doesnt use `StartBuild` before creating executable.

- [ ] Test on `Clang`, `GCC` and `MSVC` on windows
- [ ] Test on `Clang` and `GCC` on linux
