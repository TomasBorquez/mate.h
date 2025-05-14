#include "api.h"

static MateConfig state = {0};
static Executable executable = {0};

static const String MSVCStr = {.data = "cl.exe", .length = 6};
bool isMSVC() {
  return StrEq(state.compiler, MSVCStr);
}

static const String GCCStr = {.data = "gcc", .length = 3};
bool isGCC() {
  return StrEq(state.compiler, GCCStr);
}

static const String ClangStr = {.data = "clang", .length = 5};
bool isClang() {
  return StrEq(state.compiler, ClangStr);
}

static const String TCCStr = {.data = "tcc", .length = 3};
bool isTCC() {
  return StrEq(state.compiler, TCCStr);
}

bool isLinux() {
#if defined(PLATFORM_LINUX)
  return true;
#else
  return false;
#endif
}

bool isWindows() {
#if defined(PLATFORM_WIN)
  return true;
#else
  return false;
#endif
}

static String fixPathExe(String str) {
  String path = NormalizeExePath(state.arena, str);
#if defined(PLATFORM_WIN)
  return F(state.arena, "%s\\%s", GetCwd(), path.data);
#else
  return F(state.arena, "%s/%s", GetCwd(), path.data);
#endif
}

static String fixPath(String str) {
  String path = NormalizePath(state.arena, str);
#if defined(PLATFORM_WIN)
  return F(state.arena, "%s\\%s", GetCwd(), path.data);
#else
  return F(state.arena, "%s/%s", GetCwd(), path.data);
#endif
}

String ConvertNinjaPath(String str) {
#if defined(PLATFORM_WIN)
  String copy = StrNewSize(state.arena, str.data, str.length + 1);
  memmove(&copy.data[2], &copy.data[1], str.length - 1);
  copy.data[1] = '$';
  copy.data[2] = ':';
  return copy;
#else
  return str;
#endif
}

static void setDefaultState() {
  state.arena = ArenaCreate(20000 * sizeof(String));
  state.compiler = GetCompiler();

  state.mateExe = fixPathExe(S("./mate"));
  state.mateSource = fixPath(S("./mate.c"));
  state.buildDirectory = fixPath(S("./build"));
}

static MateConfig parseMateConfig(MateOptions options) {
  MateConfig result;
  result.mateExe = StrNew(state.arena, options.mateExe);
  result.compiler = StrNew(state.arena, options.compiler);
  result.mateSource = StrNew(state.arena, options.mateSource);
  result.buildDirectory = StrNew(state.arena, options.buildDirectory);
  return result;
}

void CreateConfig(MateOptions options) {
  setDefaultState();
  MateConfig config = parseMateConfig(options);

  if (!StrIsNull(config.mateExe)) {
    state.mateExe = fixPathExe(config.mateExe);
  }

  if (!StrIsNull(config.mateSource)) {
    state.mateSource = fixPath(config.mateSource);
  }

  if (!StrIsNull(config.buildDirectory)) {
    state.buildDirectory = fixPath(config.buildDirectory);
  }

  if (!StrIsNull(config.compiler)) {
    state.compiler = config.compiler;
  }

  state.initConfig = true;
}

