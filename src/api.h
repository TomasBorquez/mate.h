/* MIT License

  mate.h - A single-header library for compiling your C code in C
  Version - 2025-05-03 (0.1.5):
  https://github.com/TomasBorquez/mate.h

  Guide on the `README.md`
*/
#pragma once

#ifdef MATE_IMPLEMENTATION
#define BASE_IMPLEMENTATION
#endif

#include "../vendor/base/base.h"

// --- MATE.H START ---
/* MIT License
   mate.h - Mate Definitions start here
   Guide on the `README.md`
*/
typedef struct {
  i64 lastBuild;
  bool firstBuild;
} MateCache;

typedef struct {
  char *compiler;
  char *buildDirectory;
  char *mateSource;
  char *mateExe;
  char *mateCachePath;
} MateOptions;

typedef struct {
  String compiler;

  // Paths
  String buildDirectory;
  String mateSource;
  String mateExe;
  String mateCachePath;

  // Cache
  MateCache mateCache;

  // Misc
  Arena arena;
  bool customConfig;
  i64 startTime;
  i64 totalTime;
} MateConfig;

typedef struct {
  String output;
  String flags;
  String linkerFlags;
  // TODO: add target
  // TODO: optimize
  // TODO: warnings
  // TODO: debugSymbols
  String includes;
  String libs;
  StringVector sources;
} Executable;

typedef struct {
  char *output;
  char *flags;
  char *linkerFlags;
  char *includes;
  char *libs;
} ExecutableOptions;

void CreateConfig(MateOptions options);
void StartBuild();
void reBuild();
void CreateExecutable(ExecutableOptions executableOptions);

#define AddLibraryPaths(...)                                                                                                                                                                                                                   \
  ({                                                                                                                                                                                                                                           \
    StringVector vector = {0};                                                                                                                                                                                                                 \
    StringVectorPushMany(vector, __VA_ARGS__);                                                                                                                                                                                                 \
    addLibraryPaths(&vector);                                                                                                                                                                                                                  \
  })
static void addLibraryPaths(StringVector *vector);

#define AddIncludePaths(...)                                                                                                                                                                                                                   \
  ({                                                                                                                                                                                                                                           \
    StringVector vector = {0};                                                                                                                                                                                                                 \
    StringVectorPushMany(vector, __VA_ARGS__);                                                                                                                                                                                                 \
    addIncludePaths(&vector);                                                                                                                                                                                                                  \
  })
static void addIncludePaths(StringVector *vector);

#define LinkSystemLibraries(...)                                                                                                                                                                                                               \
  ({                                                                                                                                                                                                                                           \
    StringVector vector = {0};                                                                                                                                                                                                                 \
    StringVectorPushMany(vector, __VA_ARGS__);                                                                                                                                                                                                 \
    linkSystemLibraries(&vector);                                                                                                                                                                                                              \
  })
static void linkSystemLibraries(StringVector *vector);

#define AddFile(source) addFile(S(source));
static void addFile(String source);
String InstallExecutable();
i32 RunCommand(String command);
void EndBuild();

static bool needRebuild();
static void setDefaultState();

// NOTE: Here goes MATE_IMPLEMENTATION
// --- MATE.H END ---
