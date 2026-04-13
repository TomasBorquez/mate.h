/* MIT License

  mate.h - A single-header library for compiling your C code in C
  Version - 2026-04-12 (0.2.3):
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
  i64 last_build;
  bool samurai_build;
  bool first_build;
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
  String ninjaBuildPath;

  String flags;
  String arFlags;
  String libs;
  String includes;
  StringVector sources;
} StaticLib;

typedef struct {
  String output;
  String outputPath;
  String ninjaBuildPath;

  String flags;
  String linkerFlags;
  String libs;
  String includes;
  StringVector sources;
} Executable;

typedef struct {
  Compiler compiler;

  // Paths
  String build_directory;
  String mate_source;
  String mate_exe;

  // Cache
  MateCache mate_cache;
  IniFile cache;

  // Misc
  Arena *arena;
  bool init_config;

  i64 start_time;
  i64 total_time;
} MateConfig;

typedef enum {
  FLAG_WARNINGS_NONE = 1, // -w
  FLAG_WARNINGS_MINIMAL,  // -Wall
  FLAG_WARNINGS,          // -Wall -Wextra
  FLAG_WARNINGS_VERBOSE,  // -Wall -Wextra -Wpedantic
} FlagWarnings;

typedef enum {
  FLAG_DEBUG_MINIMAL = 1, // -g1
  FLAG_DEBUG_MEDIUM,      // -g/g2
  FLAG_DEBUG,             // -g3
} FlagDebug;

typedef enum {
  FLAG_OPTIMIZATION_NONE = 1,  // -O0
  FLAG_OPTIMIZATION_BASIC,     // -O1
  FLAG_OPTIMIZATION,           // -O2
  FLAG_OPTIMIZATION_SIZE,      // -Os
  FLAG_OPTIMIZATION_AGGRESSIVE // -O3
} FlagOptimization;

typedef enum {
  FLAG_STD_C99 = 1, // -std=c99
  FLAG_STD_C11,     // -std=c11
  FLAG_STD_C17,     // -std=c17
  FLAG_STD_C23,     // -std=c23
  FLAG_STD_C2X      // -std=c2x
} FlagSTD;

typedef enum {
  FLAG_ERROR = 0, // -fdiagnostics-color=always
  FLAG_ERROR_MAX  // -fdiagnostics-color=always -fcolor-diagnostics ...
} FlagErrorFormat;

typedef struct {
  char *output; // WARNING: Required
  char *flags;
  char *linker_flags;
  char *includes;
  char *libs;

  // NOTE: Flag options
  FlagSTD std;
  FlagDebug debug;
  FlagWarnings warnings;
  FlagErrorFormat error;
  FlagOptimization optimization;
} ExecutableOptions;

typedef struct {
  char *output; // WARNING: Required
  char *flags;
  char *arFlags;
  char *includes;
  char *libs;

  // NOTE: Flag options
  FlagSTD std;
  FlagDebug debug;
  FlagWarnings warnings;
  FlagErrorFormat error;
  FlagOptimization optimization;
} StaticLibOptions;

typedef enum { NONE = 0, NEEDED, WEAK } LinkFrameworkOptions;

typedef StringBuilder FlagBuilder;

/* --- Build System --- */
void CreateConfig(MateOptions options);

void StartBuild(void);
void EndBuild(void);

Executable CreateExecutable(ExecutableOptions executableOptions);
#define InstallExecutable(target) mate_install_executable(&(target))
static void mate_install_executable(Executable *executable);

StaticLib CreateStaticLib(StaticLibOptions staticLibOptions);
#define InstallStaticLib(target) mate_install_static_lib(&(target))
static void mate_install_static_lib(StaticLib *staticLib);

typedef enum { COMPILE_COMMANDS_SUCCESS = 0, COMPILE_COMMANDS_FAILED_OPEN_FILE = 1000, COMPILE_COMMANDS_FAILED_COMPDB } CreateCompileCommandsError;
#define CreateCompileCommands(target) mate_create_compile_commands((target).ninjaBuildPath);
static WARN_UNUSED CreateCompileCommandsError mate_create_compile_commands(String ninjaBuildPath);

#define AddLibraryPaths(target, ...)             \
  do {                                           \
    StringVector _libs = {0};                    \
    StringVectorPushMany(_libs, __VA_ARGS__);    \
    mate_add_library_paths(&(target).libs, &_libs); \
                                                 \
    /* Cleanup */                                \
    VecFree(_libs);                              \
  } while (0)
static void mate_add_library_paths(String *targetLibs, StringVector *libs);