static void readCache() {
  String mateCachePath = F(state.arena, "%s/mate-cache.ini", state.buildDirectory.data);
  errno_t err = IniParse(mateCachePath, &state.cache);
  Assert("MateReadCache: failed reading MateCache at %s, err: %d", mateCachePath.data, err);

  state.mateCache.lastBuild = IniGetLong(&state.cache, S("modify-time"));
  if (state.mateCache.lastBuild == 0) {
    state.mateCache.firstBuild = true;
    state.mateCache.lastBuild = TimeNow() / 1000;

    String modifyTime = F(state.arena, "%ld", state.mateCache.lastBuild);
    IniSet(&state.cache, S("modify-time"), modifyTime);
  }

#if defined(PLATFORM_WIN)
  if (state.mateCache.firstBuild) {
    errno_t ninjaCheck = RunCommand(S("ninja --version > nul 2> nul"));
    Assert(ninjaCheck == SUCCESS, "MateReadCache: Ninja build system not found. Please install Ninja and add it to your PATH.");
  }
#else
  state.mateCache.samuraiBuild = IniGetBool(&state.cache, S("samurai-build"));
  if (state.mateCache.samuraiBuild == false) {
    Assert(state.mateCache.firstBuild, "MateCache: This is not the first build and samurai is not compiled, could be a cache error, delete `./build` folder and rebuild `./mate.c`");

    String samuraiAmalgam = s(SAMURAI_AMALGAM);
    String sourcePath = F(state.arena, "%s/samurai.c", state.buildDirectory.data);
    errno_t errFileWrite = FileWrite(sourcePath, samuraiAmalgam);
    Assert(errFileWrite == SUCCESS, "MateReadCache: failed writing samurai source code to path %s", sourcePath.data);

    String outputPath = F(state.arena, "%s/samurai", state.buildDirectory.data);
    String compileCommand = F(state.arena, "%s \"%s\" -o \"%s\" -lrt -std=c99", state.compiler.data, sourcePath.data, outputPath.data);

    errno_t err = RunCommand(compileCommand);
    Assert(err == SUCCESS, "MateReadCache: Error meanwhile compiling samurai at %s, if you are seeing this please make an issue at github.com/TomasBorquez/mate.h", sourcePath.data);

    LogSuccess("Successfully compiled samurai");
    state.mateCache.samuraiBuild = true;
    IniSet(&state.cache, S("samurai-build"), S("true"));
  }
#endif

  err = IniWrite(mateCachePath, &state.cache);
  Assert(err == SUCCESS, "MateReadCache: Failed writing cache, err: %d", err);
}

void StartBuild() {
  LogInit();
  if (!state.initConfig) {
    setDefaultState();
  }

  state.initConfig = true;
  state.startTime = TimeNow();

  Mkdir(state.buildDirectory);
  readCache();
  reBuild();
}

static bool needRebuild() {
  File stats = {0};
  errno_t err = FileStats(state.mateSource, &stats);
  Assert(err == SUCCESS, "Aborting rebuild: Could not read fileStats for %s, error: %d", state.mateSource.data, err);

  if (stats.modifyTime <= state.mateCache.lastBuild) {
    return false;
  }

  String mateCachePath = F(state.arena, "%s/mate-cache.ini", state.buildDirectory.data);
  String modifyTime = F(state.arena, "%ld", stats.modifyTime);
  IniSet(&state.cache, S("modify-time"), modifyTime);

  err = IniWrite(mateCachePath, &state.cache);
  Assert(err == SUCCESS, "Aborting rebuild: Could not write cache for path %s, error: %d", mateCachePath.data, err);

  return true;
}

void reBuild() {
  if (state.mateCache.firstBuild || !needRebuild()) {
    return;
  }

  String mateExeNew = NormalizeExePath(state.arena, F(state.arena, "%s/mate-new", state.buildDirectory.data));
  String mateExeOld = NormalizeExePath(state.arena, F(state.arena, "%s/mate-old", state.buildDirectory.data));
  String mateExe = NormalizeExePath(state.arena, state.mateExe);

  String compileCommand;
  if (isMSVC()) {
    compileCommand = F(state.arena, "cl.exe \"%s\" /Fe:\"%s\"", state.mateSource.data, mateExeNew.data);
  } else {
    compileCommand = F(state.arena, "%s \"%s\" -o \"%s\"", state.compiler.data, state.mateSource.data, mateExeNew.data);
  }

  LogWarn("%s changed rebuilding...", state.mateSource.data);
  errno_t rebuildErr = RunCommand(compileCommand);
  Assert(rebuildErr == SUCCESS, "MateRebuild: failed command %s, err: %d", compileCommand.data, rebuildErr);

  errno_t renameErr = FileRename(mateExe, mateExeOld);
  Assert(renameErr == SUCCESS, "MateRebuild: failed renaming original executable failed, err: %d", renameErr);

  renameErr = FileRename(mateExeNew, mateExe);
  Assert(renameErr == SUCCESS, "MateRebuild: failed renaming new executable into old: %d", renameErr);

  LogInfo("Rebuild finished, running %s", mateExe.data);
  errno_t err = RunCommand(mateExe);
  exit(err);
}

void defaultExecutable() {
  String executableOutput = NormalizeExePath(state.arena, S("main"));
  executable.output = NormalizePath(state.arena, executableOutput);
  executable.libs = S("");
  executable.includes = S("");
  executable.linkerFlags = S("");
  executable.flags = S("");
}

