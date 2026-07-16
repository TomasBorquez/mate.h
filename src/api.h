/* MIT License

  mate.h - A single-header library for compiling your C code in C
  Version - 2026-07-16 (0.3.1):
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
typedef enum {
  ARCH_X64 = 1,
  ARCH_X86,
  ARCH_ARM64,
  ARCH_ARM32,
  ARCH_RISCV64,
  ARCH_PPC64,
  ARCH_S390X,
  ARCH_WASM32,
} Arch;

typedef struct {
  OS os;
  Arch arch;
  char *compiler;
  char *ar;
  CompilerFamily compilerFamily;
} Target;

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
  FLAG_SANITIZER_ADDRESS = 1, // fsanitize=address
  FLAG_SANITIZER_UB,          // fsanitize=undefined
  FLAG_SANITIZER,             // fsanitize=address,undefined
} FlagSanitizer;

typedef enum {
  FLAG_ERROR,     // -fdiagnostics-color=always
  FLAG_ERROR_MAX  // -fdiagnostics-color=always -fcolor-diagnostics ...
} FlagErrorFormat;

typedef struct {
/* required: */
  char *output; // NOTE: adds extension automatically

/* optional: */
  Target target;
  char *flags;
  char *libs;
  char *includes;
  char *linkerFlags;

  FlagWarnings warnings;
  FlagDebug debug;
  FlagOptimization optimization;
  FlagSTD std;
  FlagSanitizer sanitizer;
  FlagErrorFormat error;
} ExecutableOptions;

typedef struct {
/* required: */
  char *output; // NOTE: adds "lib" prefix and extension automatically

/* optional: */
  Target target;
  char *flags;
  char *libs;
  char *includes;
  char *arFlags;

  FlagWarnings warnings;
  FlagDebug debug;
  FlagOptimization optimization;
  FlagSTD std;
  FlagSanitizer sanitizer;
  FlagErrorFormat error;
} StaticLibOptions;

typedef struct {
/* required: */
  char *output; // NOTE: adds "lib" prefix and extension automatically

/* optional: */
  Target target;
  char *flags;
  char *libs;
  char *includes;
  char *linkerFlags;

  FlagWarnings warnings;
  FlagDebug debug;
  FlagOptimization optimization;
  FlagSTD std;
  FlagSanitizer sanitizer;
  FlagErrorFormat error;
} SharedLibOptions;

typedef struct {
  Target scriptCompiler;
  char *buildDirectory;
  char *rebuildFlags;
} MateOptions;

typedef struct {
  uint64_t last_build;
  bool samurai_build;
  bool first_build;
} MateCache;

typedef struct {
  Target script_compiler;

  String build_directory;
  String mate_source;
  String mate_exe;
  String rebuild_flags;

  String cwd;
  MateCache mate_cache;
  IniFile cache;

  Arena *arena;
  bool init_config;

  int64_t start_time;
  int64_t total_time;
} MateConfig;

typedef struct {
  String output;
  String outputPath;
  String ninjaBuildPath;

  Target target;
  String flags;
  String libs;
  String includes;
  String linkerFlags;

  bool installed;

  StringVector sources;
  StringVector staticLibOutputs;
  StringVector sharedLibOutputs;
} Executable;

typedef struct {
  String output;
  String outputPath;
  String ninjaBuildPath;

  Target target;
  String flags;
  String libs;
  String includes;
  String arFlags;

  bool installed;

  StringVector sources;
} StaticLib;

typedef struct {
  String output;
  String outputPath;
  String ninjaBuildPath;

  Target target;
  String flags;
  String libs;
  String includes;
  String linkerFlags;

  bool installed;

  StringVector sources;
  StringVector staticLibOutputs;
  StringVector sharedLibOutputs;
} SharedLib;

typedef enum { NONE = 0, NEEDED, WEAK } LinkFrameworkOptions;

typedef StringBuilder FlagBuilder;

/* --- Build System --- */
void CreateConfig(MateOptions options);

void StartBuild(void);
void StartBuildEx(int argc, char **argv);
void EndBuild(void);

Executable CreateExecutable(ExecutableOptions opts);
#define InstallExecutable(_target) mate_install_executable(&(_target))
static void mate_install_executable(Executable *executable);

StaticLib CreateStaticLib(StaticLibOptions opts);
#define InstallStaticLib(_target) mate_install_static_lib(&(_target))
static void mate_install_static_lib(StaticLib *static_lib);

SharedLib CreateSharedLib(SharedLibOptions opts);
#define InstallSharedLib(_target) mate_install_shared_lib(&(_target))
static void mate_install_shared_lib(SharedLib *shared_lib);

#define LinkStaticLib(_target, _static_lib) mate_link_static_lib(&(_target).staticLibOutputs, &(_static_lib));
#define LinkSharedLib(_target, _shared_lib) mate_link_shared_lib(&(_target).sharedLibOutputs, &(_shared_lib));
static void mate_link_static_lib(StringVector *static_lib_outputs, StaticLib *static_lib);
static void mate_link_shared_lib(StringVector *shared_lib_outputs, SharedLib *shared_lib);

