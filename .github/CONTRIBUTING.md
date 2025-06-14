# Contributing to mate.h
Thank you for your interest in contributing to mate.h! This document outlines how to contribute to this C build system project.

## About mate.h
mate.h is a build system for C written in C. It aims to provide a simple, fast, and dependency-free build solution where all you need is a C compiler. The project uses Samurai (a ninja rewrite in C) under the hood for fast parallel builds.

## Getting Started
### Prerequisites
- A C compiler (GCC, Clang, MSVC, or TCC)
- A way to run `.sh` files (on **Windows** you can use MinGW or git bash)
- Git

### Setting up the Development Environment
1. **Fork the repository** on GitHub
2. **Clone your fork with submodules:**
    ```bash
    git clone --recursive https://github.com/YOUR_USERNAME/mate.h.git
    cd mate.h
    ```
    Or if you already cloned without --recursive:
    ```bash
    git clone https://github.com/YOUR_USERNAME/mate.h.git
    cd mate.h
    git submodule update --init --recursive
    ```
    This is **important** since `base.h` is a submodule and more could be added in the future.
3. **Important:** Do NOT use `mate.h` directly for development. It is a 6,000+ line amalgamated file. Instead, work with the individual source files:
   - `./src/api.h` - Main API header
   - `./src/api.c` - Main API implementation
   - `./vendor/base/base.h` - This is our own cross-platform standard library
   - `./vendor/samurai/` - This is the mentioned rewrite of ninja in C
4. **Amalgam:** Once you made your edits run the script:
    ```bash
   ./scripts/run-amalgam.sh
    ```
    This will add all your changes to the main `mate.h`, which is necessary if you want to run your changes on tests
5. **Run tests** for your platform:
   ```bash
   # Linux
   ./scripts/run-tests-linux.sh

   # macOS
   ./scripts/run-tests-macos.sh

   # Windows
   ./scripts/run-tests-windows.sh

   # FreeBSD
   ./scripts/run-tests-freebsd.sh
   ```
   This is important to know that you didn't break anything, and if you did you get to know why

## How to Contribute
### Reporting Issues
Before creating an issue, please:
1. **Search existing issues** to avoid duplicates
2. **Provide clear reproduction steps** including:
   - Operating system and version
   - Compiler version
   - Use mate.h latest version
   - Expected vs actual behavior
   - Minimal code example that reproduces the issue if necessary