static Executable parseExecutableOptions(ExecutableOptions options) {
  Executable result;
  result.output = StrNew(state.arena, options.output);
  result.flags = StrNew(state.arena, options.flags);
  result.linkerFlags = StrNew(state.arena, options.linkerFlags);
  result.includes = StrNew(state.arena, options.includes);
  result.libs = StrNew(state.arena, options.libs);
  return result;
}

void CreateExecutable(ExecutableOptions executableOptions) {
  Assert(state.initConfig,
         "CreateExecutable: before creating an executable you must use StartBuild(), like this: \n"
         "\n"
         "StartBuild()\n"
         "{\n"
         " // ...\n"
         "}\n"
         "EndBuild()");

  defaultExecutable();
  Executable options = parseExecutableOptions(executableOptions);
  if (!StrIsNull(options.output)) {
    String executableOutput = NormalizeExePath(state.arena, options.output);
    executable.output = NormalizePath(state.arena, executableOutput);
  }

  String flagsStr = options.flags;
  if (flagsStr.data == NULL) {
    flagsStr = S("");
  }

  if (executableOptions.warnings != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (executableOptions.warnings) {
    case FLAG_WARNINGS_MINIMAL:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-Wall");
      break;
    case FLAG_WARNINGS:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-Wall -Wextra");
      break;
    case FLAG_WARNINGS_VERBOSE:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-Wall -Wextra -Wpedantic");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (executableOptions.warnings) {
    case FLAG_WARNINGS_MINIMAL:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/W3");
      break;
    case FLAG_WARNINGS:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/W4");
      break;
    case FLAG_WARNINGS_VERBOSE:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/Wall");
      break;
    }
#endif
  }

  if (executableOptions.debug != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (executableOptions.debug) {
    case FLAG_DEBUG_MINIMAL:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-g1");
      break;
    case FLAG_DEBUG_MEDIUM:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-g2");
      break;
    case FLAG_DEBUG:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-g3");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (executableOptions.debug) {
    case FLAG_DEBUG_MINIMAL:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/Zi");
      break;
    case FLAG_DEBUG_MEDIUM:
    case FLAG_DEBUG:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/ZI");
      break;
    }
#endif
  }

  if (executableOptions.optimization != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (executableOptions.optimization) {
    case FLAG_OPTIMIZATION_NONE:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-O0");
      break;
    case FLAG_OPTIMIZATION_BASIC:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-O1");
      break;
    case FLAG_OPTIMIZATION:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-O2");
      break;
    case FLAG_OPTIMIZATION_SIZE:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-Os");
      break;
    case FLAG_OPTIMIZATION_AGGRESSIVE:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-O3");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (executableOptions.optimization) {
    case FLAG_OPTIMIZATION_NONE:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/Od");
      break;
    case FLAG_OPTIMIZATION_BASIC:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/O1");
      break;
    case FLAG_OPTIMIZATION:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/O2");
      break;
    case FLAG_OPTIMIZATION_SIZE:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/O1");
      break;
    case FLAG_OPTIMIZATION_AGGRESSIVE:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/Ox");
      break;
    }
#endif
  }

  if (executableOptions.std != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (executableOptions.std) {
    case FLAG_STD_C99:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-std=c99");
      break;
    case FLAG_STD_C11:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-std=c11");
      break;
    case FLAG_STD_C17:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-std=c17");
      break;
    case FLAG_STD_C23:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-std=c2x");
      break;
    case FLAG_STD_C2X:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "-std=c2x");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (executableOptions.std) {
    case FLAG_STD_C99:
    case FLAG_STD_C11:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/std:c11");
      break;
    case FLAG_STD_C17:
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/std:c17");
      break;
    case FLAG_STD_C23:
    case FLAG_STD_C2X:
      // NOTE: MSVC doesn't have C23 yet
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/std:clatest");
      break;
    }
#endif
  }

  if (!StrIsNull(flagsStr)) {
    executable.flags = flagsStr;
  }
  if (!StrIsNull(options.linkerFlags)) {
    executable.linkerFlags = options.linkerFlags;
  }
  if (!StrIsNull(options.includes)) {
    executable.includes = options.includes;
  }
  if (!StrIsNull(options.libs)) {
    executable.libs = options.libs;
  }
}

