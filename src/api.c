#include "api.h"

static MateConfig mateState = {0};

static const String MSVCStr = {.data = "cl.exe", .length = 6};
bool isMSVC() {
  return StrEq(mateState.compiler, MSVCStr);
}

static const String GCCStr = {.data = "gcc", .length = 3};
bool isGCC() {
  return StrEq(mateState.compiler, GCCStr);
}

static const String ClangStr = {.data = "clang", .length = 5};
bool isClang() {
  return StrEq(mateState.compiler, ClangStr);
}

static const String TCCStr = {.data = "tcc", .length = 3};
bool isTCC() {
  return StrEq(mateState.compiler, TCCStr);
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
  String path = NormalizeExePath(mateState.arena, str);
#if defined(PLATFORM_WIN)
  return F(mateState.arena, "%s\\%s", GetCwd(), path.data);
#else
  return F(state.arena, "%s/%s", GetCwd(), path.data);
#endif
}

static String fixPath(String str) {
  String path = NormalizePath(mateState.arena, str);
#if defined(PLATFORM_WIN)
  return F(mateState.arena, "%s\\%s", GetCwd(), path.data);
#else
  return F(state.arena, "%s/%s", GetCwd(), path.data);
#endif
}

String ConvertNinjaPath(String str) {
#if defined(PLATFORM_WIN)
  String copy = StrNewSize(mateState.arena, str.data, str.length + 1);
  memmove(&copy.data[2], &copy.data[1], str.length - 1);
  copy.data[1] = '$';
  copy.data[2] = ':';
  return copy;
#else
  return str;
#endif
}

static void setDefaultState() {
  mateState.arena = ArenaCreate(20000 * sizeof(String));
  mateState.compiler = GetCompiler();

  mateState.mateExe = fixPathExe(S("./mate"));
  mateState.mateSource = fixPath(S("./mate.c"));
  mateState.buildDirectory = fixPath(S("./build"));
}

static MateConfig parseMateConfig(MateOptions options) {
  MateConfig result;
  result.mateExe = StrNew(mateState.arena, options.mateExe);
  result.compiler = StrNew(mateState.arena, options.compiler);
  result.mateSource = StrNew(mateState.arena, options.mateSource);
  result.buildDirectory = StrNew(mateState.arena, options.buildDirectory);
  return result;
}

void CreateConfig(MateOptions options) {
  setDefaultState();
  MateConfig config = parseMateConfig(options);

  if (!StrIsNull(config.mateExe)) {
    mateState.mateExe = fixPathExe(config.mateExe);
  }

  if (!StrIsNull(config.mateSource)) {
    mateState.mateSource = fixPath(config.mateSource);
  }

  if (!StrIsNull(config.buildDirectory)) {
    mateState.buildDirectory = fixPath(config.buildDirectory);
  }

  if (!StrIsNull(config.compiler)) {
    mateState.compiler = config.compiler;
  }

  mateState.initConfig = true;
}

static void readCache() {
  String mateCachePath = F(mateState.arena, "%s/mate-cache.ini", mateState.buildDirectory.data);
  errno_t err = IniParse(mateCachePath, &mateState.cache);
  Assert("MateReadCache: failed reading MateCache at %s, err: %d", mateCachePath.data, err);

  mateState.mateCache.lastBuild = IniGetLong(&mateState.cache, S("modify-time"));
  if (mateState.mateCache.lastBuild == 0) {
    mateState.mateCache.firstBuild = true;
    mateState.mateCache.lastBuild = TimeNow() / 1000;

    String modifyTime = F(mateState.arena, "%ld", mateState.mateCache.lastBuild);
    IniSet(&mateState.cache, S("modify-time"), modifyTime);
  }

#if defined(PLATFORM_WIN)
  if (mateState.mateCache.firstBuild) {
    errno_t ninjaCheck = RunCommand(S("ninja --version > nul 2> nul"));
    Assert(ninjaCheck == SUCCESS, "MateReadCache: Ninja build system not found. Please install Ninja and add it to your PATH.");
  }
#else
  mateState.mateCache.samuraiBuild = IniGetBool(&mateState.cache, S("samurai-build"));
  if (mateState.mateCache.samuraiBuild == false) {
    Assert(mateState.mateCache.firstBuild, "MateCache: This is not the first build and samurai is not compiled, could be a cache error, delete `./build` folder and rebuild `./mate.c`");

    String samuraiAmalgam = s(SAMURAI_AMALGAM);
    String sourcePath = F(mateState.arena, "%s/samurai.c", mateState.buildDirectory.data);
    errno_t errFileWrite = FileWrite(sourcePath, samuraiAmalgam);
    Assert(errFileWrite == SUCCESS, "MateReadCache: failed writing samurai source code to path %s", sourcePath.data);

    String outputPath = F(mateState.arena, "%s/samurai", mateState.buildDirectory.data);
    String compileCommand = F(mateState.arena, "%s \"%s\" -o \"%s\" -lrt -std=c99", mateState.compiler.data, sourcePath.data, outputPath.data);

    errno_t err = RunCommand(compileCommand);
    Assert(err == SUCCESS, "MateReadCache: Error meanwhile compiling samurai at %s, if you are seeing this please make an issue at github.com/TomasBorquez/mate.h", sourcePath.data);

    LogSuccess("Successfully compiled samurai");
    mateState.mateCache.samuraiBuild = true;
    IniSet(&mateState.cache, S("samurai-build"), S("true"));
  }
#endif

  err = IniWrite(mateCachePath, &mateState.cache);
  Assert(err == SUCCESS, "MateReadCache: Failed writing cache, err: %d", err);
}

