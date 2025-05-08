#include "api.h"

static MateConfig state = {0};
static Executable executable = {0};

String FixPathExe(String str) {
  String path = ConvertPath(state.arena, ConvertExe(state.arena, str));
#if defined(PLATFORM_WIN)
  return F(state.arena, "%s\\%s", GetCwd(), path.data);
#else
  return F(state.arena, "%s/%s", GetCwd(), path.data);
#endif
}

String FixPath(String str) {
  String path = ConvertPath(state.arena, str);
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
  state.mateExe = FixPath(S("./mate"));
  state.compiler = GetCompiler();
  state.mateSource = FixPath(S("./mate.c"));
  state.buildDirectory = FixPath(S("./build"));
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

  if (!StrIsNull(&config.mateSource)) {
    state.mateSource = FixPath(config.mateSource);
  }

  if (!StrIsNull(&config.mateExe)) {
    state.mateExe = FixPath(config.mateExe);
  }

  if (!StrIsNull(&config.buildDirectory)) {
    state.buildDirectory = FixPath(config.buildDirectory);
  }

  if (!StrIsNull(&config.compiler)) {
    state.compiler = config.compiler;
  }

  state.customConfig = true;
}

static void readCache() {
  String mateCachePath = F(state.arena, "%s/mate-cache.ini", state.buildDirectory.data);
  state.cache = IniParse(&mateCachePath);

  i32 modifyTime = IniGetInt(&state.cache, &S("modify-time"));
  if (modifyTime == 0) {
    state.firstBuild = true;
    state.mateCache.lastBuild = TimeNow() / 1000;
    String modifyTime = F(state.arena, "%d", state.mateCache.lastBuild);
    IniSet(&state.cache, S("modify-time"), modifyTime);
  } else {
    state.mateCache.lastBuild = modifyTime;
  }

  state.mateCache.samuraiBuild = IniGetBool(&state.cache, &S("samurai-build"));
#if !defined(PLATFORM_WIN)
  if (state.mateCache.samuraiBuild == false) {
    String samuraiAmalgam = s(SAMURAI_AMALGAM);
    String sourcePath = F(state.arena, "%s/samurai.c", state.buildDirectory.data);
    FileWrite(&sourcePath, &samuraiAmalgam);

    String compileCommand;
    String outputPath = F(state.arena, "%s/samurai", state.buildDirectory.data);
    if (StrEqual(&state.compiler, &S("gcc"))) {
      compileCommand = F(state.arena, "gcc \"%s\" -o \"%s\" -lrt -std=c99 -O2", sourcePath.data, outputPath.data);
    }
    if (StrEqual(&state.compiler, &S("clang"))) {
      compileCommand = F(state.arena, "clang \"%s\" -o \"%s\" -lrt -std=c99 -O2", sourcePath.data, outputPath.data);
    }
    if (StrEqual(&state.compiler, &S("tcc"))) {
      compileCommand = F(state.arena, "tcc \"%s\" -o \"%s\" -lrt -std=c99 -O2", sourcePath.data, outputPath.data);
    }
    errno_t err = RunCommand(compileCommand);
    if (err != SUCCESS) {
      LogError("Error meanwhile compiling samurai at %s, if you are seeing this please make an issue at github.com/TomasBorquez/mate.h", sourcePath.data);
      abort();
    }

    LogSuccess("Successfully compiled samurai");
    state.mateCache.samuraiBuild = true;
    IniSet(&state.cache, S("samurai-build"), S("true"));
  }
#endif

  IniWrite(&mateCachePath, &state.cache);
}

void StartBuild() {
  LogInit();
  if (!state.customConfig) {
    setDefaultState();
  }

  state.startTime = TimeNow();
  Mkdir(state.buildDirectory);
  readCache();
  reBuild();
}

static bool needRebuild() {
  File stats = {0};
  errno_t result = FileStats(&state.mateSource, &stats);
  if (result != SUCCESS) {
    LogError("Could not read fileStats %d", result);
    LogError("Aborting rebuild");
    abort();
  }

  if (stats.modifyTime > state.mateCache.lastBuild) {
    String mateCachePath = F(state.arena, "%s/mate-cache.ini", state.buildDirectory.data);
    String modifyTime = F(state.arena, "%llu", stats.modifyTime);
    IniSet(&state.cache, S("modify-time"), modifyTime);
    IniWrite(&mateCachePath, &state.cache);
    return true;
  }

  return false;
}

