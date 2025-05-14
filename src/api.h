/* MIT License

  mate.h - A single-header library for compiling your C code in C
  Version - 2025-05-14 (0.1.9):
  https://github.com/TomasBorquez/mate.h

  Guide on the `README.md`
*/
#pragma once

#ifdef MATE_IMPLEMENTATION
#  define BASE_IMPLEMENTATION
#endif

#include "../vendor/base/base.h"

// --- MATE.H START ---
/* MIT License
   mate.h - Mate Definitions start here
   Guide on the `README.md`
*/
typedef struct {
  i64 lastBuild;
  bool samuraiBuild;
  bool firstBuild;
} MateCache;

typedef struct {
  char *compiler;
  char *buildDirectory;
  char *mateSource;
  char *mateExe;
} MateOptions;

typedef struct {
  String compiler;

  // Paths
  String buildDirectory;
  String mateSource;
  String mateExe;

  // Cache
  MateCache mateCache;
  IniFile cache;

  // Misc
  Arena *arena;
  bool initConfig;

  i64 startTime;
  i64 totalTime;
} MateConfig;

typedef struct {
  String output;
  String flags;
  String arFlags;
  String includes;
  String ninjaBuildPath;
} StaticLib;

typedef struct {
  String output;
  String flags;
  String linkerFlags;
  String includes;
  String libs;
  String ninjaBuildPath;
} Executable;

enum WarningsFlags {
  FLAG_WARNINGS_MINIMAL = 1, // -Wall
  FLAG_WARNINGS,             // -Wall -Wextra
  FLAG_WARNINGS_VERBOSE,     // -Wall -Wextra -Wpedantic
};

enum DebugFlags {
  FLAG_DEBUG_MINIMAL = 1, // -g1
  FLAG_DEBUG_MEDIUM,      // -g/g2
  FLAG_DEBUG,             // -g3
};

enum OptimizationFlags {
  FLAG_OPTIMIZATION_NONE = 1,  // -O0
  FLAG_OPTIMIZATION_BASIC,     // -O1
  FLAG_OPTIMIZATION,           // -O2
  FLAG_OPTIMIZATION_SIZE,      // -Os
  FLAG_OPTIMIZATION_AGGRESSIVE // -O3
};

enum StandardFlags {
  FLAG_STD_C99 = 1, // -std=c99
  FLAG_STD_C11,     // -std=c11
  FLAG_STD_C17,     // -std=c17
  FLAG_STD_C23,     // -std=c23
  FLAG_STD_C2X      // -std=c2x
};

typedef struct {
  char *output;
  char *flags;
  char *linkerFlags;
  char *includes;
  char *libs;
  u8 warnings;
  u8 debug;
  u8 optimization;
  u8 std;
} ExecutableOptions;

typedef struct {
  char *output;
  char *flags;
  char *arFlags;
  char *includes;
  char *libs;
  u8 warnings;
  u8 debug;
  u8 optimization;
  u8 std;
} StaticLibOptions;

void StartBuild();
void EndBuild();

void CreateConfig(MateOptions options);

String CreateExecutable(ExecutableOptions executableOptions);
String InstallExecutable();
void ResetExecutable();

String CreateStaticLib(StaticLibOptions staticLibOptions);
String InstallStaticLib();
void ResetStaticLib();

WARN_UNUSED errno_t RunCommand(String command);

enum CreateCompileCommandsError { COMPILE_COMMANDS_FAILED_OPEN_FILE = 1000, COMPILE_COMMANDS_FAILED_COMPDB };
WARN_UNUSED errno_t CreateCompileCommands(String ninjaBuildPath);

#define AddLibraryPaths(...)                   \
  do {                                         \
    StringVector vector = {0};                 \
    StringVectorPushMany(vector, __VA_ARGS__); \
    addLibraryPaths(&vector);                  \
  } while (0)
static void addLibraryPaths(StringVector *vector);

#define AddIncludePaths(...)                   \
  do {                                         \
    StringVector vector = {0};                 \
    StringVectorPushMany(vector, __VA_ARGS__); \
    addIncludePaths(&vector);                  \
  } while (0)
static void addIncludePaths(StringVector *vector);

#define LinkSystemLibraries(...)               \
  do {                                         \
    StringVector vector = {0};                 \
    StringVectorPushMany(vector, __VA_ARGS__); \
    linkSystemLibraries(&vector);              \
  } while (0)
static void linkSystemLibraries(StringVector *vector);

#define AddFile(source) addFile(S(source));
static void addFile(String source);

#define RemoveFile(source) removeFile(S(source));
static bool removeFile(String source);

static void reBuild();
static bool needRebuild();
static void setDefaultState();

// Utility platform/compiler functions
bool isWindows();
bool isLinux();

bool isMSVC();
bool isGCC();
bool isClang();
bool isTCC();

#define SAMURAI_AMALGAM "SAMURAI SOURCE"

// --- MATE.H END ---