void StartBuild() {
  LogInit();
  if (!mateState.initConfig) {
    setDefaultState();
  }

  mateState.initConfig = true;
  mateState.startTime = TimeNow();

  Mkdir(mateState.buildDirectory);
  readCache();
  reBuild();
}

static bool needRebuild() {
  File stats = {0};
  errno_t err = FileStats(mateState.mateSource, &stats);
  Assert(err == SUCCESS, "Aborting rebuild: Could not read fileStats for %s, error: %d", mateState.mateSource.data, err);

  if (stats.modifyTime <= mateState.mateCache.lastBuild) {
    return false;
  }

  String mateCachePath = F(mateState.arena, "%s/mate-cache.ini", mateState.buildDirectory.data);
  String modifyTime = F(mateState.arena, "%ld", stats.modifyTime);
  IniSet(&mateState.cache, S("modify-time"), modifyTime);

  err = IniWrite(mateCachePath, &mateState.cache);
  Assert(err == SUCCESS, "Aborting rebuild: Could not write cache for path %s, error: %d", mateCachePath.data, err);

  return true;
}

void reBuild() {
  if (mateState.mateCache.firstBuild || !needRebuild()) {
    return;
  }

  String mateExeNew = NormalizeExePath(mateState.arena, F(mateState.arena, "%s/mate-new", mateState.buildDirectory.data));
  String mateExeOld = NormalizeExePath(mateState.arena, F(mateState.arena, "%s/mate-old", mateState.buildDirectory.data));
  String mateExe = NormalizeExePath(mateState.arena, mateState.mateExe);

  String compileCommand;
  if (isMSVC()) {
    compileCommand = F(mateState.arena, "cl.exe \"%s\" /Fe:\"%s\"", mateState.mateSource.data, mateExeNew.data);
  } else {
    compileCommand = F(mateState.arena, "%s \"%s\" -o \"%s\"", mateState.compiler.data, mateState.mateSource.data, mateExeNew.data);
  }

  LogWarn("%s changed rebuilding...", mateState.mateSource.data);
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

void defaultStaticLib() {
  mateState.staticLib.output = S("");
  mateState.staticLib.flags = S("");
  mateState.staticLib.arFlags = S("rcs");
}

static StaticLib parseStaticLibOptions(StaticLibOptions options) {
  StaticLib result = {0};
  result.output = StrNew(mateState.arena, options.output);
  Assert(!StrIsNull(result.output),
         "MateParseStaticLibOptions: failed, StaticLibOptions.output should never be null, please define the output name like this: \n"
         "\n"
         "CreateStaticLib((StaticLibOptions) { .output = \"libexample\"});");
  result.flags = StrNew(mateState.arena, options.flags);
  result.arFlags = StrNew(mateState.arena, options.arFlags);
  return result;
}

String CreateStaticLib(StaticLibOptions staticLibOptions) {
  Assert(!isMSVC(), "CreateStaticLib: MSVC compiler not yet implemented for static libraries");
  Assert(mateState.initConfig,
         "CreateStaticLib: before creating a static library you must use StartBuild(), like this: \n"
         "\n"
         "StartBuild()\n"
         "{\n"
         " // ...\n"
         "}\n"
         "EndBuild()");

  defaultStaticLib();
  StaticLib options = parseStaticLibOptions(staticLibOptions);

  String staticLibOutput = NormalizeStaticLibPath(mateState.arena, options.output);
  mateState.staticLib.output = NormalizePath(mateState.arena, staticLibOutput);

  String flagsStr = options.flags;
  if (flagsStr.data == NULL) {
    flagsStr = S("");
  }

  if (staticLibOptions.warnings != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (staticLibOptions.warnings) {
    case FLAG_WARNINGS_NONE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-w");
      break;
    case FLAG_WARNINGS_MINIMAL:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-Wall");
      break;
    case FLAG_WARNINGS:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-Wall -Wextra");
      break;
    case FLAG_WARNINGS_VERBOSE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-Wall -Wextra -Wpedantic");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (staticLibOptions.warnings) {
    case FLAG_WARNINGS_NONE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/W0");
      break;
    case FLAG_WARNINGS_MINIMAL:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/W3");
      break;
    case FLAG_WARNINGS:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/W4");
      break;
    case FLAG_WARNINGS_VERBOSE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/Wall");
      break;
    }
#endif
  }

  if (staticLibOptions.debug != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (staticLibOptions.debug) {
    case FLAG_DEBUG_MINIMAL:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-g1");
      break;
    case FLAG_DEBUG_MEDIUM:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-g2");
      break;
    case FLAG_DEBUG:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-g3");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (staticLibOptions.debug) {
    case FLAG_DEBUG_MINIMAL:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/Zi");
      break;
    case FLAG_DEBUG_MEDIUM:
    case FLAG_DEBUG:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/ZI");
      break;
    }
#endif
  }

  if (staticLibOptions.optimization != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (staticLibOptions.optimization) {
    case FLAG_OPTIMIZATION_NONE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-O0");
      break;
    case FLAG_OPTIMIZATION_BASIC:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-O1");
      break;
    case FLAG_OPTIMIZATION:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-O2");
      break;
    case FLAG_OPTIMIZATION_SIZE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-Os");
      break;
    case FLAG_OPTIMIZATION_AGGRESSIVE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-O3");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (staticLibOptions.optimization) {
    case FLAG_OPTIMIZATION_NONE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/Od");
      break;
    case FLAG_OPTIMIZATION_BASIC:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/O1");
      break;
    case FLAG_OPTIMIZATION:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/O2");
      break;
    case FLAG_OPTIMIZATION_SIZE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/O1");
      break;
    case FLAG_OPTIMIZATION_AGGRESSIVE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/Ox");
      break;
    }
#endif
  }

  if (staticLibOptions.std != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (staticLibOptions.std) {
    case FLAG_STD_C99:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-std=c99");
      break;
    case FLAG_STD_C11:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-std=c11");
      break;
    case FLAG_STD_C17:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-std=c17");
      break;
    case FLAG_STD_C23:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-std=c2x");
      break;
    case FLAG_STD_C2X:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-std=c2x");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (staticLibOptions.std) {
    case FLAG_STD_C99:
    case FLAG_STD_C11:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/std:c11");
      break;
    case FLAG_STD_C17:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/std:c17");
      break;
    case FLAG_STD_C23:
    case FLAG_STD_C2X:
      // NOTE: MSVC doesn't have C23 yet
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/std:clatest");
      break;
    }
#endif
  }

  if (!StrIsNull(flagsStr)) {
    mateState.staticLib.flags = flagsStr;
  }
  if (!StrIsNull(options.arFlags)) {
    mateState.staticLib.arFlags = options.arFlags;
  }

  String optionIncludes = s(staticLibOptions.includes);
  if (!StrIsNull(optionIncludes)) {
    mateState.includes = optionIncludes;
  }

  String optionLibs = s(staticLibOptions.libs);
  if (!StrIsNull(optionLibs)) {
    mateState.includes = optionLibs;
  }

  mateState.staticLib.ninjaBuildPath = F(mateState.arena, "%s/static-%s.ninja", mateState.buildDirectory.data, NormalizeExtension(mateState.arena, mateState.staticLib.output).data);
  return mateState.staticLib.ninjaBuildPath;
}

void defaultExecutable() {
  String executableOutput = NormalizeExePath(mateState.arena, S("main"));
  mateState.executable.output = NormalizePath(mateState.arena, executableOutput);
  mateState.executable.linkerFlags = S("");
  mateState.executable.flags = S("");
}

static Executable parseExecutableOptions(ExecutableOptions options) {
  Executable result;
  result.output = StrNew(mateState.arena, options.output);
  result.flags = StrNew(mateState.arena, options.flags);
  result.linkerFlags = StrNew(mateState.arena, options.linkerFlags);
  return result;
}

String CreateExecutable(ExecutableOptions executableOptions) {
  Assert(mateState.initConfig,
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
    String executableOutput = NormalizeExePath(mateState.arena, options.output);
    mateState.executable.output = NormalizePath(mateState.arena, executableOutput);
  }

  String flagsStr = options.flags;
  if (flagsStr.data == NULL) {
    flagsStr = S("");
  }

  if (executableOptions.warnings != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (executableOptions.warnings) {
    case FLAG_WARNINGS_NONE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-w");
      break;
    case FLAG_WARNINGS_MINIMAL:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-Wall");
      break;
    case FLAG_WARNINGS:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-Wall -Wextra");
      break;
    case FLAG_WARNINGS_VERBOSE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-Wall -Wextra -Wpedantic");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (executableOptions.warnings) {
    case FLAG_WARNINGS_NONE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/W0");
      break;
    case FLAG_WARNINGS_MINIMAL:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/W3");
      break;
    case FLAG_WARNINGS:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/W4");
      break;
    case FLAG_WARNINGS_VERBOSE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/Wall");
      break;
    }
#endif
  }

  if (executableOptions.debug != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (executableOptions.debug) {
    case FLAG_DEBUG_MINIMAL:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-g1");
      break;
    case FLAG_DEBUG_MEDIUM:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-g2");
      break;
    case FLAG_DEBUG:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-g3");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (executableOptions.debug) {
    case FLAG_DEBUG_MINIMAL:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/Zi");
      break;
    case FLAG_DEBUG_MEDIUM:
    case FLAG_DEBUG:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/ZI");
      break;
    }
#endif
  }

  if (executableOptions.optimization != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (executableOptions.optimization) {
    case FLAG_OPTIMIZATION_NONE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-O0");
      break;
    case FLAG_OPTIMIZATION_BASIC:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-O1");
      break;
    case FLAG_OPTIMIZATION:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-O2");
      break;
    case FLAG_OPTIMIZATION_SIZE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-Os");
      break;
    case FLAG_OPTIMIZATION_AGGRESSIVE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-O3");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (executableOptions.optimization) {
    case FLAG_OPTIMIZATION_NONE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/Od");
      break;
    case FLAG_OPTIMIZATION_BASIC:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/O1");
      break;
    case FLAG_OPTIMIZATION:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/O2");
      break;
    case FLAG_OPTIMIZATION_SIZE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/O1");
      break;
    case FLAG_OPTIMIZATION_AGGRESSIVE:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/Ox");
      break;
    }
#endif
  }

  if (executableOptions.std != 0) {
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    switch (executableOptions.std) {
    case FLAG_STD_C99:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-std=c99");
      break;
    case FLAG_STD_C11:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-std=c11");
      break;
    case FLAG_STD_C17:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-std=c17");
      break;
    case FLAG_STD_C23:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-std=c2x");
      break;
    case FLAG_STD_C2X:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "-std=c2x");
      break;
    }
#elif defined(COMPILER_MSVC)
    switch (executableOptions.std) {
    case FLAG_STD_C99:
    case FLAG_STD_C11:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/std:c11");
      break;
    case FLAG_STD_C17:
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/std:c17");
      break;
    case FLAG_STD_C23:
    case FLAG_STD_C2X:
      // NOTE: MSVC doesn't have C23 yet
      flagsStr = F(mateState.arena, "%s %s", flagsStr.data, "/std:clatest");
      break;
    }
#endif
  }

  if (!StrIsNull(flagsStr)) {
    mateState.executable.flags = flagsStr;
  }
  if (!StrIsNull(options.linkerFlags)) {
    mateState.executable.linkerFlags = options.linkerFlags;
  }

  String optionIncludes = s(executableOptions.includes);
  if (!StrIsNull(optionIncludes)) {
    mateState.includes = optionIncludes;
  }

  String optionLibs = s(executableOptions.libs);
  if (!StrIsNull(optionLibs)) {
    mateState.includes = optionLibs;
  }

  mateState.executable.ninjaBuildPath = F(mateState.arena, "%s/exe-%s.ninja", mateState.buildDirectory.data, NormalizeExtension(mateState.arena, mateState.executable.output).data);
  return mateState.executable.ninjaBuildPath;
}