#define LinkSystemLibraries(target, ...)             \
  do {                                               \
    StringVector _libs = {0};                        \
    StringVectorPushMany(_libs, __VA_ARGS__);        \
    mate_link_system_libraries(&(target).libs, &_libs); \
                                                     \
    /* Cleanup */                                    \
    VecFree(_libs);                                  \
  } while (0)
static void mate_link_system_libraries(String *targetLibs, StringVector *libs);

#define LinkFrameworks(target, ...)                        \
  do {                                                     \
    StringVector _frameworks = {0};                        \
    StringVectorPushMany(_frameworks, __VA_ARGS__);        \
    /* Frameworks are just specialized library bundles. */ \
    mate_link_frameworks(&(target).libs, &_frameworks);      \
                                                           \
    /* Cleanup */                                          \
    VecFree(_frameworks);                                  \
  } while (0)
static void mate_link_frameworks(String *targetLibs, StringVector *frameworks);

#define LinkFrameworksWithOptions(target, options, ...)                              \
  do {                                                                    \
    StringVector _frameworks = {0};                                       \
    StringVectorPushMany(_frameworks, __VA_ARGS__);                       \
    /* Frameworks are just specialized library bundles. */                \
    mate_link_frameworks_with_options(&(target).libs, options, &_frameworks); \
                                                                          \
    /* Cleanup */                                                         \
    VecFree(_frameworks);                                                 \
  } while (0)
static void mate_link_frameworks_with_options(String *targetLibs, LinkFrameworkOptions options, StringVector *frameworks);

#define AddIncludePaths(target, ...)                     \
  do {                                                   \
    StringVector _includes = {0};                        \
    StringVectorPushMany(_includes, __VA_ARGS__);        \
    mate_add_include_paths(&(target).includes, &_includes); \
                                                         \
    /* Cleanup */                                        \
    VecFree(_includes);                                  \
  } while (0)
static void mate_add_include_paths(String *targetIncludes, StringVector *vector);

#define AddFrameworkPaths(target, ...)                       \
  do {                                                       \
    StringVector _frameworks = {0};                          \
    StringVectorPushMany(_frameworks, __VA_ARGS__);          \
    /* Frameworks are just specialized library bundles. */   \
    mate_add_framework_paths(&(target).includes, &_frameworks); \
                                                             \
    /* Cleanup */                                            \
    VecFree(_frameworks);                                    \
  } while (0)
static void mate_add_framework_paths(String *targetIncludes, StringVector *includes);

#define AddFile(target, source) mate_add_file(&(target).sources, s(source));
static void mate_add_file(StringVector *sources, String source);

#define AddFiles(target, files)                                                                           \
  do {                                                                                                    \
    Assert(sizeof(files) != sizeof(*(files)), "AddFiles: failed, files must be an array, not a pointer"); \
    mate_add_files(&(target).sources, files, sizeof(files) / sizeof(*(files)));                             \
  } while (0);
static void mate_add_files(StringVector *sources, char **source, size_t size);

#define RemoveFile(target, source) mate_remove_file(&(target).sources, s(source));
static bool mate_remove_file(StringVector *sources, String source);

/* --- Flag Builder --- */
StringBuilder FlagBuilderCreate(void);
FlagBuilder FlagBuilderReserve(size_t count);

#define FlagBuilderAdd(builder, ...)           \
  do {                                         \
    StringVector _flags = {0};                 \
    StringVectorPushMany(_flags, __VA_ARGS__); \
    mate_flag_builder_add(builder, _flags);       \
                                               \
    /* Cleanup */                              \
    VecFree(_flags);                           \
  } while (0)

/* --- Path Utils --- */
static void mate_flag_builder_add(FlagBuilder *builder, StringVector flags);
static void mate_flag_builder_add_single(FlagBuilder *builder, char *flag);
static void mate_flag_builder_add_string(FlagBuilder *builder, String *flag);
static void mate_flag_builder_add_list(FlagBuilder *fb, char **flags);

static bool mate_is_valid_executable(String *exePath);

static String mate_fix_path(String str);
static String mate_fix_path_exe(String str);
static String mate_convert_ninja_path(String str);

static StringVector mate_output_transformer(StringVector vector);
static bool mate_global_match(String pattern, String text);

/* --- Utils --- */
WARN_UNUSED errno_t RunCommand(String command);
#define RunCommandF(format, ...) RunCommand(F(mate_state.arena, format, __VA_ARGS__))

String CompilerToStr(Compiler compiler);

bool isMSVC(void);
bool isGCC(void);
bool isClang(void);
bool isTCC(void);

#define SAMURAI_AMALGAM "SAMURAI SOURCE"

// --- MATE.H END ---
