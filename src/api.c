/* MIT License
   mate.h - Mate Implementations start here
   Guide on the `README.md`
*/
#include "api.h"

static MateConfig state = {0};
static Executable executable = {0};

String FixPathExe(String *str) {
  String path = ConvertPath(&state.arena, ConvertExe(&state.arena, *str));
#if defined(PLATFORM_WIN)
  return F(&state.arena, "%s\\%s", GetCwd(), path.data);
#elif defined(PLATFORM_LINUX)
  return F(&state.arena, "%s/%s", GetCwd(), path.data);
#endif
}

String FixPath(String *str) {
  String path = ConvertPath(&state.arena, *str);
#if defined(PLATFORM_WIN)
  return F(&state.arena, "%s\\%s", GetCwd(), path.data);
#elif defined(PLATFORM_LINUX)
  return F(&state.arena, "%s/%s", GetCwd(), path.data);
#endif
}

String ConvertNinjaPath(String str) {
  String copy = StrNewSize(&state.arena, str.data, str.length + 1);
  memmove(&copy.data[2], &copy.data[1], str.length - 1);
  copy.data[1] = '$';
  copy.data[2] = ':';
  return copy;
}

static void setDefaultState() {
  state.arena = ArenaInit(12000 * sizeof(String));
  state.mateExe = FixPath(&S("./mate"));
  state.compiler = GetCompiler();
  state.mateSource = FixPath(&S("./mate.c"));
  state.mateCachePath = FixPath(&S("./build/mate-cache.json"));
  state.buildDirectory = ConvertPath(&state.arena, S("./build"));
}

static MateConfig parseMateConfig(MateOptions options) {
  MateConfig result;
  result.mateExe = StrNew(&state.arena, options.mateExe);
  result.compiler = StrNew(&state.arena, options.compiler);
  result.mateSource = StrNew(&state.arena, options.mateSource);
  result.mateCachePath = StrNew(&state.arena, options.mateCachePath);
  result.buildDirectory = StrNew(&state.arena, options.buildDirectory);
  return result;
}

void CreateConfig(MateOptions options) {
  setDefaultState();
  MateConfig config = parseMateConfig(options);

  if (!StrIsNull(&config.mateSource)) {
    state.mateSource = FixPath(&config.mateSource);
  }

  if (!StrIsNull(&config.mateCachePath)) {
    state.mateCachePath = FixPath(&config.mateCachePath);
  }

  if (!StrIsNull(&config.mateExe)) {
    state.mateExe = FixPath(&config.mateExe);
  }

  if (!StrIsNull(&config.buildDirectory)) {
    state.buildDirectory = ConvertPath(&state.arena, config.buildDirectory);
  }

  if (!StrIsNull(&config.compiler)) {
    state.compiler = config.compiler;
  }

  state.customConfig = true;
}

errno_t readCache() {
  String cache = {0};
  errno_t err = FileRead(&state.arena, &state.mateCachePath, &cache);

  if (err == FILE_NOT_EXIST) {
    String modifyTime = F(&state.arena, "%llu", TimeNow() / 1000);
    FileWrite(&state.mateCachePath, &modifyTime);
    state.mateCache.firstBuild = true;
    cache = modifyTime;
  } else if (err != SUCCESS) {
    return err;
  }

  char *endptr;
  state.mateCache.lastBuild = strtoll(cache.data, &endptr, 10);
  return SUCCESS;
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
    String modifyTime = F(&state.arena, "%llu", stats.modifyTime);
    FileWrite(&state.mateCachePath, &modifyTime);
    return true;
  }

  return false;
}