errno_t CreateCompileCommands(String ninjaBuildPath) {
  FILE *ninjaPipe;
  FILE *outputFile;
  char buffer[4096];
  size_t bytes_read;

  String compileCommandsPath = NormalizePath(mateState.arena, F(mateState.arena, "%s/compile_commands.json", mateState.buildDirectory.data));
  errno_t err = fopen_s(&outputFile, compileCommandsPath.data, "w");
  if (err != SUCCESS || outputFile == NULL) {
    LogError("CreateCompileCommands: Failed to open file %s, err: %d", compileCommandsPath.data, err);
    return COMPILE_COMMANDS_FAILED_OPEN_FILE;
  }

  String compdbCommand;
  if (mateState.mateCache.samuraiBuild == true) {
    String samuraiOutputPath = F(mateState.arena, "%s/samurai", mateState.buildDirectory.data);
    compdbCommand = NormalizePath(mateState.arena, F(mateState.arena, "%s -f %s -t compdb", samuraiOutputPath.data, ninjaBuildPath.data));
  }

  if (mateState.mateCache.samuraiBuild == false) {
    compdbCommand = NormalizePath(mateState.arena, F(mateState.arena, "ninja -f %s -t compdb", ninjaBuildPath.data));
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

  LogSuccess("Successfully created %s", NormalizePathEnd(mateState.arena, compileCommandsPath).data);
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
    VecPush(mateState.sources, source);
    return;
  }

  String directory = {0};
  i32 lastSlash = -1;
  for (size_t i = 0; i < source.length; i++) {
    if (source.data[i] == '/') {
      lastSlash = i;
    }
  }

  directory = StrSlice(mateState.arena, source, 0, lastSlash);
  String pattern = StrSlice(mateState.arena, source, lastSlash + 1, source.length);

  StringVector files = ListDir(mateState.arena, directory);
  for (size_t i = 0; i < files.length; i++) {
    String file = VecAt(files, i);

    if (globMatch(pattern, file)) {
      String finalSource = F(mateState.arena, "%s/%s", directory.data, file.data);
      VecPush(mateState.sources, finalSource);
    }
  }
}