### Suggesting Features
When suggesting new features:
1. **Check if the feature aligns** with mate.h's philosophy of simplicity
2. **Explain the use case** and why it's beneficial
3. **Consider backwards compatibility** (this is not as important for now since we haven't hit 1.0 but still good not to)
4. **Propose implementation details** if possible

### Code Contributions
You can contribute based on our existing [TODOS](../TODOS.md) or from our [issues](https://github.com/TomasBorquez/mate.h/issues) tab
#### Development Workflow
1. **Create a feature branch:**
   ```bash
   git checkout -b feature/your-feature-name
   ```
2. **Make your changes** in the source files:
   - Edit `./src/api.h` and `./src/api.c` for main functionality
   - Vendor packages are automatically managed in `./vendor/` (base.h, samurai)
3. **Generate the amalgamated header:**
   ```bash
   ./scripts/run-amalgam.sh
   ```
   This script combines all source files into the single `mate.h` header.
4. **Test your changes:**
   ```bash
   # Run platform-specific tests
   ./scripts/run-tests-linux.sh      # For Linux
   ./scripts/run-tests-macos.sh      # For macOS
   ./scripts/run-tests-windows.sh    # For Windows
   ./scripts/run-tests-freebsd.sh    # For FreeBSD
   ```
5. **Commit your changes:**
   ```bash
   git add .
   git commit -m "TYPE_OF_CHANGE: brief description of your changes"
   ```
6. **Push to your fork:**
   ```bash
   # Feature
   git push origin feature/your-feature-name
   # Fix
   git push origin fix/your-feature-name
   # Chore
   git push origin chore/your-feature-name
   ```
7. **Create a Pull Request** with a clear description

#### Coding Standards
- **Follow C99 standards** for maximum compatibility
- **No GNU Extensions** or if used they must be compatible with all the supported compilers
- **Use the project's `.clang-format` file** for consistent formatting
- **Naming conventions** (following Go-style conventions):
  - **camelCase** for private functions and variables
  - **PascalCase** for global/public functions and variables
- **Avoid comments** only used when unclear purpose or documentation
- **Work in source files**, not the amalgamated `mate.h`

#### Code Style Guidelines
```c
// Global/Public functions: PascalCase
ExecutableOptions CreateExecutableOptions();
void InstallExecutable(Executable executable);

// Global variables: PascalCase
extern const String DefaultCompiler;

// Private functions: camelCase
static void buildExecutable(Executable* exec);
static bool checkFileExists(String* path);

// Private in-function variables: camelCase
char* outputPath = "build/main";
bool isVerbose = false;
f32 version = 0.69f;

// Constants: UPPER_SNAKE_CASE
#define MATE_MAX_FILES 1024

// Structs/Types: PascalCase
typedef struct {
    char* output;
    char* flags;
} ExecutableOptions;

// Use built-in cross-platform types like (more info in ./vendor/base/base.h):
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef struct {
  size_t length; // Does not include null terminator
  char *data;
} String;

#define FMT_I8 "%" PRIi8
#define FMT_I16 "%" PRIi16
#define FMT_I32 "%" PRIi32
#define FMT_I64 "%" PRIi64

#define FMT_U8 "%" PRIu8
#define FMT_U16 "%" PRIu16
#define FMT_U32 "%" PRIu32
#define FMT_U64 "%" PRIu64

// Use PLATFORM/COMPILER macros/functions (more info in ./vendor/base/base.h):
bool isLinux(void);
bool isMacOs(void);
bool isWindows(void);

typedef enum { WINDOWS = 1, LINUX, MACOS } Platform;
Platform GetPlatform(void);

typedef enum { GCC = 1, CLANG, TCC, MSVC } Compiler;
Compiler GetCompiler(void);

#if defined(__clang__)
#  define COMPILER_CLANG
#elif defined(_MSC_VER)
#  define COMPILER_MSVC
#elif defined(__GNUC__)
#  define COMPILER_GCC
#elif defined(__TINYC__)
#  define COMPILER_TCC
#else
#  error "The codebase only supports GCC, Clang, TCC and MSVC"
#endif
```

#### Documentation
- **Update README.md** if your changes affect usage
- **Add inline documentation** for new API functions
- **Update examples** if api breaks
- **Document breaking changes** clearly

### Platform-Specific Contributions
mate.h aims to support multiple platforms:
- **Linux** ✅ (Primary support)
- **macOS** ✅ (Supported)
- **Windows** 🚧 (In progress MSVC, MinGW works fine)
- **FreeBSD** 🚧 (In progress GCC, Clang works fine)
- **BSD variants** (Community contributions welcome)

When contributing platform-specific code:
- Use `#if defined(PLATFORM_YOUR_PLATFORM)` guards appropriately
- Test on the target platform
- Maintain compatibility with existing platforms (or say in your PR the platforms your)

### Testing
Currently, mate.h relies on manual testing, CI/CD and examples. When contributing:
1. **Test your changes** with the provided examples
2. **Create new examples** for new features
3. **Verify cache behavior** (mate-cache.ini functionality if changed)
It's **important** you catch your errors in development and not in production

#### Testing Checklist
- [ ] Basic compilation works
- [ ] Tests run without errors
- [ ] Cache system functions properly
- [ ] Cross-platform compatibility (if applicable)

## Submitting Pull Requests
### Pull Request Guidelines
1. **Use a clear title** that describes the change
2. **Reference related issues** using `#issue-number` (if existing fix/feature-request)
3. **Provide a detailed description** including:
   - What changes were made
   - Why they were necessary
   - Any breaking changes
   - How to test the changes if not expicit

## Project Structure
```
mate.h/
├── mate.h              # Amalgamated single-header library (auto-generated)
├── src/
│   ├── api.h           # Main API header (primary development file)
│   └── api.c           # Main API implementation (primary development file)
├── vendor/             # Vendor packages (git submodules)
│   ├── base.h          # Base utilities (auto-managed)
│   └── samurai/        # Samurai build system (manually-managed planned to be a fork)
├── scripts/
│   ├── run-amalgam.sh  # Script to generate mate.h from source files
│   ├── run-tests-linux.sh
│   ├── run-tests-macos.sh
│   └── run-tests-windows.sh
├── examples/           # Usage examples
├── .clang-format       # Code formatting rules
├── LICENSE             # MIT License
├── LICENSE-SAMURAI.txt # Samurai license
└── README.md           # Project documentation
```

## Build System Integration
mate.h bootstraps Samurai on first run and caches it. When contributing:
- **Understand the Samurai integration**
- **Maintain cache compatibility**
- **Consider build performance impact**
- **Test incremental builds**

## Community Guidelines
### Communication
- **Use discord** for general questions and fast discussions, no need for formality :)
- **Use GitHub issues** for bug reports and feature requests
- **Use pull requests** for code discussions
- **Be patient** with review processes
- **Ask questions** if anything is unclear

## Inspiration and Philosophy
mate.h is inspired by:
- build.zig - Simple, code-based build configuration
- nob.h - Single-header C build system approach
- CMake - Cross-platform build capabilities
- raylib - Header-only library philosophy

The project aims to maintain:
- **Simplicity** over feature bloat
- **Performance** through smart caching and parallel builds
- **Portability** cross platforms and compilers
- **Zero dependencies** beyond a C compiler and bootstrapped vendor packages

## Release Process
Releases follow semantic versioning:
- **Major** version for breaking changes
- **Minor** version for new features
- **Patch** version for bug fixes

## License
By contributing to mate.h, you agree that your contributions will be licensed under the same MIT License that covers the project.

## Getting Help
If you need help:
1. **Check the examples** directory
2. **Read the README.md** thoroughly
3. **Search existing issues**
4. **Create a new issue** with the "question" label
5. **Use the Discord** by just asking on the mate.h text channel, [join](https://discord.gg/TSuGhzas5V)!

## Recognition
Contributors are recognized in:
- Git commit history
- Release notes for significant contributions
- README.md acknowledgments section
- And if any donations they are given to the most active depending on how the repo owner (Lewboski) decides to fairly split them

Thank you for helping make mate.h better and keeping the dream alive :)