errno_t CreateCompileCommands() {
  FILE *ninjaPipe;
  FILE *outputFile;
  char buffer[4096];
  size_t bytes_read;

  String compileCommandsPath = NormalizePath(state.arena, F(state.arena, "%s/compile_commands.json", state.buildDirectory.data));
  errno_t err = fopen_s(&outputFile, compileCommandsPath.data, "w");
  if (err != SUCCESS || outputFile == NULL) {
    LogError("CreateCompileCommands: Failed to open file %s, err: %d", compileCommandsPath.data, err);
    return COMPILE_COMMANDS_FAILED_OPEN_FILE;
  }

  String compdbCommand;
  if (state.mateCache.samuraiBuild == true) {
    String samuraiOutputPath = F(state.arena, "%s/samurai", state.buildDirectory.data);
    compdbCommand = NormalizePath(state.arena, F(state.arena, "%s -f %s/build.ninja -t compdb", samuraiOutputPath.data, state.buildDirectory.data));
  }

  if (state.mateCache.samuraiBuild == false) {
    compdbCommand = NormalizePath(state.arena, F(state.arena, "ninja -f %s/build.ninja -t compdb", state.buildDirectory.data));
  }

  ninjaPipe = popen(compdbCommand.data, "r");
  if (ninjaPipe == NULL) {
    LogError("CreateCompileCommands: Failed to run compdb command, %s", compdbCommand.data);
    fclose(outputFile);
    return COMPILE_COMMANDS_FAILED_COMPDB;
  }

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), ninjaPipe)) > 0) {
    fwrite(buffer, 1, bytes_read, outputFile);
  }

  fclose(outputFile);
  errno_t status = pclose(ninjaPipe);
  if (status != SUCCESS) {
    LogError("CreateCompileCommands: Command failed with status %d\n", status);
    return COMPILE_COMMANDS_FAILED_COMPDB;
  }

  LogSuccess("Successfully created %s", compileCommandsPath.data);
  return SUCCESS;
}

static bool globMatch(String pattern, String text) {
  if (pattern.length == 1 && pattern.data[0] == '*') {
    return true;
  }

  size_t p = 0;
  size_t t = 0;
  size_t starP = -1;
  size_t starT = -1;
  while (t < text.length) {
    if (p < pattern.length && pattern.data[p] == text.data[t]) {
      p++;
      t++;
    } else if (p < pattern.length && pattern.data[p] == '*') {
      starP = p;
      starT = t;
      p++;
    } else if (starP != (size_t)-1) {
      p = starP + 1;
      t = ++starT;
    } else {
      return false;
    }
  }

  while (p < pattern.length && pattern.data[p] == '*') {
    p++;
  }

  return p == pattern.length;
}

static void addFile(String source) {
  Assert(!StrIsNull(executable.output), "AddFile: Before adding a file you must first CreateExecutable()");

  bool isGlob = false;
  for (size_t i = 0; i < source.length; i++) {
    if (source.data[i] == '*') {
      isGlob = true;
      break;
    }
  }

  Assert(source.length > 2 && source.data[0] == '.' && source.data[1] == '/',
         "AddFile: failed to a add file, to add a file it has to "
         "contain the relative path, for example AddFile(\"./main.c\")");

  Assert(source.data[source.length - 1] != '/',
         "AddFile: failed to add a file, you can't add a slash at the end of a path.\n"
         "For example, valid: AddFile(\"./main.c\"), invalid: AddFile(\"./main.c/\")");

  if (!isGlob) {
    VecPush(executable.sources, source);
    return;
  }

  String directory = {0};
  i32 lastSlash = -1;
  for (size_t i = 0; i < source.length; i++) {
    if (source.data[i] == '/') {
      lastSlash = i;
    }
  }

  directory = StrSlice(state.arena, source, 0, lastSlash);
  String pattern = StrSlice(state.arena, source, lastSlash + 1, source.length);

  StringVector files = ListDir(state.arena, directory);
  for (size_t i = 0; i < files.length; i++) {
    String file = VecAt(files, i);

    if (globMatch(pattern, file)) {
      String finalSource = F(state.arena, "%s/%s", directory.data, file.data);
      VecPush(executable.sources, finalSource);
    }
  }
}