static bool removeFile(String source) {
  Assert(mateState.sources.length > 0, "RemoveFile: Before removing a file you must first add a file, use: AddFile()");

  for (size_t i = 0; i < mateState.sources.length; i++) {
    String *currValue = VecAtPtr(mateState.sources, i);
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
      if (StrIsNull(currentExecutable)) {
        VecPush(result, S(""));
        continue;
      }

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

      String objOutput = StrNewSize(mateState.arena, filenameStart, filenameLength + 2);
      objOutput.data[objOutput.length - 3] = 'o';
      objOutput.data[objOutput.length - 2] = 'b';
      objOutput.data[objOutput.length - 1] = 'j';

      VecPush(result, objOutput);
    }
    return result;
  }

  for (size_t i = 0; i < vector.length; i++) {
    String currentExecutable = VecAt(vector, i);
    if (StrIsNull(currentExecutable)) {
      VecPush(result, S(""));
      continue;
    }

    size_t lastCharIndex = 0;
    for (size_t j = currentExecutable.length - 1; j > 0; j--) {
      char currentChar = currentExecutable.data[j];
      if (currentChar == '/') {
        lastCharIndex = j;
        break;
      }
    }
    Assert(lastCharIndex != 0, "MateOutputTransformer: failed to transform %s, to an object file", currentExecutable.data);
    String output = StrSlice(mateState.arena, currentExecutable, lastCharIndex + 1, currentExecutable.length);
    output.data[output.length - 1] = 'o';
    VecPush(result, output);
  }
  return result;
}