void reBuild() {
  if (state.mateCache.firstBuild || !needRebuild()) {
    return;
  }

  String mateExeNew = F(&state.arena, "%s/mate-new", state.buildDirectory.data);
  String mateExeOld = F(&state.arena, "%s/mate-old", state.buildDirectory.data);
  String mateExe = ConvertExe(&state.arena, state.mateExe);
  mateExeNew = FixPathExe(&mateExeNew);
  mateExeOld = FixPathExe(&mateExeOld);

  String compileCommand;
  if (StrEqual(&state.compiler, &S("gcc"))) {
    compileCommand = F(&state.arena, "gcc -o \"%s\" -Wall -g \"%s\"", mateExeNew.data, state.mateSource.data);
  }

  if (StrEqual(&state.compiler, &S("clang"))) {
    compileCommand = F(&state.arena, "clang -o \"%s\" -Wall -g \"%s\"", mateExeNew.data, state.mateSource.data);
  }

  if (StrEqual(&state.compiler, &S("MSVC"))) {
    compileCommand = F(&state.arena, "cl.exe /Fe:\"%s\" /W4 /Zi \"%s\"", mateExeNew.data, state.mateSource.data);
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
  String executableOutput = ConvertExe(&state.arena, S("main"));
  executable.output = ConvertPath(&state.arena, executableOutput);
  executable.flags = S("-Wall -g");
  executable.linkerFlags = S("");
  executable.includes = S("");
  executable.libs = S("");
}

static Executable parseExecutableOptions(ExecutableOptions options) {
  Executable result;
  result.output = StrNew(&state.arena, options.output);
  result.flags = StrNew(&state.arena, options.flags);
  result.linkerFlags = StrNew(&state.arena, options.linkerFlags);
  result.includes = StrNew(&state.arena, options.includes);
  result.libs = StrNew(&state.arena, options.libs);
  return result;
}

void CreateExecutable(ExecutableOptions executableOptions) {
  defaultExecutable();
  Executable options = parseExecutableOptions(executableOptions);

  if (!StrIsNull(&options.output)) {
    String executableOutput = ConvertExe(&state.arena, options.output);
    executable.output = ConvertPath(&state.arena, executableOutput);
  }

  if (!StrIsNull(&options.flags)) {
    executable.flags = options.flags;
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

// TODO: Implement for linux
// TODO: Add error enum
errno_t CreateCompileCommands() {
  FILE *ninjaPipe;
  FILE *outputFile;
  char buffer[4096];
  size_t bytes_read;

  String buildPath = StrNew(&state.arena, F(&state.arena, "%s\\%s", GetCwd(), ParsePath(&state.arena, state.buildDirectory).data).data);
  String compileCommandsPath = ConvertPath(&state.arena, F(&state.arena, "%s/compile_commands.json", buildPath.data));

  errno_t err = fopen_s(&outputFile, compileCommandsPath.data, "w");
  if (err != 0 || outputFile == NULL) {
    LogError("Failed to open output file '%s': %s", compileCommandsPath.data, strerror(err));
    return 1;
  }

  String compdbCommand = ConvertPath(&state.arena, F(&state.arena, "ninja -f %s/build.ninja -t compdb", buildPath.data));

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

static void addFile(String source) {
  VecPush(executable.sources, source);
}

static StringVector outputTransformer(StringVector vector) {
  StringVector result = {0};
  for (size_t i = 0; i < vector.length; i++) {
    String *currentExecutable = VecAt(vector, i);
    String output = S("");
    for (size_t j = currentExecutable->length - 1; j > 0; j--) {
      String currentChar = StrNewSize(&state.arena, &currentExecutable->data[j], 1);
      if (currentChar.data[0] == '/') {
        break;
      }
      output = StrConcat(&state.arena, &currentChar, &output);
    }
    output.data[output.length - 1] = 'o';
    VecPush(result, output);
  }

  return result;
}

// TODO: Make linux version
String InstallExecutable() {
  if (executable.sources.length == 0) {
    LogError("Executable has zero sources, add at least one with AddFile(\"./main.c\")");
    abort();
  }

  String linkCommand;
  String compileCommand;
  if (StrEqual(&state.compiler, &S("gcc"))) {
    linkCommand = F(&state.arena, "rule link\n  command = $cc $flags $linker_flags -o $out $in $libs\n");
    compileCommand = F(&state.arena, "rule compile\n  command = $cc $flags $includes -c $in -o $out\n");
  }

  if (StrEqual(&state.compiler, &S("clang"))) {
    linkCommand = F(&state.arena, "rule link\n  command = $cc $flags $linker_flags -o $out $in $libs\n");
    compileCommand = F(&state.arena, "rule compile\n  command = $cc $flags $includes -c $in -o $out\n");
  }

  if (StrEqual(&state.compiler, &S("MSVC"))) {
    LogError("MSVC not yet implemented");
    abort();
  }

  String ninjaOutput = F(&state.arena,
                         "cc = %s\n"
                         "linker_flag = %s\n"
                         "flags = %s\n"
                         "cwd = %s\n"
                         "builddir = $cwd/%s\n"
                         "target = $builddir/%s\n"
                         "includes = %s\n"
                         "libs = %s\n"
                         "\n"
                         "%s\n"
                         "%s\n",
                         state.compiler.data,
                         executable.linkerFlags.data,
                         executable.flags.data,
                         ConvertNinjaPath(StrNew(&state.arena, GetCwd())).data,
                         state.buildDirectory.data,
                         executable.output.data,
                         executable.includes.data,
                         executable.libs.data,
                         linkCommand.data,
                         compileCommand.data);
  StringVector outputFiles = outputTransformer(executable.sources);

  assert(outputFiles.length == executable.sources.length && "Something went wrong in the parsing");

  String outputString = S("");
  for (size_t i = 0; i < executable.sources.length; i++) {
    String sourceFile = ParsePath(&state.arena, *VecAt(executable.sources, i));
    String outputFile = *VecAt(outputFiles, i);
    String source = F(&state.arena, "build $builddir/%s: compile $cwd/%s\n", outputFile.data, sourceFile.data);
    ninjaOutput = StrConcat(&state.arena, &ninjaOutput, &source);
    outputString = F(&state.arena, "%s $builddir/%s", outputString.data, outputFile.data);
  }

  String target = F(&state.arena,
                    "build $target: link%s\n"
                    "\n"
                    "default $target\n",
                    outputString.data);
  ninjaOutput = StrConcat(&state.arena, &ninjaOutput, &target);

  String buildNinjaPath = F(&state.arena, "%s\\build.ninja", FixPath(&state.buildDirectory).data);
  FileWrite(&buildNinjaPath, &ninjaOutput);

  errno_t result = RunCommand(F(&state.arena, "ninja -f %s", buildNinjaPath.data));
  if (result != 0) {
    LogError("Ninja file compilation failed with code: %d", result);
    abort();
  }

  LogSuccess("Ninja file compilation done");
  state.totalTime = TimeNow() - state.startTime;
  return F(&state.arena, "%s\\%s", FixPath(&state.buildDirectory).data, executable.output.data);
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
      executable.libs = F(&state.arena, "-L\"%s\"", currLib->data);
      continue;
    }

    executable.libs = F(&state.arena, "%s -L\"%s\"", executable.libs.data, currLib->data);
  }
}

// TODO: Same thing here
static void addIncludePaths(StringVector *vector) {
  for (size_t i = 0; i < vector->length; i++) {
    String *currInclude = VecAt((*vector), i);
    if (i == 0 && executable.includes.length == 0) {
      executable.includes = F(&state.arena, "-I\"%s\"", currInclude->data);
      continue;
    }

    executable.includes = F(&state.arena, "%s -I\"%s\"", executable.includes.data, currInclude->data);
  }
}

static void linkSystemLibraries(StringVector *vector) {
  for (size_t i = 0; i < vector->length; i++) {
    String *currLib = VecAt((*vector), i);
    if (i == 0 && executable.libs.length == 0) {
      executable.libs = F(&state.arena, "-l%s", currLib->data);
      continue;
    }

    executable.libs = F(&state.arena, "%s -l%s", executable.libs.data, currLib->data);
  }
}

void EndBuild() {
  LogInfo("Build took: %llums", state.totalTime);
  ArenaFree(&state.arena);
}