static bool removeFile(String source) {
  for (size_t i = 0; i < executable.sources.length; i++) {
    String *currValue = VecAtPtr(executable.sources, i);
    if (StrEq(source, *currValue)) {
      currValue->data = NULL;
      currValue->length = 0;
      return true;
    }
  }
  return false;
}

// TODO: Create something like NormalizeOutput
static StringVector outputTransformer(StringVector vector) {
  StringVector result = {0};

  if (isMSVC()) {
    for (size_t i = 0; i < vector.length; i++) {
      String currentExecutable = VecAt(vector, i);
      if (StrIsNull(currentExecutable)) continue;
      size_t lastCharIndex = 0;
      for (size_t j = currentExecutable.length - 1; j > 0; j--) {
        char currentChar = currentExecutable.data[j];
        if (currentChar == '/') {
          lastCharIndex = j;
          break;
        }
      }

      Assert(lastCharIndex != 0, "MateOutputTransformer: failed to transform %s, to an object file", currentExecutable.data);
      char *filenameStart = currentExecutable.data + lastCharIndex + 1;
      size_t filenameLength = currentExecutable.length - (lastCharIndex + 1);

      String objOutput = StrNewSize(state.arena, filenameStart, filenameLength + 2);
      objOutput.data[objOutput.length - 3] = 'o';
      objOutput.data[objOutput.length - 2] = 'b';
      objOutput.data[objOutput.length - 1] = 'j';

      VecPush(result, objOutput);
    }
    return result;
  }

  for (size_t i = 0; i < vector.length; i++) {
    String currentExecutable = VecAt(vector, i);
    if (StrIsNull(currentExecutable)) continue;
    size_t lastCharIndex = 0;
    for (size_t j = currentExecutable.length - 1; j > 0; j--) {
      char currentChar = currentExecutable.data[j];
      if (currentChar == '/') {
        lastCharIndex = j;
        break;
      }
    }
    Assert(lastCharIndex != 0, "MateOutputTransformer: failed to transform %s, to an object file", currentExecutable.data);
    String output = StrSlice(state.arena, currentExecutable, lastCharIndex + 1, currentExecutable.length);
    output.data[output.length - 1] = 'o';
    VecPush(result, output);
  }
  return result;
}

