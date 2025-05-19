/* MIT License

  mate.h - A single-header library for compiling your C code in C
  Version - 2025-05-19 (0.2.0):
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
  String flags;
  String arFlags;
  String ninjaBuildPath;
} StaticLib;

typedef struct {
  String output;
  String flags;
  String linkerFlags;
  String ninjaBuildPath;
} Executable;

typedef struct {
  Compiler compiler;

  // Build State
  String libs;
  String includes;
  StringVector sources;
  Executable executable;
  StaticLib staticLib;

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
  WarningsFlag warnings;
  DebugFlag debug;
  OptimizationFlag optimization;
  STDFlag std;
  ErrorFormatFlag error;
} ExecutableOptions;

typedef struct {
  char *output; // WARNING: Required
  char *flags;
  char *arFlags;
  char *includes;
  char *libs;
  WarningsFlag warnings;
  DebugFlag debug;
  OptimizationFlag optimization;
  STDFlag std;
  ErrorFormatFlag error;
} StaticLibOptions;

typedef StringBuilder FlagBuilder;

/* --- Build Functions --- */
void StartBuild(void);
void EndBuild(void);

void CreateConfig(MateOptions options);

String CreateExecutable(ExecutableOptions executableOptions);
String InstallExecutable(void);
static void mateResetExecutable(void);

String CreateStaticLib(StaticLibOptions staticLibOptions);
String InstallStaticLib(void);
static void mateResetStaticLib(void);

enum CreateCompileCommandsError { COMPILE_COMMANDS_FAILED_OPEN_FILE = 1000, COMPILE_COMMANDS_FAILED_COMPDB };
WARN_UNUSED errno_t CreateCompileCommands(String ninjaBuildPath);

#define AddLibraryPaths(...)                   \
  do {                                         \
    StringVector vector = {0};                 \
    StringVectorPushMany(vector, __VA_ARGS__); \
    mateAddLibraryPaths(&vector);              \
  } while (0)
static void mateAddLibraryPaths(StringVector *vector);

#define AddIncludePaths(...)                   \
  do {                                         \
    StringVector vector = {0};                 \
    StringVectorPushMany(vector, __VA_ARGS__); \
    mateAddIncludePaths(&vector);              \
  } while (0)
static void mateAddIncludePaths(StringVector *vector);

#define LinkSystemLibraries(...)               \
  do {                                         \
    StringVector vector = {0};                 \
    StringVectorPushMany(vector, __VA_ARGS__); \
    mateLinkSystemLibraries(&vector);          \
  } while (0)
static void mateLinkSystemLibraries(StringVector *vector);

#define AddFile(source) mateAddFile(S(source));
static void mateAddFile(String source);

#define RemoveFile(source) mateRemoveFile(S(source));
static bool mateRemoveFile(String source);

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

#define FlagBuilderAdd(builder, flag) mateFlagBuilderAdd(builder, &S(flag));
static void mateFlagBuilderAdd(FlagBuilder *builder, String *flag);

#define FlagBuilderAddMany(builder, ...)       \
  do {                                         \
    StringVector _flags = {0};                 \
    StringVectorPushMany(_flags, __VA_ARGS__); \
    mateFlagBuilderAddMany(builder, _flags);   \
  } while (0)
static void mateFlagBuilderAddMany(FlagBuilder *builder, StringVector flags);

static String mateFixPath(String str);
static String mateFixPathExe(String str);
static String mateConvertNinjaPath(String str);

static StringVector mateOutputTransformer(StringVector vector);
static bool mateGlobMatch(String pattern, String text);

#define SAMURAI_AMALGAM "SAMURAI SOURCE"

// --- MATE.H END ---
