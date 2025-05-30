/* MIT License

  mate.h - A single-header library for compiling your C code in C
  Version - 2025-05-29 (0.2.1):
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

/* --- Type Definitions --- */
typedef struct {
  i64 lastBuild;
  bool samuraiBuild;
  bool firstBuild;
} MateCache;

typedef struct {
  Compiler compiler;
  char *buildDirectory;
  char *mateSource;
  char *mateExe;
} MateOptions;

typedef struct {
  String output;
  String outputPath;
  String flags;
  String arFlags;
  String libs;
  String includes;
  StringVector sources;
  String ninjaBuildPath;
} StaticLib;

typedef struct {
  String output;
  String outputPath;
  String flags;
  String linkerFlags;
  String libs;
  String includes;
  StringVector sources;
  String ninjaBuildPath;
} Executable;

typedef struct {
  Compiler compiler;

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

typedef enum {
  FLAG_WARNINGS_NONE = 1, // -w
  FLAG_WARNINGS_MINIMAL,  // -Wall
  FLAG_WARNINGS,          // -Wall -Wextra
  FLAG_WARNINGS_VERBOSE,  // -Wall -Wextra -Wpedantic
} WarningsFlag;

typedef enum {
  FLAG_DEBUG_MINIMAL = 1, // -g1
  FLAG_DEBUG_MEDIUM,      // -g/g2
  FLAG_DEBUG,             // -g3
} DebugFlag;

typedef enum {
  FLAG_OPTIMIZATION_NONE = 1,  // -O0
  FLAG_OPTIMIZATION_BASIC,     // -O1
  FLAG_OPTIMIZATION,           // -O2
  FLAG_OPTIMIZATION_SIZE,      // -Os
  FLAG_OPTIMIZATION_AGGRESSIVE // -O3
} OptimizationFlag;

typedef enum {
  FLAG_STD_C99 = 1, // -std=c99
  FLAG_STD_C11,     // -std=c11
  FLAG_STD_C17,     // -std=c17
  FLAG_STD_C23,     // -std=c23
  FLAG_STD_C2X      // -std=c2x
} STDFlag;

typedef enum {
  FLAG_ERROR,    // -fdiagnostics-color=always
  FLAG_ERROR_MAX // -fdiagnostics-color=always -fcolor-diagnostics ...
} ErrorFormatFlag;

typedef struct {
  char *output; // WARNING: Required
  char *flags;
  char *linkerFlags;
  char *includes;
  char *libs;

  // NOTE: Flag options
  STDFlag std;
  DebugFlag debug;
  WarningsFlag warnings;
  ErrorFormatFlag error;
  OptimizationFlag optimization;
} ExecutableOptions;

typedef struct {
  char *output; // WARNING: Required
  char *flags;
  char *arFlags;
  char *includes;
  char *libs;

  // NOTE: Flag options
  STDFlag std;
  DebugFlag debug;
  WarningsFlag warnings;
  ErrorFormatFlag error;
  OptimizationFlag optimization;
} StaticLibOptions;

typedef StringBuilder FlagBuilder;

/* --- Build System --- */
void StartBuild(void);
void EndBuild(void);

void CreateConfig(MateOptions options);

Executable CreateExecutable(ExecutableOptions executableOptions);
#define InstallExecutable(target) mateInstallExecutable(&(target))
static void mateInstallExecutable(Executable *executable);
static void mateResetExecutable(Executable *executable);

StaticLib CreateStaticLib(StaticLibOptions staticLibOptions);
#define InstallStaticLib(target) mateInstallStaticLib(&(target))
static void mateInstallStaticLib(StaticLib *staticLib);
static void mateResetStaticLib(StaticLib *staticLib);

typedef enum { COMPILE_COMMANDS_SUCCESS = 0, COMPILE_COMMANDS_FAILED_OPEN_FILE = 1000, COMPILE_COMMANDS_FAILED_COMPDB } CreateCompileCommandsError;

#define CreateCompileCommands(target) mateCreateCompileCommands(&(target).ninjaBuildPath);
static WARN_UNUSED CreateCompileCommandsError mateCreateCompileCommands(String *ninjaBuildPath);

#define AddLibraryPaths(target, ...)             \
  do {                                           \
    StringVector _libs = {0};                    \
    StringVectorPushMany(_libs, __VA_ARGS__);    \
    mateAddLibraryPaths(&(target).libs, &_libs); \
                                                 \
    /* Cleanup */                                \
    VecFree(_libs);                              \
  } while (0)
static void mateAddLibraryPaths(String *targetLibs, StringVector *libs);

#define LinkSystemLibraries(target, ...)             \
  do {                                               \
    StringVector _libs = {0};                        \
    StringVectorPushMany(_libs, __VA_ARGS__);        \
    mateLinkSystemLibraries(&(target).libs, &_libs); \
                                                     \
    /* Cleanup */                                    \
    VecFree(_libs);                                  \
  } while (0)
static void mateLinkSystemLibraries(String *targetLibs, StringVector *libs);

#define AddIncludePaths(target, ...)                     \
  do {                                                   \
    StringVector _includes = {0};                        \
    StringVectorPushMany(_includes, __VA_ARGS__);        \
    mateAddIncludePaths(&(target).includes, &_includes); \
                                                         \
    /* Cleanup */                                        \
    VecFree(_includes);                                  \
  } while (0)
static void mateAddIncludePaths(String *targetIncludes, StringVector *vector);

#define AddFile(target, source) mateAddFile(&(target).sources, s(source));
static void mateAddFile(StringVector *sources, String source);

#define AddFiles(target, files)                                                                           \
  do {                                                                                                    \
    Assert(sizeof(files) != sizeof(*(files)), "AddFiles: failed, files must be an array, not a pointer"); \
    mateAddFiles(&(target).sources, files, sizeof(files) / sizeof(*(files)));                             \
  } while (0);
static void mateAddFiles(StringVector *sources, char **source, size_t size);

#define RemoveFile(target, source) mateRemoveFile(&(target).sources, s(source));
static bool mateRemoveFile(StringVector *sources, String source);

static void mateRebuild(void);
static bool mateNeedRebuild(void);
static void mateSetDefaultState(void);

/* --- Utils --- */
String CompilerToStr(Compiler compiler);

bool isMSVC(void);
bool isGCC(void);
bool isClang(void);
bool isTCC(void);

WARN_UNUSED errno_t RunCommand(String command);

StringBuilder FlagBuilderCreate(void);
void FlagBuilderAddString(FlagBuilder *builder, String *flag);

String CompilerToStr(Compiler compiler);

WARN_UNUSED errno_t RunCommand(String command);

StringBuilder FlagBuilderCreate(void);
void FlagBuilderAddString(FlagBuilder *builder, String *flag);

#define FlagBuilderReserve(count) mateFlagBuilderReserve(count);
static FlagBuilder mateFlagBuilderReserve(size_t count);

#define FlagBuilderAdd(builder, ...)           \
  do {                                         \
    StringVector _flags = {0};                 \
    StringVectorPushMany(_flags, __VA_ARGS__); \
    mateFlagBuilderAdd(builder, _flags);       \
                                               \
    /* Cleanup */                              \
    VecFree(_flags);                           \
  } while (0)
static void mateFlagBuilderAdd(FlagBuilder *builder, StringVector flags);

static String mateFixPath(String str);
static String mateFixPathExe(String str);
static String mateConvertNinjaPath(String str);

static StringVector mateOutputTransformer(StringVector vector);
static bool mateGlobMatch(String pattern, String text);

#define SAMURAI_AMALGAM "SAMURAI SOURCE"

// --- MATE.H END ---