String InstallExecutable() {
  Assert(executable.sources.length != 0, "InstallExecutable: Executable has zero sources, add at least one with AddFile(\"./main.c\")");
  Assert(!StrIsNull(executable.output), "InstallExecutable: Before installing executable you must first CreateExecutable()");

  StringBuilder builder = StringBuilderReserve(state.arena, 1024);

  // Compiler
  StringBuilderAppend(state.arena, &builder, &S("cc = "));
  StringBuilderAppend(state.arena, &builder, &state.compiler);
  StringBuilderAppend(state.arena, &builder, &S("\n"));

  // Linker flags
  if (executable.linkerFlags.length > 0) {
    StringBuilderAppend(state.arena, &builder, &S("linker_flags = "));
    StringBuilderAppend(state.arena, &builder, &executable.linkerFlags);
    StringBuilderAppend(state.arena, &builder, &S("\n"));
  }

  // Compiler flags
  if (executable.flags.length > 0) {
    StringBuilderAppend(state.arena, &builder, &S("flags = "));
    StringBuilderAppend(state.arena, &builder, &executable.flags);
    StringBuilderAppend(state.arena, &builder, &S("\n"));
  }

  // Include paths
  if (executable.includes.length > 0) {
    StringBuilderAppend(state.arena, &builder, &S("includes = "));
    StringBuilderAppend(state.arena, &builder, &executable.includes);
    StringBuilderAppend(state.arena, &builder, &S("\n"));
  }

  // Libraries
  if (executable.libs.length > 0) {
    StringBuilderAppend(state.arena, &builder, &S("libs = "));
    StringBuilderAppend(state.arena, &builder, &executable.libs);
    StringBuilderAppend(state.arena, &builder, &S("\n"));
  }

  // Current working directory
  String cwd_path = ConvertNinjaPath(s(GetCwd()));
  StringBuilderAppend(state.arena, &builder, &S("cwd = "));
  StringBuilderAppend(state.arena, &builder, &cwd_path);
  StringBuilderAppend(state.arena, &builder, &S("\n"));

  // Build directory
  String build_dir_path = ConvertNinjaPath(state.buildDirectory);
  StringBuilderAppend(state.arena, &builder, &S("builddir = "));
  StringBuilderAppend(state.arena, &builder, &build_dir_path);
  StringBuilderAppend(state.arena, &builder, &S("\n"));

  // Target
  StringBuilderAppend(state.arena, &builder, &S("target = $builddir/"));
  StringBuilderAppend(state.arena, &builder, &executable.output);
  StringBuilderAppend(state.arena, &builder, &S("\n\n"));

  // Link command
  StringBuilderAppend(state.arena, &builder, &S("rule link\n  command = $cc"));
  if (executable.flags.length > 0) {
    StringBuilderAppend(state.arena, &builder, &S(" $flags"));
  }

  if (executable.linkerFlags.length > 0) {
    StringBuilderAppend(state.arena, &builder, &S(" $linker_flags"));
  }

  if (isMSVC()) {
    StringBuilderAppend(state.arena, &builder, &S(" /Fe:$out $in"));
  } else {
    StringBuilderAppend(state.arena, &builder, &S(" -o $out $in"));
  }

  if (executable.libs.length > 0) {
    StringBuilderAppend(state.arena, &builder, &S(" $libs"));
  }
  StringBuilderAppend(state.arena, &builder, &S("\n\n"));

  // Compile command
  StringBuilderAppend(state.arena, &builder, &S("rule compile\n  command = $cc"));
  if (executable.flags.length > 0) {
    StringBuilderAppend(state.arena, &builder, &S(" $flags"));
  }
  if (executable.includes.length > 0) {
    StringBuilderAppend(state.arena, &builder, &S(" $includes"));
  }

  if (isMSVC()) {
    StringBuilderAppend(state.arena, &builder, &S(" /c $in /Fo:$out\n\n"));
  } else {
    StringBuilderAppend(state.arena, &builder, &S(" -c $in -o $out\n\n"));
  }

  // Build individual source files
  StringVector outputFiles = outputTransformer(executable.sources);
  StringBuilder outputBuilder = StringBuilderCreate(state.arena);

  for (size_t i = 0; i < executable.sources.length; i++) {
    String currSource = VecAt(executable.sources, i);
    if (StrIsNull(currSource)) continue;

    String outputFile = VecAt(outputFiles, i);
    String sourceFile = NormalizePathStart(state.arena, currSource);

    // Source build command
    StringBuilderAppend(state.arena, &builder, &S("build $builddir/"));
    StringBuilderAppend(state.arena, &builder, &outputFile);
    StringBuilderAppend(state.arena, &builder, &S(": compile $cwd/"));
    StringBuilderAppend(state.arena, &builder, &sourceFile);
    StringBuilderAppend(state.arena, &builder, &S("\n"));

    // Add to output files list
    if (outputBuilder.buffer.length == 0) {
      StringBuilderAppend(state.arena, &outputBuilder, &S("$builddir/"));
      StringBuilderAppend(state.arena, &outputBuilder, &outputFile);
    } else {
      StringBuilderAppend(state.arena, &outputBuilder, &S(" $builddir/"));
      StringBuilderAppend(state.arena, &outputBuilder, &outputFile);
    }
  }

  // Build target
  StringBuilderAppend(state.arena, &builder, &S("build $target: link "));
  StringBuilderAppend(state.arena, &builder, &outputBuilder.buffer);
  StringBuilderAppend(state.arena, &builder, &S("\n\n"));

  // Default target
  StringBuilderAppend(state.arena, &builder, &S("default $target\n"));

  String buildNinjaPath;
  buildNinjaPath = F(state.arena, "%s/build.ninja", state.buildDirectory.data);

  errno_t errWrite = FileWrite(buildNinjaPath, builder.buffer);
  Assert(errWrite == SUCCESS, "InstallExecutable: failed to write build.ninja for %s, err: %d", buildNinjaPath.data, errWrite);

  i64 err;
  String buildCommand;
  if (state.mateCache.samuraiBuild) {
    String samuraiOutputPath = F(state.arena, "%s/samurai", state.buildDirectory.data);
    buildCommand = F(state.arena, "%s -f %s", samuraiOutputPath.data, buildNinjaPath.data);
  } else {
    buildCommand = F(state.arena, "ninja -f %s", buildNinjaPath.data);
  }

  err = RunCommand(buildCommand);
  Assert(err == SUCCESS, "InstallExecutable: Ninja file compilation failed with code: %lu", err);

  LogSuccess("Ninja file compilation done");
  state.totalTime = TimeNow() - state.startTime;

#if defined(PLATFORM_WIN)
  return F(state.arena, "%s\\%s", state.buildDirectory.data, executable.output.data);
#else
  return F(state.arena, "%s/%s", state.buildDirectory.data, executable.output.data);
#endif
}