void reBuild() {
  if (state.firstBuild || !needRebuild()) {
    return;
  }

  String mateExeNew = ConvertExe(state.arena, F(state.arena, "%s/mate-new", state.buildDirectory.data));
  String mateExeOld = ConvertExe(state.arena, F(state.arena, "%s/mate-old", state.buildDirectory.data));
  String mateExe = ConvertExe(state.arena, state.mateExe);

  String compileCommand;
  if (StrEqual(&state.compiler, &S("gcc"))) {
    compileCommand = F(state.arena, "gcc \"%s\" -o \"%s\"", state.mateSource.data, mateExeNew.data);
  }

  if (StrEqual(&state.compiler, &S("clang"))) {
    compileCommand = F(state.arena, "clang \"%s\" -o \"%s\"", state.mateSource.data, mateExeNew.data);
  }

  if (StrEqual(&state.compiler, &S("tcc"))) {
    compileCommand = F(state.arena, "tcc \"%s\" -o \"%s\"", state.mateSource.data, mateExeNew.data);
  }

  if (StrEqual(&state.compiler, &S("MSVC"))) {
    compileCommand = F(state.arena, "cl.exe \"%s\" /Fe:\"%s\"", state.mateSource.data, mateExeNew.data);
  }

  LogWarn("%s changed rebuilding...", state.mateSource.data);
  errno_t rebuildResult = RunCommand(compileCommand);
  if (rebuildResult != 0) {
    LogError("Rebuild failed, with error: %d", rebuildResult);
    abort();
  }

  errno_t renameResult = FileRename(&mateExe, &mateExeOld);
  if (renameResult != SUCCESS) {
    LogError("Error renaming original executable: %d", renameResult);
    abort();
  }

  renameResult = FileRename(&mateExeNew, &mateExe);
  if (renameResult != SUCCESS) {
    LogError("Error moving new executable into place: %d", renameResult);
    FileRename(&mateExeOld, &state.mateExe);
    abort();
  }

  LogInfo("Rebuild finished, running %s", mateExe.data);
  errno_t result = RunCommand(mateExe);
  return exit(result);
}

void defaultExecutable() {
  String executableOutput = ConvertExe(state.arena, S("main"));
  executable.output = ConvertPath(state.arena, executableOutput);
  executable.flags = S("");
  executable.linkerFlags = S("");
  executable.includes = S("");
  executable.libs = S("");
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
  defaultExecutable();
  Executable options = parseExecutableOptions(executableOptions);
  if (!StrIsNull(&options.output)) {
    String executableOutput = ConvertExe(state.arena, options.output);
    executable.output = ConvertPath(state.arena, executableOutput);
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
      // MSVC doesn't have C23 yet
      flagsStr = F(state.arena, "%s %s", flagsStr.data, "/std:clatest");
      break;
    }
#endif
  }

  if (!StrIsNull(&flagsStr)) {
    executable.flags = flagsStr;
  }

  if (!StrIsNull(&options.linkerFlags)) {
    executable.linkerFlags = options.linkerFlags;
  }
  if (!StrIsNull(&options.includes)) {
    executable.includes = options.includes;
  }
  if (!StrIsNull(&options.libs)) {
    executable.libs = options.libs;
  }
}

// TODO: Add error enum
errno_t CreateCompileCommands() {
  FILE *ninjaPipe;
  FILE *outputFile;
  char buffer[4096];
  size_t bytes_read;

  String compileCommandsPath = ConvertPath(state.arena, F(state.arena, "%s/compile_commands.json", state.buildDirectory.data));

  errno_t err = fopen_s(&outputFile, compileCommandsPath.data, "w");
  if (err != 0 || outputFile == NULL) {
    LogError("Failed to open output file '%s': %s", compileCommandsPath.data, strerror(err));
    return 1;
  }

  String compdbCommand;
  if (state.mateCache.samuraiBuild) {
    String samuraiOutputPath = F(state.arena, "%s/samurai", state.buildDirectory.data);
    compdbCommand = ConvertPath(state.arena, F(state.arena, "%s -f %s/build.ninja -t compdb", samuraiOutputPath.data, state.buildDirectory.data));
  } else {
    compdbCommand = ConvertPath(state.arena, F(state.arena, "ninja -f %s/build.ninja -t compdb", state.buildDirectory.data));
  }

  ninjaPipe = popen(compdbCommand.data, "r");
  if (ninjaPipe == NULL) {
    LogError("Failed to run command");
    fclose(outputFile);
    return 1;
  }

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), ninjaPipe)) > 0) {
    fwrite(buffer, 1, bytes_read, outputFile);
  }

  fclose(outputFile);
  i32 status = pclose(ninjaPipe);
  if (status != 0) {
    LogError("Command failed with status %d\n", status);
    return status;
  }

  LogSuccess("Successfully created %s\n", compileCommandsPath.data);
  return SUCCESS;
}