void ResetExecutable() {
  mateState.executable = (Executable){0};
  VecFree(mateState.sources);
}

String InstallExecutable() {
  Assert(mateState.sources.length != 0, "InstallExecutable: Executable has zero sources, add at least one with AddFile(\"./main.c\")");
  Assert(!StrIsNull(mateState.executable.output), "InstallExecutable: Before installing executable you must first CreateExecutable()");

  StringBuilder builder = StringBuilderReserve(mateState.arena, 1024);

  // Compiler
  StringBuilderAppend(mateState.arena, &builder, &S("cc = "));
  StringBuilderAppend(mateState.arena, &builder, &mateState.compiler);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Linker flags
  if (mateState.executable.linkerFlags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("linker_flags = "));
    StringBuilderAppend(mateState.arena, &builder, &mateState.executable.linkerFlags);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Compiler flags
  if (mateState.executable.flags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("flags = "));
    StringBuilderAppend(mateState.arena, &builder, &mateState.executable.flags);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Include paths
  if (mateState.includes.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("includes = "));
    StringBuilderAppend(mateState.arena, &builder, &mateState.includes);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Libraries
  if (mateState.libs.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("libs = "));
    StringBuilderAppend(mateState.arena, &builder, &mateState.libs);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Current working directory
  String cwd_path = ConvertNinjaPath(s(GetCwd()));
  StringBuilderAppend(mateState.arena, &builder, &S("cwd = "));
  StringBuilderAppend(mateState.arena, &builder, &cwd_path);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Build directory
  String build_dir_path = ConvertNinjaPath(mateState.buildDirectory);
  StringBuilderAppend(mateState.arena, &builder, &S("builddir = "));
  StringBuilderAppend(mateState.arena, &builder, &build_dir_path);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Target
  StringBuilderAppend(mateState.arena, &builder, &S("target = $builddir/"));
  StringBuilderAppend(mateState.arena, &builder, &mateState.executable.output);
  StringBuilderAppend(mateState.arena, &builder, &S("\n\n"));

  // Link command
  StringBuilderAppend(mateState.arena, &builder, &S("rule link\n  command = $cc"));
  if (mateState.executable.flags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $flags"));
  }

  if (mateState.executable.linkerFlags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $linker_flags"));
  }

  if (isMSVC()) {
    StringBuilderAppend(mateState.arena, &builder, &S(" /Fe:$out $in"));
  } else {
    StringBuilderAppend(mateState.arena, &builder, &S(" -o $out $in"));
  }

  if (mateState.libs.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $libs"));
  }
  StringBuilderAppend(mateState.arena, &builder, &S("\n\n"));

  // Compile command
  StringBuilderAppend(mateState.arena, &builder, &S("rule compile\n  command = $cc"));
  if (mateState.executable.flags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $flags"));
  }
  if (mateState.includes.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $includes"));
  }

  if (isMSVC()) {
    StringBuilderAppend(mateState.arena, &builder, &S(" /c $in /Fo:$out\n\n"));
  } else {
    StringBuilderAppend(mateState.arena, &builder, &S(" -c $in -o $out\n\n"));
  }

  // Build individual source files
  StringVector outputFiles = outputTransformer(mateState.sources);
  StringBuilder outputBuilder = StringBuilderCreate(mateState.arena);
  for (size_t i = 0; i < mateState.sources.length; i++) {
    String currSource = VecAt(mateState.sources, i);
    if (StrIsNull(currSource)) continue;

    String outputFile = VecAt(outputFiles, i);
    String sourceFile = NormalizePathStart(mateState.arena, currSource);

    // Source build command
    StringBuilderAppend(mateState.arena, &builder, &S("build $builddir/"));
    StringBuilderAppend(mateState.arena, &builder, &outputFile);
    StringBuilderAppend(mateState.arena, &builder, &S(": compile $cwd/"));
    StringBuilderAppend(mateState.arena, &builder, &sourceFile);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));

    // Add to output files list
    if (outputBuilder.buffer.length == 0) {
      StringBuilderAppend(mateState.arena, &outputBuilder, &S("$builddir/"));
      StringBuilderAppend(mateState.arena, &outputBuilder, &outputFile);
    } else {
      StringBuilderAppend(mateState.arena, &outputBuilder, &S(" $builddir/"));
      StringBuilderAppend(mateState.arena, &outputBuilder, &outputFile);
    }
  }

  // Build target
  StringBuilderAppend(mateState.arena, &builder, &S("build $target: link "));
  StringBuilderAppend(mateState.arena, &builder, &outputBuilder.buffer);
  StringBuilderAppend(mateState.arena, &builder, &S("\n\n"));

  // Default target
  StringBuilderAppend(mateState.arena, &builder, &S("default $target\n"));

  String ninjaBuildPath = mateState.executable.ninjaBuildPath;
  errno_t errWrite = FileWrite(ninjaBuildPath, builder.buffer);
  Assert(errWrite == SUCCESS, "InstallExecutable: failed to write build.ninja for %s, err: %d", ninjaBuildPath.data, errWrite);

  String buildCommand;
  if (mateState.mateCache.samuraiBuild) {
    String samuraiOutputPath = F(mateState.arena, "%s/samurai", mateState.buildDirectory.data);
    buildCommand = F(mateState.arena, "%s -f %s", samuraiOutputPath.data, ninjaBuildPath.data);
  } else {
    buildCommand = F(mateState.arena, "ninja -f %s", ninjaBuildPath.data);
  }

  i64 err = RunCommand(buildCommand);
  Assert(err == SUCCESS, "InstallExecutable: Ninja file compilation failed with code: %lu", err);

  LogSuccess("Ninja file compilation done for %s", NormalizePathEnd(mateState.arena, ninjaBuildPath).data);
  mateState.totalTime = TimeNow() - mateState.startTime;