errno_t RunCommand(String command) {
  return system(command.data);
}

static void addLibraryPaths(StringVector *vector) {
  StringBuilder builder = StringBuilderCreate(state.arena);

  if (isMSVC() && executable.libs.length == 0) {
    StringBuilderAppend(state.arena, &builder, &S("/link"));
  }

  if (executable.libs.length) {
    StringBuilderAppend(state.arena, &builder, &executable.libs);
  }

  if (isMSVC()) {
    // MSVC format: /LIBPATH:"path"
    for (size_t i = 0; i < vector->length; i++) {
      String currLib = VecAt((*vector), i);
      String buffer = F(state.arena, " /LIBPATH:\"%s\"", currLib.data);
      StringBuilderAppend(state.arena, &builder, &buffer);
    }
  } else {
    // GCC/Clang format: -L"path"
    for (size_t i = 0; i < vector->length; i++) {
      String currLib = VecAt((*vector), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(state.arena, "-L\"%s\"", currLib.data);
        StringBuilderAppend(state.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(state.arena, " -L\"%s\"", currLib.data);
      StringBuilderAppend(state.arena, &builder, &buffer);
    }
  }

  executable.libs = builder.buffer;
}

static void linkSystemLibraries(StringVector *vector) {
  StringBuilder builder = StringBuilderCreate(state.arena);

  if (isMSVC() && executable.libs.length == 0) {
    StringBuilderAppend(state.arena, &builder, &S("/link"));
  }

  if (executable.libs.length) {
    StringBuilderAppend(state.arena, &builder, &executable.libs);
  }

  if (isMSVC()) {
    // MSVC format: library.lib
    for (size_t i = 0; i < vector->length; i++) {
      String currLib = VecAt((*vector), i);
      String buffer = F(state.arena, " %s.lib", currLib.data);
      StringBuilderAppend(state.arena, &builder, &buffer);
    }
  } else {
    // GCC/Clang format: -llib
    for (size_t i = 0; i < vector->length; i++) {
      String currLib = VecAt((*vector), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(state.arena, "-l%s", currLib.data);
        StringBuilderAppend(state.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(state.arena, " -l%s", currLib.data);
      StringBuilderAppend(state.arena, &builder, &buffer);
    }
  }

  executable.libs = builder.buffer;
}

static void addIncludePaths(StringVector *vector) {
  StringBuilder builder = StringBuilderCreate(state.arena);

  if (executable.includes.length) {
    StringBuilderAppend(state.arena, &builder, &executable.includes);
    StringBuilderAppend(state.arena, &builder, &S(" "));
  }

  if (isMSVC()) {
    // MSVC format: /I"path"
    for (size_t i = 0; i < vector->length; i++) {
      String currInclude = VecAt((*vector), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(state.arena, "/I\"%s\"", currInclude.data);
        StringBuilderAppend(state.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(state.arena, " /I\"%s\"", currInclude.data);
      StringBuilderAppend(state.arena, &builder, &buffer);
    }
  } else {
    // GCC/Clang format: -I"path"
    for (size_t i = 0; i < vector->length; i++) {
      String currInclude = VecAt((*vector), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(state.arena, "-I\"%s\"", currInclude.data);
        StringBuilderAppend(state.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(state.arena, " -I\"%s\"", currInclude.data);
      StringBuilderAppend(state.arena, &builder, &buffer);
    }
  }

  executable.includes = builder.buffer;
}

void EndBuild() {
  LogInfo("Build took: %ldms", state.totalTime);
  ArenaFree(state.arena);
}