static bool globMatch(String pattern, String text) {
  if (pattern.length == 1 && pattern.data[0] == '*') {
    return true;
  }

  i32 p = 0;
  i32 t = 0;
  i32 starP = -1;
  i32 starT = -1;
  while (t < text.length) {
    if (p < pattern.length && pattern.data[p] == text.data[t]) {
      p++;
      t++;
    } else if (p < pattern.length && pattern.data[p] == '*') {
      starP = p;
      starT = t;
      p++;
    } else if (starP != -1) {
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
  bool isGlob = false;
  for (int i = 0; i < source.length; i++) {
    if (source.data[i] == '*') {
      isGlob = true;
      break;
    }
  }

  if (source.length < 2 || source.data[0] != '.' || source.data[1] != '/') {
    abort();
  }

  if (source.length > 0 && source.data[source.length - 1] == '/') {
    abort();
  }

  if (!isGlob) {
    VecPush(executable.sources, source);
    return;
  }

  String directory = {0};
  i32 lastSlash = -1;
  for (int i = 0; i < source.length; i++) {
    if (source.data[i] == '/') {
      lastSlash = i;
    }
  }

  directory = StrSlice(state.arena, &source, 0, lastSlash);
  String pattern = StrSlice(state.arena, &source, lastSlash + 1, source.length);

  StringVector files = ListDir(state.arena, directory);
  for (int i = 0; i < files.length; i++) {
    String *file = VecAt(files, i);

    if (globMatch(pattern, *file)) {
      String finalSource = F(state.arena, "%s/%s", directory.data, file->data);
      VecPush(executable.sources, finalSource);
    }
  }
}

static bool removeFile(String source) {
  for (size_t i = 0; i < executable.sources.length; i++) {
    String *currValue = VecAt(executable.sources, i);
    if (StrEqual(&source, currValue)) {
      currValue->data = NULL;
      currValue->length = 0;
      return true;
    }
  }
  return false;
}

static StringVector outputTransformer(StringVector vector) {
  StringVector result = {0};
  for (size_t i = 0; i < vector.length; i++) {
    String *currentExecutable = VecAt(vector, i);
    if (StrIsNull(currentExecutable)) continue;

    String output = S("");
    for (size_t j = currentExecutable->length - 1; j > 0; j--) {
      String currentChar = StrNewSize(state.arena, &currentExecutable->data[j], 1);
      if (currentChar.data[0] == '/') {
        break;
      }
      output = StrConcat(state.arena, &currentChar, &output);
    }
    output.data[output.length - 1] = 'o';
    VecPush(result, output);
  }

  return result;
}

String InstallExecutable() {
  if (executable.sources.length == 0) {
    LogError("Executable has zero sources, add at least one with AddFile(\"./main.c\")");
    abort();
  }

  String linkCommand;
  String compileCommand;
  if (StrEqual(&state.compiler, &S("gcc"))) {
    linkCommand = F(state.arena, "rule link\n  command = $cc $flags $linker_flags -o $out $in $libs\n");
    compileCommand = F(state.arena, "rule compile\n  command = $cc $flags $includes -c $in -o $out\n");
  }

  if (StrEqual(&state.compiler, &S("clang"))) {
    linkCommand = F(state.arena, "rule link\n  command = $cc $flags $linker_flags -o $out $in $libs\n");
    compileCommand = F(state.arena, "rule compile\n  command = $cc $flags $includes -c $in -o $out\n");
  }

  if (StrEqual(&state.compiler, &S("tcc"))) {
    linkCommand = F(state.arena, "rule link\n  command = $cc $flags $linker_flags -o $out $in $libs\n");
    compileCommand = F(state.arena, "rule compile\n  command = $cc $flags $includes -c $in -o $out\n");
  }

  if (StrEqual(&state.compiler, &S("MSVC"))) {
    LogError("MSVC not yet implemented");
    abort();
  }

  String ninjaOutput = F(state.arena,
                         "cc = %s\n"
                         "linker_flags = %s\n"
                         "flags = %s\n"
                         "cwd = %s\n"
                         "builddir = %s\n"
                         "target = $builddir/%s\n"
                         "includes = %s\n"
                         "libs = %s\n"
                         "\n"
                         "%s\n"
                         "%s\n",
                         state.compiler.data,
                         executable.linkerFlags.data,
                         executable.flags.data,
                         ConvertNinjaPath(StrNew(state.arena, GetCwd())).data,
                         ConvertNinjaPath(state.buildDirectory).data,
                         executable.output.data,
                         executable.includes.data,
                         executable.libs.data,
                         linkCommand.data,
                         compileCommand.data);
  StringVector outputFiles = outputTransformer(executable.sources);

  String outputString = S("");
  for (size_t i = 0; i < executable.sources.length; i++) {
    String *currSource = VecAt(executable.sources, i);
    if (StrIsNull(currSource)) continue;
    String sourceFile = ParsePath(state.arena, *currSource);
    String outputFile = *VecAt(outputFiles, i);
    String source = F(state.arena, "build $builddir/%s: compile $cwd/%s\n", outputFile.data, sourceFile.data);
    ninjaOutput = StrConcat(state.arena, &ninjaOutput, &source);
    outputString = F(state.arena, "%s $builddir/%s", outputString.data, outputFile.data);
  }

  String target = F(state.arena,
                    "build $target: link%s\n"
                    "\n"
                    "default $target\n",
                    outputString.data);
  ninjaOutput = StrConcat(state.arena, &ninjaOutput, &target);

  String buildNinjaPath = F(state.arena, "%s/build.ninja", state.buildDirectory.data);
  FileWrite(&buildNinjaPath, &ninjaOutput);

  errno_t result;
  if (state.mateCache.samuraiBuild) {
    String samuraiOutputPath = F(state.arena, "%s/samurai", state.buildDirectory.data);
    result = RunCommand(F(state.arena, "%s -f %s", samuraiOutputPath.data, buildNinjaPath.data));
  } else {
    result = RunCommand(F(state.arena, "ninja -f %s", buildNinjaPath.data));
  }

  if (result != 0) {
    LogError("Ninja file compilation failed with code: %d", result);
    abort();
  }

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

// TODO: Allocate total string size instead of concat
// TODO: Do it depending on compiler
static void addLibraryPaths(StringVector *vector) {
  for (size_t i = 0; i < vector->length; i++) {
    String *currLib = VecAt((*vector), i);
    if (i == 0 && executable.libs.length == 0) {
      executable.libs = F(state.arena, "-L\"%s\"", currLib->data);
      continue;
    }

    executable.libs = F(state.arena, "%s -L\"%s\"", executable.libs.data, currLib->data);
  }
}

// TODO: Same thing here
static void addIncludePaths(StringVector *vector) {
  for (size_t i = 0; i < vector->length; i++) {
    String *currInclude = VecAt((*vector), i);
    if (i == 0 && executable.includes.length == 0) {
      executable.includes = F(state.arena, "-I\"%s\"", currInclude->data);
      continue;
    }

    executable.includes = F(state.arena, "%s -I\"%s\"", executable.includes.data, currInclude->data);
  }
}

static void linkSystemLibraries(StringVector *vector) {
  for (size_t i = 0; i < vector->length; i++) {
    String *currLib = VecAt((*vector), i);
    if (i == 0 && executable.libs.length == 0) {
      executable.libs = F(state.arena, "-l%s", currLib->data);
      continue;
    }

    executable.libs = F(state.arena, "%s -l%s", executable.libs.data, currLib->data);
  }
}

void EndBuild() {
  LogInfo("Build took: %llums", state.totalTime);
  ArenaFree(state.arena);
}