typedef enum { COMPILE_COMMANDS_SUCCESS = 0, COMPILE_COMMANDS_FAILED_OPEN_FILE = 1000, COMPILE_COMMANDS_FAILED_COMPDB } CreateCompileCommandsError;
#define CreateCompileCommands(_target) mate_create_compile_commands((_target).ninjaBuildPath);
static WARN_UNUSED CreateCompileCommandsError mate_create_compile_commands(String ninja_build_path);

#define AddLibraryPaths(_target, ...)                                                \
  do {                                                                              \
    char *_libs[] = {__VA_ARGS__};                                                  \
    mate_add_library_paths((_target).target, &(_target).libs, _libs, ARR_LEN(_libs)); \
  } while (0)
static void mate_add_library_paths(Target t, String *targetLibs, char **libs, size_t libs_size);

#define LinkSystemLibraries(_target, ...)                                                \
  do {                                                                                  \
    char *_libs[] = {__VA_ARGS__};                                                      \
    mate_link_system_libraries((_target).target, &(_target).libs, _libs, ARR_LEN(_libs)); \
  } while (0)
static void mate_link_system_libraries(Target t, String *targetLibs, char **libs, size_t libs_size);

#define LinkFrameworks(_target, ...)                                                           \
  do {                                                                                        \
    char *_frameworks[] = {__VA_ARGS__};                                                      \
    mate_link_frameworks((_target).target, &(_target).libs, _frameworks, ARR_LEN(_frameworks)); \
  } while (0)
static void mate_link_frameworks(Target t, String *targetLibs, char **frameworks, size_t frameworks_size);

#define LinkFrameworksWithOptions(_target, _options, ...)                                                             \
  do {                                                                                                              \
    char *_frameworks[] = {__VA_ARGS__};                                                                            \
    mate_link_frameworks_with_options((_target).target, &(_target).libs, _options, _frameworks, ARR_LEN(_frameworks)); \
  } while (0)
static void mate_link_frameworks_with_options(Target t, String *targetLibs, LinkFrameworkOptions options, char **frameworks, size_t frameworks_size);

#define AddIncludePaths(_target, ...)                                                            \
  do {                                                                                          \
    char *_includes[] = {__VA_ARGS__};                                                          \
    mate_add_include_paths((_target).target, &(_target).includes, _includes, ARR_LEN(_includes)); \
  } while (0)
static void mate_add_include_paths(Target t, String *targetIncludes, char **includes, size_t includes_size);

#define AddFrameworkPaths(_target, ...)                                                                \
  do {                                                                                                \
    char *_frameworks[] = {__VA_ARGS__};                                                              \
    mate_add_framework_paths((_target).target, &(_target).includes, _frameworks, ARR_LEN(_frameworks)); \
  } while (0)
static void mate_add_framework_paths(Target t, String *targetIncludes, char **includes, size_t includes_size);

#define AddFile(_target, ...)                                    \
  do {                                                          \
    char *_files[] = {__VA_ARGS__};                             \
    mate_add_files(&(_target).sources, _files, ARR_LEN(_files)); \
  } while (0);
static void mate_add_file(StringVector *sources, String source);
static void mate_add_files(StringVector *sources, char **source, size_t size);

#define RemoveFile(_target, _source) mate_remove_file(&(_target).sources, s(_source));
static bool mate_remove_file(StringVector *sources, String source);

/* --- Flag Builder --- */
StringBuilder FlagBuilderCreate(void);
FlagBuilder FlagBuilderReserve(size_t count);

#define FlagBuilderAdd(_t, _builder, ...) mate_flag_builder_add_list(_t, _builder, (char *[]){__VA_ARGS__, NULL})
static void mate_flag_builder_add_string(Target t, FlagBuilder *builder, char *flag);
static void mate_flag_builder_add_list(Target t, FlagBuilder *fb, char **flags);

/* --- Path Utils --- */
static String mate_path_with_platform_ext(Target t, Arena *arena, String path, String unix_ext, String win_ext, String macos_ext);

String PathJoin(String base, String tail);
String PathStem(String path);

String NormPath(String path);
String NormPathStart(String path);
String NormPathEnd(String path) ;

String NormPathExe(Target t, String str);
String NormPathStaticLib(Target t, String str);
String NormPathSharedLib(Target t, String str);
String NormPathNinja(String str);
String NormPathOutput(Target t, String str);

String AbsoluteNormPath(String str);
String AbsoluteNormPathExe(Target t, String str);
String AbsoluteNormPathStaticLib(Target t, String str);

/* --- Utils --- */
WARN_UNUSED errno_t RunCommand(String command);
#define RunCommandF(_format, ...) RunCommand(F(mate_state.arena, _format, __VA_ARGS__))

char *GetAr(Target t);
char *GetScriptCompiler(void);

Target HostTarget(void);
Target CreateTarget(Target t);
bool isTargetSet(Target t);
bool isTargetHost(Target t);

bool isLinux(Target t);
bool isMacOS(Target t);
bool isWindows(Target t);
bool isAndroid(Target t);
bool isEmscripten(Target t);
bool isFreeBSD(Target t);

bool isGCC(Target t);
bool isClang(Target t);
bool isTCC(Target t);
bool isMSVC(Target t);

#define SAMURAI_AMALGAM "SAMURAI SOURCE"

// --- MATE.H END ---