#if defined(PLATFORM_WIN)
  String path = F(mateState.arena, "%s\\%s", mateState.buildDirectory.data, mateState.executable.output.data);
#else
  String path = F(mateState.arena, "%s/%s", mateState.buildDirectory.data, mateState.executable.output.data);
#endif

  ResetExecutable();
  return path;
}

void ResetStaticLib() {
  mateState.staticLib = (StaticLib){0};
  VecFree(mateState.sources);
}

String InstallStaticLib() {
  Assert(mateState.sources.length != 0, "InstallStaticLib: Static Library has zero sources, add at least one with AddFile(\"./main.c\")");
  Assert(!StrIsNull(mateState.staticLib.output), "InstallStaticLib: Before installing static library you must first CreateStaticLib()");

  StringBuilder builder = StringBuilderReserve(mateState.arena, 1024);

  // Compiler
  StringBuilderAppend(mateState.arena, &builder, &S("cc = "));
  StringBuilderAppend(mateState.arena, &builder, &mateState.compiler);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Archive
  StringBuilderAppend(mateState.arena, &builder, &S("ar = ar\n")); // TODO: Add different ar for MSVC

  // Compiler flags
  if (mateState.staticLib.flags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("flags = "));
    StringBuilderAppend(mateState.arena, &builder, &mateState.staticLib.flags);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Archive flags
  if (mateState.staticLib.arFlags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("ar_flags = "));
    StringBuilderAppend(mateState.arena, &builder, &mateState.staticLib.arFlags);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Include paths
  if (mateState.includes.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("includes = "));
    StringBuilderAppend(mateState.arena, &builder, &mateState.includes);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));
  }

  // Current working directory
  String cwd_path = ConvertNinjaPath(s(GetCwd()));
  StringBuilderAppend(mateState.arena, &builder, &S("cwd = "));
  StringBuilderAppend(mateState.arena, &builder, &cwd_path);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Build directory
  String build_dir_path = ConvertNinjaPath(mateState.buildDirectory);
  StringBuilderAppend(mateState.arena, &builder, &S("builddir = "));
  StringBuilderAppend(mateState.arena, &builder, &build_dir_path);
  StringBuilderAppend(mateState.arena, &builder, &S("\n"));

  // Target
  StringBuilderAppend(mateState.arena, &builder, &S("target = $builddir/"));
  StringBuilderAppend(mateState.arena, &builder, &mateState.staticLib.output);
  StringBuilderAppend(mateState.arena, &builder, &S("\n\n"));

  // Archive command
  StringBuilderAppend(mateState.arena, &builder, &S("rule archive\n  command = $ar $ar_flags $out $in\n\n"));

  // Compile command
  StringBuilderAppend(mateState.arena, &builder, &S("rule compile\n  command = $cc"));
  if (mateState.staticLib.flags.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $flags"));
  }
  if (mateState.includes.length > 0) {
    StringBuilderAppend(mateState.arena, &builder, &S(" $includes"));
  }
  StringBuilderAppend(mateState.arena, &builder, &S(" -c $in -o $out\n\n"));

  // Build individual source files
  StringVector outputFiles = outputTransformer(mateState.sources);
  StringBuilder outputBuilder = StringBuilderCreate(mateState.arena);
  for (size_t i = 0; i < mateState.sources.length; i++) {
    String currSource = VecAt(mateState.sources, i);
    if (StrIsNull(currSource)) continue;

    String outputFile = VecAt(outputFiles, i);
    String sourceFile = NormalizePathStart(mateState.arena, currSource);

    // Source build command
    StringBuilderAppend(mateState.arena, &builder, &S("build $builddir/"));
    StringBuilderAppend(mateState.arena, &builder, &outputFile);
    StringBuilderAppend(mateState.arena, &builder, &S(": compile $cwd/"));
    StringBuilderAppend(mateState.arena, &builder, &sourceFile);
    StringBuilderAppend(mateState.arena, &builder, &S("\n"));

    // Add to output files list
    if (outputBuilder.buffer.length == 0) {
      StringBuilderAppend(mateState.arena, &outputBuilder, &S("$builddir/"));
      StringBuilderAppend(mateState.arena, &outputBuilder, &outputFile);
    } else {
      StringBuilderAppend(mateState.arena, &outputBuilder, &S(" $builddir/"));
      StringBuilderAppend(mateState.arena, &outputBuilder, &outputFile);
    }
  }

  // Build target
  StringBuilderAppend(mateState.arena, &builder, &S("build $target: archive "));
  StringBuilderAppend(mateState.arena, &builder, &outputBuilder.buffer);
  StringBuilderAppend(mateState.arena, &builder, &S("\n\n"));

  // Default target
  StringBuilderAppend(mateState.arena, &builder, &S("default $target\n"));

  String ninjaBuildPath = mateState.staticLib.ninjaBuildPath;
  errno_t errWrite = FileWrite(ninjaBuildPath, builder.buffer);
  Assert(errWrite == SUCCESS, "InstallStaticLib: failed to write build-static-library.ninja for %s, err: %d", ninjaBuildPath.data, errWrite);

  String buildCommand;
  if (mateState.mateCache.samuraiBuild) {
    String samuraiOutputPath = F(mateState.arena, "%s/samurai", mateState.buildDirectory.data);
    buildCommand = F(mateState.arena, "%s -f %s", samuraiOutputPath.data, ninjaBuildPath.data);
  } else {
    buildCommand = F(mateState.arena, "ninja -f %s", ninjaBuildPath.data);
  }

  i64 err = RunCommand(buildCommand);
  Assert(err == SUCCESS, "InstallStaticLib: Ninja file compilation failed with code: %lu", err);

  LogSuccess("Ninja file compilation done for %s", NormalizePathEnd(mateState.arena, ninjaBuildPath).data);
  mateState.totalTime = TimeNow() - mateState.startTime;

#if defined(PLATFORM_WIN)
  String path = F(mateState.arena, "%s\\%s", mateState.buildDirectory.data, mateState.staticLib.output.data);
#else
  String path = F(state.arena, "%s/%s", mateState.buildDirectory.data, mateState.staticLib.output.data);
#endif

  ResetStaticLib();
  return path;
}

errno_t RunCommand(String command) {
#if defined(PLATFORM_LINUX)
  return system(command.data) >> 8;
#else
  return system(command.data);
#endif
}

static void addLibraryPaths(StringVector *vector) {
  StringBuilder builder = StringBuilderCreate(mateState.arena);

  if (isMSVC() && mateState.libs.length == 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("/link"));
  }

  if (mateState.libs.length) {
    StringBuilderAppend(mateState.arena, &builder, &mateState.libs);
  }

  if (isMSVC()) {
    // MSVC format: /LIBPATH:"path"
    for (size_t i = 0; i < vector->length; i++) {
      String currLib = VecAt((*vector), i);
      String buffer = F(mateState.arena, " /LIBPATH:\"%s\"", currLib.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  } else {
    // GCC/Clang format: -L"path"
    for (size_t i = 0; i < vector->length; i++) {
      String currLib = VecAt((*vector), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mateState.arena, "-L\"%s\"", currLib.data);
        StringBuilderAppend(mateState.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(mateState.arena, " -L\"%s\"", currLib.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  }

  mateState.libs = builder.buffer;
}

static void linkSystemLibraries(StringVector *vector) {
  StringBuilder builder = StringBuilderCreate(mateState.arena);

  if (isMSVC() && mateState.libs.length == 0) {
    StringBuilderAppend(mateState.arena, &builder, &S("/link"));
  }

  if (mateState.libs.length) {
    StringBuilderAppend(mateState.arena, &builder, &mateState.libs);
  }

  if (isMSVC()) {
    // MSVC format: library.lib
    for (size_t i = 0; i < vector->length; i++) {
      String currLib = VecAt((*vector), i);
      String buffer = F(mateState.arena, " %s.lib", currLib.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  } else {
    // GCC/Clang format: -llib
    for (size_t i = 0; i < vector->length; i++) {
      String currLib = VecAt((*vector), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mateState.arena, "-l%s", currLib.data);
        StringBuilderAppend(mateState.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(mateState.arena, " -l%s", currLib.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  }

  mateState.libs = builder.buffer;
}

static void addIncludePaths(StringVector *vector) {
  StringBuilder builder = StringBuilderCreate(mateState.arena);

  if (mateState.includes.length) {
    StringBuilderAppend(mateState.arena, &builder, &mateState.includes);
    StringBuilderAppend(mateState.arena, &builder, &S(" "));
  }

  if (isMSVC()) {
    // MSVC format: /I"path"
    for (size_t i = 0; i < vector->length; i++) {
      String currInclude = VecAt((*vector), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mateState.arena, "/I\"%s\"", currInclude.data);
        StringBuilderAppend(mateState.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(mateState.arena, " /I\"%s\"", currInclude.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  } else {
    // GCC/Clang format: -I"path"
    for (size_t i = 0; i < vector->length; i++) {
      String currInclude = VecAt((*vector), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mateState.arena, "-I\"%s\"", currInclude.data);
        StringBuilderAppend(mateState.arena, &builder, &buffer);
        continue;
      }
      String buffer = F(mateState.arena, " -I\"%s\"", currInclude.data);
      StringBuilderAppend(mateState.arena, &builder, &buffer);
    }
  }

  mateState.includes = builder.buffer;
}

void EndBuild() {
  LogInfo("Build took: %ldms", mateState.totalTime);
  ArenaFree(mateState.arena);
}
