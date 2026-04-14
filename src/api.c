#include "api.h"

static MateConfig mate_state = {0};

/* --- Build System Implementation --- */
static void mate_set_default_state(void) {
  mate_state.arena = ArenaCreate(20000 * sizeof(String));
  mate_state.compiler = GetCompiler();

  mate_state.mate_exe = mate_fix_path_exe(S("./mate"));
  mate_state.mate_source = mate_fix_path(S("./mate.c"));
  mate_state.build_directory = mate_fix_path(S("./build"));

  mate_state.init_config = true;
}

void CreateConfig(MateOptions options) {
  mate_set_default_state();
  if (options.compiler != 0) mate_state.compiler = options.compiler;
  if (options.mateExe != NULL) mate_state.mate_exe = mate_fix_path_exe(s(options.mateExe));
  if (options.mateSource != NULL) mate_state.mate_source = mate_fix_path(s(options.mateSource));
  if (options.buildDirectory != NULL) mate_state.build_directory = mate_fix_path(s(options.buildDirectory));
}

static void mate_read_cache(void) {
  String mateCachePath = F(mate_state.arena, "%s/mate-cache.ini", mate_state.build_directory.data);
  errno_t err = IniParse(mateCachePath, &mate_state.cache);
  Assert(err == SUCCESS, "MateReadCache: failed reading MateCache at %s, err: %d", mateCachePath.data, err);

  mate_state.mate_cache.last_build = IniGetLong(&mate_state.cache, S("modify-time"));
  if (mate_state.mate_cache.last_build == 0) {
    mate_state.mate_cache.first_build = true;
    mate_state.mate_cache.last_build = TimeNow() / 1000;

    String modifyTime = F(mate_state.arena, FMT_I64, mate_state.mate_cache.last_build);
    IniSet(&mate_state.cache, S("modify-time"), modifyTime);
  }

#if defined(PLATFORM_WIN)
  if (mate_state.mate_cache.first_build) {
    errno_t ninjaCheck = RunCommand(S("ninja --version > nul 2> nul"));
    Assert(ninjaCheck == SUCCESS, "MateReadCache: Ninja build system not found. Please install Ninja and add it to your PATH.");
  }
#else
  mate_state.mate_cache.samurai_build = IniGetBool(&mate_state.cache, S("samurai-build"));
  if (mate_state.mate_cache.samurai_build == false) {
    Assert(mate_state.mate_cache.first_build, "MateCache: This is not the first build and samurai is not compiled, could be a cache error, delete `./build` folder and rebuild `./mate.c`");

    String samuraiAmalgam = s(SAMURAI_AMALGAM);
    String sourcePath = F(mate_state.arena, "%s/samurai.c", mate_state.build_directory.data);
    errno_t errFileWrite = FileWrite(sourcePath, samuraiAmalgam);
    Assert(errFileWrite == SUCCESS, "MateReadCache: failed writing samurai source code to path %s", sourcePath.data);

    String outputPath = F(mate_state.arena, "%s/samurai", mate_state.build_directory.data);
    String compileCommand = F(mate_state.arena, "%s \"%s\" -o \"%s\" -std=c99", CompilerToStr(mate_state.compiler).data, sourcePath.data, outputPath.data);

    err = RunCommand(compileCommand);
    Assert(err == SUCCESS, "MateReadCache: Error meanwhile compiling samurai at %s, if you are seeing this please make an issue at github.com/TomasBorquez/mate.h", sourcePath.data);

    LogSuccess("Successfully compiled samurai");
    mate_state.mate_cache.samurai_build = true;
    IniSet(&mate_state.cache, S("samurai-build"), S("true"));
  }
#endif

  err = IniWrite(mateCachePath, &mate_state.cache);
  Assert(err == SUCCESS, "MateReadCache: Failed writing cache, err: %d", err);
}

static bool mate_need_rebuild(void) {
  File stats = {0};
  errno_t err = FileStats(mate_state.mate_source, &stats);
  Assert(err == SUCCESS, "could not read fileStats for %s, error: %d", mate_state.mate_source.data, err);

  if (stats.modifyTime <= mate_state.mate_cache.last_build) {
    return false;
  }

  String mateCachePath = F(mate_state.arena, "%s/mate-cache.ini", mate_state.build_directory.data);
  String modifyTime = F(mate_state.arena, FMT_I64, stats.modifyTime);
  IniSet(&mate_state.cache, S("modify-time"), modifyTime);

  err = IniWrite(mateCachePath, &mate_state.cache);
  Assert(err == SUCCESS, "could not write cache for path %s, error: %d", mateCachePath.data, err);

  return true;
}

static void mate_rebuild(void) {
  if (mate_state.mate_cache.first_build || !mate_need_rebuild()) {
    return;
  }

  String mateExeNew = NormalizeExePath(mate_state.arena, F(mate_state.arena, "%s/mate-new", mate_state.build_directory.data));
  String mateExeOld = NormalizeExePath(mate_state.arena, F(mate_state.arena, "%s/mate-old", mate_state.build_directory.data));
  String mateExe = NormalizeExePath(mate_state.arena, mate_state.mate_exe);

  String compileCommand;
  if (isMSVC()) {
    compileCommand = F(mate_state.arena, "cl.exe \"%s\" /Fe:\"%s\"", mate_state.mate_source.data, mateExeNew.data);
  } else {
    compileCommand = F(mate_state.arena, "%s \"%s\" -o \"%s\"", CompilerToStr(mate_state.compiler).data, mate_state.mate_source.data, mateExeNew.data);
  }

  LogWarn("%s changed rebuilding...", mate_state.mate_source.data);
  errno_t rebuildErr = RunCommand(compileCommand);
  Assert(rebuildErr == SUCCESS, "MateRebuild: failed command %s, err: %d", compileCommand.data, rebuildErr);

  errno_t renameErr = FileRename(mateExe, mateExeOld);
  Assert(renameErr == SUCCESS, "MateRebuild: failed renaming original executable failed, err: %d", renameErr);

  renameErr = FileRename(mateExeNew, mateExe);
  Assert(renameErr == SUCCESS, "MateRebuild: failed renaming new executable into old: %d", renameErr);

  LogInfo("Rebuild finished, running %s", mateExe.data);
  exit(RunCommand(mateExe));
}

void StartBuild(void) {
  LogInit();
  if (!mate_state.init_config) {
    mate_set_default_state();
  }

  mate_state.start_time = TimeNow();

  Mkdir(mate_state.build_directory);
  mate_read_cache();
  mate_rebuild();
}

static void mate_apply_warning_flags(FlagBuilder *fb, Compiler c, FlagWarnings w) {
  static char *general[][4] = {
      [FLAG_WARNINGS_NONE] = {"w", NULL},
      [FLAG_WARNINGS_MINIMAL] = {"Wall", NULL},
      [FLAG_WARNINGS] = {"Wall", "Wextra", NULL},
      [FLAG_WARNINGS_VERBOSE] = {"Wall", "Wextra", "Wpedantic", NULL},
  };
  static char *msvc[][2] = {
      [FLAG_WARNINGS_NONE] = {"W0", NULL},
      [FLAG_WARNINGS_MINIMAL] = {"W3", NULL},
      [FLAG_WARNINGS] = {"W4", NULL},
      [FLAG_WARNINGS_VERBOSE] = {"Wall", NULL},
  };
  mate_flag_builder_add_list(fb, c == MSVC ? msvc[w] : general[w]);
}

static void mate_apply_debug_flags(FlagBuilder *fb, Compiler c, FlagDebug d) {
  static char *general[][2] = {
      [FLAG_DEBUG_MINIMAL] = {"g1", NULL},
      [FLAG_DEBUG_MEDIUM] = {"g2", NULL},
      [FLAG_DEBUG] = {"g3", NULL},
  };
  static char *msvc[][2] = {
      [FLAG_DEBUG_MINIMAL] = {"Zi", NULL},
      [FLAG_DEBUG_MEDIUM] = {"ZI", NULL},
      [FLAG_DEBUG] = {"ZI", NULL},
  };
  mate_flag_builder_add_list(fb, c == MSVC ? msvc[d] : general[d]);
}

static void mate_apply_optimization_flags(FlagBuilder *fb, Compiler c, FlagOptimization o) {
  static char *general[][2] = {
      [FLAG_OPTIMIZATION_NONE] = {"O0", NULL},
      [FLAG_OPTIMIZATION_BASIC] = {"O1", NULL},
      [FLAG_OPTIMIZATION] = {"O2", NULL},
      [FLAG_OPTIMIZATION_SIZE] = {"Os", NULL},
      [FLAG_OPTIMIZATION_AGGRESSIVE] = {"O3", NULL},
  };
  static char *msvc[][2] = {
      [FLAG_OPTIMIZATION_NONE] = {"Od", NULL},
      [FLAG_OPTIMIZATION_BASIC] = {"O1", NULL},
      [FLAG_OPTIMIZATION] = {"O2", NULL},
      [FLAG_OPTIMIZATION_SIZE] = {"O1", NULL},
      [FLAG_OPTIMIZATION_AGGRESSIVE] = {"Ox", NULL},
  };
  mate_flag_builder_add_list(fb, c == MSVC ? msvc[o] : general[o]);
}

static void mate_apply_std_flags(FlagBuilder *fb, Compiler c, FlagSTD std) {
  static char *general[][2] = {
      [FLAG_STD_C99] = {"std=c99", NULL},
      [FLAG_STD_C11] = {"std=c11", NULL},
      [FLAG_STD_C17] = {"std=c17", NULL},
      [FLAG_STD_C23] = {"std=c2x", NULL},
      [FLAG_STD_C2X] = {"std=c2x", NULL},
  };
  static char *msvc[][2] = {
      [FLAG_STD_C99] = {"std:c11", NULL}, // closest available
      [FLAG_STD_C11] = {"std:c11", NULL},
      [FLAG_STD_C17] = {"std:c17", NULL},
      [FLAG_STD_C23] = {"std:clatest", NULL},
      [FLAG_STD_C2X] = {"std:clatest", NULL},
  };
  mate_flag_builder_add_list(fb, c == MSVC ? msvc[std] : general[std]);
}

static void mate_apply_sanitizer_flags(FlagBuilder *fb, Compiler c, FlagSanitizer s) {
  static char *general[][4] = {
      [FLAG_SANITIZER_ADDRESS] = {"fsanitize=address", "fno-omit-frame-pointer", NULL},
      [FLAG_SANITIZER_UB] = {"fsanitize=undefined", NULL},
      [FLAG_SANITIZER] = {"fsanitize=address,undefined", "fno-omit-frame-pointer", NULL},
  };
  // NOTE: only on VS 2019 16.9+
  static char *msvc[][2] = {
      [FLAG_SANITIZER_ADDRESS] = {"fsanitize=address", NULL},
      [FLAG_SANITIZER_UB] = {NULL},
      [FLAG_SANITIZER] = {"fsanitize=address", NULL},
  };
  mate_flag_builder_add_list(fb, c == MSVC ? msvc[s] : general[s]);
}

static void mate_apply_error_flags(FlagBuilder *fb, Compiler c, FlagErrorFormat e) {
  static char *general[][4] = {
      [FLAG_ERROR] = {"fdiagnostics-color=always", NULL},
      [FLAG_ERROR_MAX] = {"fdiagnostics-color=always", "fdiagnostics-show-caret", "fdiagnostics-show-option", "fdiagnostics-generate-patch"},
  };
  static char *clang[][4] = {
      [FLAG_ERROR] = {"fcolor-diagnostics", NULL},
      [FLAG_ERROR_MAX] = {"fcolor-diagnostics", "fcaret-diagnostics", "fdiagnostics-fixit-info", "fdiagnostics-show-option"},
  };
  static char *msvc[][3] = {
      [FLAG_ERROR] = {"nologo", NULL},
      [FLAG_ERROR_MAX] = {"nologo", "diagnostics:caret", NULL},
  };
  char **row = c == MSVC ? msvc[e] : c == CLANG ? clang[e] : general[e];
  mate_flag_builder_add_list(fb, row);
}


Executable CreateExecutable(ExecutableOptions opts) {
  Assert(mate_state.init_config,
         "CreateExecutable: before creating an executable you must use StartBuild(), like this: \n"
         "\n"
         "StartBuild()\n"
         "{\n"
         " // ...\n"
         "}\n"
         "EndBuild()");
  Assert(opts.output != NULL,
      "CreateExecutable: failed, ExecutableOptions.output should never be null, please define the output name like this: \n"
      "\n"
      "CreateExecutable((ExecutableOptions) { .output = \"main\"});");

  String executable_output = NormalizeExePath(mate_state.arena, s(opts.output)); // TODO: first validate then normalize
  Assert(mate_is_valid_output(executable_output),
         "CreateExecutable: ExecutableOptions.output shouldn't be a path, e.g: \n"
         "\n"
         "Correct:\n"
         "CreateExecutable((ExecutableOptions){.output = \"main\"});\n"
         "\n"
         "Incorrect:\n"
         "CreateExecutable((ExecutableOptions){.output = \"./output/main\"});\n"
         "\n"
         "TIP: For custom build folder look into `CreateConfig, e.g:\n"
         "CreateConfig((MateOptions){.buildDirectory = \"./custom-dir\"});");

  Executable result = {0};
  result.output = executable_output;

  FlagBuilder fb = FlagBuilderCreate();
  if (opts.flags != NULL) StringBuilderAppend(mate_state.arena, &fb, s(opts.flags));
  if (opts.warnings != 0) mate_apply_warning_flags(&fb, mate_state.compiler, opts.warnings);
  if (opts.debug != 0) mate_apply_debug_flags(&fb, mate_state.compiler, opts.debug);
  if (opts.optimization != 0) mate_apply_optimization_flags(&fb, mate_state.compiler, opts.optimization);
  if (opts.std != 0) mate_apply_std_flags(&fb, mate_state.compiler, opts.std);
  if (opts.sanitizer != 0) mate_apply_sanitizer_flags(&fb, mate_state.compiler, opts.sanitizer);

  mate_apply_error_flags(&fb, mate_state.compiler, opts.error);
  if (!StrIsNull(fb.buffer)) result.flags = fb.buffer;

  if (opts.libs != NULL) result.libs = s(opts.libs);
  if (opts.includes != NULL) result.includes = s(opts.includes);
  if (opts.linkerFlags != NULL) result.linkerFlags = s(opts.linkerFlags);

  result.ninjaBuildPath = F(mate_state.arena, "%s/exe-%s.ninja", mate_state.build_directory.data, NormalizeExtension(mate_state.arena, result.output).data);
  return result;
}

StaticLib CreateStaticLib(StaticLibOptions opts) {
  Assert(!isMSVC(), "CreateStaticLib: MSVC compiler not yet implemented for static libraries");
  Assert(mate_state.init_config,
         "CreateStaticLib: before creating a static library you must use StartBuild(), like this: \n"
         "\n"
         "StartBuild();\n"
         "{\n"
         "CreateStaticLib(...);\n"
         " // ...\n"
         "}\n"
         "EndBuild();");
  Assert(opts.output != NULL,
      "CreateStaticLib: failed, StaticLibOptions.output should never be null, please define the output name like this: \n"
      "\n"
      "CreateStaticLib((StaticLibOptions) { .output = \"libexample\"});");

  String staticLibOutput = NormalizeStaticLibPath(mate_state.arena, s(opts.output)); // TODO: first validate then normalize
  Assert(mate_is_valid_output(s(opts.output)),
         "MateParseStaticLibOptions: failed, StaticLibOptions.output shouldn't "
         "be a path, e.g: \n"
         "\n"
         "Correct:\n"
         "CreateStaticLib((StaticLibOptions) { .output = \"libexample\"});\n"
         "\n"
         "Incorrect:\n"
         "CreateStaticLib((StaticLibOptions) { .output = "
         "\"./output/libexample.a\"});");

  StaticLib result = {0};
  result.output = NormalizePath(mate_state.arena, staticLibOutput);
  result.arFlags = S("rcs");

  FlagBuilder fb = FlagBuilderCreate();
  if (opts.flags != NULL) StringBuilderAppend(mate_state.arena, &fb, s(opts.flags));
  if (opts.warnings != 0) mate_apply_warning_flags(&fb, mate_state.compiler, opts.warnings);
  if (opts.debug != 0) mate_apply_debug_flags(&fb, mate_state.compiler, opts.debug);
  if (opts.optimization != 0) mate_apply_optimization_flags(&fb, mate_state.compiler, opts.optimization);
  if (opts.std != 0) mate_apply_std_flags(&fb, mate_state.compiler, opts.std);
  if (opts.sanitizer != 0) mate_apply_sanitizer_flags(&fb, mate_state.compiler, opts.sanitizer);

  mate_apply_error_flags(&fb, mate_state.compiler, opts.error);
  if (!StrIsNull(fb.buffer)) result.flags = fb.buffer;

  if (opts.libs != NULL) result.libs = s(opts.libs);
  if (opts.includes != NULL) result.includes = s(opts.includes);
  if (opts.arFlags != NULL) result.arFlags = s(opts.arFlags);

  result.ninjaBuildPath = F(mate_state.arena, "%s/static-%s.ninja", mate_state.build_directory.data, NormalizeExtension(mate_state.arena, result.output).data);
  return result;
}

static CreateCompileCommandsError mate_create_compile_commands(String ninjaBuildPath) {
  FILE *ninjaPipe;
  FILE *outputFile;
  char buffer[4096];
  size_t bytes_read;

  String compileCommandsPath = NormalizePath(mate_state.arena, F(mate_state.arena, "%s/compile_commands.json", mate_state.build_directory.data));
  errno_t err = fopen_s(&outputFile, compileCommandsPath.data, "w");
  if (err != SUCCESS || outputFile == NULL) {
    LogError("CreateCompileCommands: Failed to open file %s, err: %d", compileCommandsPath.data, err);
    return COMPILE_COMMANDS_FAILED_OPEN_FILE;
  }

  String compdbCommand;
  if (mate_state.mate_cache.samurai_build == true) {
    String samuraiOutputPath = F(mate_state.arena, "%s/samurai", mate_state.build_directory.data);
    compdbCommand = NormalizePath(mate_state.arena, F(mate_state.arena, "%s -f %s -t compdb compile", samuraiOutputPath.data, ninjaBuildPath.data));
  }

  if (mate_state.mate_cache.samurai_build == false) {
    compdbCommand = NormalizePath(mate_state.arena, F(mate_state.arena, "ninja -f %s -t compdb compile", ninjaBuildPath.data));
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

  LogSuccess("Successfully created %s", NormalizePathEnd(mate_state.arena, compileCommandsPath).data);
  return COMPILE_COMMANDS_SUCCESS;
}

static void mate_add_files(StringVector *sources, char **files, size_t size) {
  for (size_t i = 0; i < size; i++) {
    String currFile = s(files[i]);
    mate_add_file(sources, currFile);
  }
}

static void mate_add_file(StringVector *sources, String source) {
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
    VecPush((*sources), source);
    return;
  }

  String directory = {0};
  i32 lastSlash = -1;
  for (size_t i = 0; i < source.length; i++) {
    if (source.data[i] == '/') {
      lastSlash = i;
    }
  }

  directory = StrSlice(mate_state.arena, source, 0, lastSlash);
  String pattern = StrSlice(mate_state.arena, source, lastSlash + 1, source.length);

  StringVector files = ListDir(mate_state.arena, directory);
  for (size_t i = 0; i < files.length; i++) {
    String file = VecAt(files, i);

    if (mate_global_match(pattern, file)) {
      String finalSource = F(mate_state.arena, "%s/%s", directory.data, file.data);
      VecPush((*sources), finalSource);
    }
  }
}

static bool mate_remove_file(StringVector *sources, String source) {
  Assert(sources->length > 0, "RemoveFile: Before removing a file you must first add a file, use: AddFile()");

  for (size_t i = 0; i < sources->length; i++) {
    String *currValue = VecAtPtr((*sources), i);
    if (StrEq(source, *currValue)) {
      currValue->data = NULL;
      currValue->length = 0;
      return true;
    }
  }
  return false;
}

static void mate_install_executable(Executable *executable) {
  Assert(executable->sources.length != 0, "InstallExecutable: Executable has zero sources, add at least one with AddFile(\"./main.c\")");
  Assert(!StrIsNull(executable->output), "InstallExecutable: Before installing executable you must first CreateExecutable()");

  StringBuilder builder = StringBuilderReserve(mate_state.arena, 1024);

  // Compiler
  StringBuilderAppend(mate_state.arena, &builder, S("cc = "));
  String compiler = CompilerToStr(mate_state.compiler);
  StringBuilderAppend(mate_state.arena, &builder, compiler);
  StringBuilderAppend(mate_state.arena, &builder, S("\n"));

  // Linker flags
  if (executable->linkerFlags.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S("linker_flags = "));
    StringBuilderAppend(mate_state.arena, &builder, executable->linkerFlags);
    StringBuilderAppend(mate_state.arena, &builder, S("\n"));
  }

  // Compiler flags
  if (executable->flags.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S("flags = "));
    StringBuilderAppend(mate_state.arena, &builder, executable->flags);
    StringBuilderAppend(mate_state.arena, &builder, S("\n"));
  }

  // Include paths
  if (executable->includes.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S("includes = "));
    StringBuilderAppend(mate_state.arena, &builder, executable->includes);
    StringBuilderAppend(mate_state.arena, &builder, S("\n"));
  }

  // Libraries
  if (executable->libs.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S("libs = "));
    StringBuilderAppend(mate_state.arena, &builder, executable->libs);
    StringBuilderAppend(mate_state.arena, &builder, S("\n"));
  }

  // Current working directory
  String cwd_path = mate_convert_ninja_path(s(GetCwd()));
  StringBuilderAppend(mate_state.arena, &builder, S("cwd = "));
  StringBuilderAppend(mate_state.arena, &builder, cwd_path);
  StringBuilderAppend(mate_state.arena, &builder, S("\n"));

  // Build directory
  String build_dir_path = mate_convert_ninja_path(mate_state.build_directory);
  StringBuilderAppend(mate_state.arena, &builder, S("builddir = "));
  StringBuilderAppend(mate_state.arena, &builder, build_dir_path);
  StringBuilderAppend(mate_state.arena, &builder, S("\n"));

  // Target
  StringBuilderAppend(mate_state.arena, &builder, S("target = $builddir/"));
  StringBuilderAppend(mate_state.arena, &builder, executable->output);
  StringBuilderAppend(mate_state.arena, &builder, S("\n"));

  StringBuilderAppend(mate_state.arena, &builder, S("\n"));

  // Link command
  StringBuilderAppend(mate_state.arena, &builder, S("rule link\n  command = $cc"));
  if (executable->flags.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S(" $flags"));
  }

  if (executable->linkerFlags.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S(" $linker_flags"));
  }

  if (isMSVC()) {
    StringBuilderAppend(mate_state.arena, &builder, S(" /Fe:$out $in"));
  } else {
    StringBuilderAppend(mate_state.arena, &builder, S(" -o $out $in"));
  }

  if (executable->libs.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S(" $libs"));
  }
  StringBuilderAppend(mate_state.arena, &builder, S("\n\n"));

  // Compile command
  StringBuilderAppend(mate_state.arena, &builder, S("rule compile\n  command = $cc"));
  if (executable->flags.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S(" $flags"));
  }
  if (executable->includes.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S(" $includes"));
  }

  if (isMSVC()) {
    StringBuilderAppend(mate_state.arena, &builder, S(" /showIncludes /c $in /Fo:$out\n"));
    StringBuilderAppend(mate_state.arena, &builder, S("  deps = msvc\n\n"));
  } else {
    if (mate_state.compiler == TCC) {
      StringBuilderAppend(mate_state.arena, &builder, S(" -c $in -o $out\n"));
    }
    if (mate_state.compiler == GCC || mate_state.compiler == CLANG) {
      StringBuilderAppend(mate_state.arena, &builder, S(" -c $in -o $out -MMD -MF $depfile\n"));
      StringBuilderAppend(mate_state.arena, &builder, S("  depfile = $depfile\n"));
      StringBuilderAppend(mate_state.arena, &builder, S("  deps = gcc\n")); // INFO: since clang replicates this
    }
    StringBuilderAppend(mate_state.arena, &builder, S("\n"));
  }

  // Build individual source files
  StringVector outputFiles = mate_output_transformer(executable->sources);
  StringBuilder outputBuilder = StringBuilderCreate(mate_state.arena);
  for (size_t i = 0; i < executable->sources.length; i++) {
    String currSource = VecAt(executable->sources, i);
    if (StrIsNull(currSource)) continue;

    String outputFile = VecAt(outputFiles, i);
    String sourceFile = NormalizePathStart(mate_state.arena, currSource);

    // Source build command
    StringBuilderAppend(mate_state.arena, &builder, S("build $builddir/"));
    StringBuilderAppend(mate_state.arena, &builder, outputFile);
    StringBuilderAppend(mate_state.arena, &builder, S(": compile $cwd/"));
    StringBuilderAppend(mate_state.arena, &builder, sourceFile);
    StringBuilderAppend(mate_state.arena, &builder, S("\n"));

    if (mate_state.compiler == GCC || mate_state.compiler == CLANG) {
      String depFile = StrNew(mate_state.arena, outputFile.data);
      depFile.data[depFile.length - 1] = 'd';
      StringBuilderAppend(mate_state.arena, &builder, S("  depfile = $builddir/"));
      StringBuilderAppend(mate_state.arena, &builder, depFile);
      StringBuilderAppend(mate_state.arena, &builder, S("\n"));
    }

    // Add to output files list
    if (outputBuilder.buffer.length == 0) {
      StringBuilderAppend(mate_state.arena, &outputBuilder, S("$builddir/"));
      StringBuilderAppend(mate_state.arena, &outputBuilder, outputFile);
    } else {
      StringBuilderAppend(mate_state.arena, &outputBuilder, S(" $builddir/"));
      StringBuilderAppend(mate_state.arena, &outputBuilder, outputFile);
    }
  }

  // Build target
  StringBuilderAppend(mate_state.arena, &builder, S("build $target: link "));
  StringBuilderAppend(mate_state.arena, &builder, outputBuilder.buffer);
  StringBuilderAppend(mate_state.arena, &builder, S("\n\n"));

  // Default target
  StringBuilderAppend(mate_state.arena, &builder, S("default $target\n"));

  String ninjaBuildPath = executable->ninjaBuildPath;
  errno_t errWrite = FileWrite(ninjaBuildPath, builder.buffer);
  Assert(errWrite == SUCCESS, "InstallExecutable: failed to write build.ninja for %s, err: %d", ninjaBuildPath.data, errWrite);

  String buildCommand;
  if (mate_state.mate_cache.samurai_build) {
    String samuraiOutputPath = F(mate_state.arena, "%s/samurai", mate_state.build_directory.data);
    buildCommand = F(mate_state.arena, "%s -f %s", samuraiOutputPath.data, ninjaBuildPath.data);
  } else {
    buildCommand = F(mate_state.arena, "ninja -f %s", ninjaBuildPath.data);
  }

  i64 err = RunCommand(buildCommand);
  Assert(err == SUCCESS, "InstallExecutable: Ninja file compilation failed with code: " FMT_I64, err);

  mate_state.total_time = TimeNow() - mate_state.start_time;

#if defined(PLATFORM_WIN)
  executable->outputPath = F(mate_state.arena, "%s\\%s", mate_state.build_directory.data, executable->output.data);
#else
  executable->outputPath = F(mate_state.arena, "%s/%s", mate_state.build_directory.data, executable->output.data);
#endif
}

static void mate_install_static_lib(StaticLib *staticLib) {
  Assert(staticLib->sources.length != 0, "InstallStaticLib: Static Library has zero sources, add at least one with AddFile(\"./main.c\")");
  Assert(!StrIsNull(staticLib->output), "InstallStaticLib: Before installing static library you must first CreateStaticLib()");

  StringBuilder builder = StringBuilderReserve(mate_state.arena, 1024);

  // Compiler
  StringBuilderAppend(mate_state.arena, &builder, S("cc = "));
  String compiler = CompilerToStr(mate_state.compiler);
  StringBuilderAppend(mate_state.arena, &builder, compiler);
  StringBuilderAppend(mate_state.arena, &builder, S("\n"));

  // Archive
  StringBuilderAppend(mate_state.arena, &builder, S("ar = ar\n")); // TODO: Add different ar for MSVC

  // Compiler flags
  if (staticLib->flags.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S("flags = "));
    StringBuilderAppend(mate_state.arena, &builder, staticLib->flags);
    StringBuilderAppend(mate_state.arena, &builder, S("\n"));
  }

  // Archive flags
  if (staticLib->arFlags.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S("ar_flags = "));
    StringBuilderAppend(mate_state.arena, &builder, staticLib->arFlags);
    StringBuilderAppend(mate_state.arena, &builder, S("\n"));
  }

  // Include paths
  if (staticLib->includes.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S("includes = "));
    StringBuilderAppend(mate_state.arena, &builder, staticLib->includes);
    StringBuilderAppend(mate_state.arena, &builder, S("\n"));
  }

  // Current working directory
  String cwd_path = mate_convert_ninja_path(s(GetCwd()));
  StringBuilderAppend(mate_state.arena, &builder, S("cwd = "));
  StringBuilderAppend(mate_state.arena, &builder, cwd_path);
  StringBuilderAppend(mate_state.arena, &builder, S("\n"));

  // Build directory
  String build_dir_path = mate_convert_ninja_path(mate_state.build_directory);
  StringBuilderAppend(mate_state.arena, &builder, S("builddir = "));
  StringBuilderAppend(mate_state.arena, &builder, build_dir_path);
  StringBuilderAppend(mate_state.arena, &builder, S("\n"));

  // Target
  StringBuilderAppend(mate_state.arena, &builder, S("target = $builddir/"));
  StringBuilderAppend(mate_state.arena, &builder, staticLib->output);
  StringBuilderAppend(mate_state.arena, &builder, S("\n\n"));

  // Archive command
  StringBuilderAppend(mate_state.arena, &builder, S("rule archive\n  command = $ar $ar_flags $out $in\n\n"));

  // Compile command
  StringBuilderAppend(mate_state.arena, &builder, S("rule compile\n  command = $cc"));
  if (staticLib->flags.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S(" $flags"));
  }
  if (staticLib->includes.length > 0) {
    StringBuilderAppend(mate_state.arena, &builder, S(" $includes"));
  }

  if (mate_state.compiler == TCC) {
    StringBuilderAppend(mate_state.arena, &builder, S(" -c $in -o $out\n"));
  }
  if (mate_state.compiler == GCC || mate_state.compiler == CLANG) {
    StringBuilderAppend(mate_state.arena, &builder, S(" -c $in -o $out -MMD -MF $depfile\n"));
    StringBuilderAppend(mate_state.arena, &builder, S("  depfile = $depfile\n"));
    StringBuilderAppend(mate_state.arena, &builder, S("  deps = gcc\n")); // INFO: since clang replicates this
  }
  StringBuilderAppend(mate_state.arena, &builder, S("\n"));

  // Build individual source files
  StringVector outputFiles = mate_output_transformer(staticLib->sources);
  StringBuilder outputBuilder = StringBuilderCreate(mate_state.arena);
  for (size_t i = 0; i < staticLib->sources.length; i++) {
    String currSource = VecAt(staticLib->sources, i);
    if (StrIsNull(currSource)) continue;

    String outputFile = VecAt(outputFiles, i);
    String sourceFile = NormalizePathStart(mate_state.arena, currSource);

    // Source build command
    StringBuilderAppend(mate_state.arena, &builder, S("build $builddir/"));
    StringBuilderAppend(mate_state.arena, &builder, outputFile);
    StringBuilderAppend(mate_state.arena, &builder, S(": compile $cwd/"));
    StringBuilderAppend(mate_state.arena, &builder, sourceFile);
    StringBuilderAppend(mate_state.arena, &builder, S("\n"));

    if (mate_state.compiler == GCC || mate_state.compiler == CLANG) {
      String depFile = StrNewSize(mate_state.arena, outputFile.data, outputFile.length);
      depFile.data[depFile.length - 1] = 'd';
      StringBuilderAppend(mate_state.arena, &builder, S("  depfile = $builddir/"));
      StringBuilderAppend(mate_state.arena, &builder, depFile);
      StringBuilderAppend(mate_state.arena, &builder, S("\n"));
    }

    // Add to output files list
    if (outputBuilder.buffer.length == 0) {
      StringBuilderAppend(mate_state.arena, &outputBuilder, S("$builddir/"));
      StringBuilderAppend(mate_state.arena, &outputBuilder, outputFile);
    } else {
      StringBuilderAppend(mate_state.arena, &outputBuilder, S(" $builddir/"));
      StringBuilderAppend(mate_state.arena, &outputBuilder, outputFile);
    }
  }

  // Build target
  StringBuilderAppend(mate_state.arena, &builder, S("build $target: archive "));
  StringBuilderAppend(mate_state.arena, &builder, outputBuilder.buffer);
  StringBuilderAppend(mate_state.arena, &builder, S("\n\n"));

  // Default target
  StringBuilderAppend(mate_state.arena, &builder, S("default $target\n"));

  String ninjaBuildPath = staticLib->ninjaBuildPath;
  errno_t errWrite = FileWrite(ninjaBuildPath, builder.buffer);
  Assert(errWrite == SUCCESS, "InstallStaticLib: failed to write build-static-library.ninja for %s, err: %d", ninjaBuildPath.data, errWrite);

  String buildCommand;
  if (mate_state.mate_cache.samurai_build) {
    String samuraiOutputPath = F(mate_state.arena, "%s/samurai", mate_state.build_directory.data);
    buildCommand = F(mate_state.arena, "%s -f %s", samuraiOutputPath.data, ninjaBuildPath.data);
  } else {
    buildCommand = F(mate_state.arena, "ninja -f %s", ninjaBuildPath.data);
  }

  i64 err = RunCommand(buildCommand);
  Assert(err == SUCCESS, "InstallStaticLib: Ninja file compilation failed with code: " FMT_I64, err);

  LogSuccess("Ninja file compilation done for %s", NormalizePathEnd(mate_state.arena, ninjaBuildPath).data);
  mate_state.total_time = TimeNow() - mate_state.start_time;

#if defined(PLATFORM_WIN)
  staticLib->outputPath = F(mate_state.arena, "%s\\%s", mate_state.build_directory.data, staticLib->output.data);
#else
  staticLib->outputPath = F(mate_state.arena, "%s/%s", mate_state.build_directory.data, staticLib->output.data);
#endif
}

// TODO: use is_empty pattern from mate_flag_builder_add_string on all:
static void mate_add_library_paths(String *targetLibs, StringVector *libs) {
  StringBuilder builder = StringBuilderCreate(mate_state.arena);

  if (isMSVC() && targetLibs->length == 0) {
    StringBuilderAppend(mate_state.arena, &builder, S("/link"));
  }

  if (targetLibs->length) {
    StringBuilderAppend(mate_state.arena, &builder, *targetLibs);
  }

  if (isMSVC()) {
    // MSVC format: /LIBPATH:"path"
    for (size_t i = 0; i < libs->length; i++) {
      String currLib = VecAt((*libs), i);
      String buffer = F(mate_state.arena, " /LIBPATH:\"%s\"", currLib.data);
      StringBuilderAppend(mate_state.arena, &builder, buffer);
    }
  } else {
    // GCC/Clang format: -L"path"
    for (size_t i = 0; i < libs->length; i++) {
      String currLib = VecAt((*libs), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mate_state.arena, "-L\"%s\"", currLib.data);
        StringBuilderAppend(mate_state.arena, &builder, buffer);
        continue;
      }
      String buffer = F(mate_state.arena, " -L\"%s\"", currLib.data);
      StringBuilderAppend(mate_state.arena, &builder, buffer);
    }
  }

  *targetLibs = builder.buffer;
}

static void mate_link_system_libraries(String *targetLibs, StringVector *libs) {
  StringBuilder builder = StringBuilderCreate(mate_state.arena);

  if (isMSVC() && targetLibs->length == 0) {
    StringBuilderAppend(mate_state.arena, &builder, S("/link"));
  }

  if (targetLibs->length) {
    StringBuilderAppend(mate_state.arena, &builder, *targetLibs);
  }

  if (isMSVC()) {
    // MSVC format: library.lib
    for (size_t i = 0; i < libs->length; i++) {
      String currLib = VecAt((*libs), i);
      String buffer = F(mate_state.arena, " %s.lib", currLib.data);
      StringBuilderAppend(mate_state.arena, &builder, buffer);
    }
  } else {
    // GCC/Clang format: -llib
    for (size_t i = 0; i < libs->length; i++) {
      String currLib = VecAt((*libs), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mate_state.arena, "-l%s", currLib.data);
        StringBuilderAppend(mate_state.arena, &builder, buffer);
        continue;
      }
      String buffer = F(mate_state.arena, " -l%s", currLib.data);
      StringBuilderAppend(mate_state.arena, &builder, buffer);
    }
  }

  *targetLibs = builder.buffer;
}

static void mate_link_frameworks(String *targetLibs, StringVector *frameworks) {
  mate_link_frameworks_with_options(targetLibs, NONE, frameworks);
}

static void mate_link_frameworks_with_options(String *targetLibs, LinkFrameworkOptions options, StringVector *frameworks) {
  Assert(!isGCC(),
          "LinkFrameworks: Automatic framework linking is not supported by GCC. "
          "Use standard linking functions after adding a framework path instead.");
  Assert(isClang(), "LinkFrameworks: This function is only supported for Clang.");

  char *frameworkFlag = NULL;
  switch (options) {
    case NONE:
      frameworkFlag = "-framework";
      break;
    case NEEDED:
      frameworkFlag = "-needed_framework";
      break;
    case WEAK:
      frameworkFlag = "-weak_framework";
      break;
    default:
      Unreachable("LinkFrameworks: Invalid framework linking option provided.");
  }

  StringBuilder builder = StringBuilderCreate(mate_state.arena);

  if (targetLibs->length) {
    StringBuilderAppend(mate_state.arena, &builder, *targetLibs);
  }

  for (size_t i = 0; i < frameworks->length; i++) {
    String currFW = VecAt((*frameworks), i);
    if (i == 0 && builder.buffer.length == 0) {
      String buffer = F(mate_state.arena, "%s %s", frameworkFlag, currFW.data);
      StringBuilderAppend(mate_state.arena, &builder, buffer);
      continue;
    }
    String buffer = F(mate_state.arena, " %s %s", frameworkFlag, currFW.data);
    StringBuilderAppend(mate_state.arena, &builder, buffer);
  }

  *targetLibs = builder.buffer;
}

static void mate_add_include_paths(String *targetIncludes, StringVector *includes) {
  StringBuilder builder = StringBuilderCreate(mate_state.arena);

  if (targetIncludes->length) {
    StringBuilderAppend(mate_state.arena, &builder, *targetIncludes);
    StringBuilderAppend(mate_state.arena, &builder, S(" "));
  }

  if (isMSVC()) {
    // MSVC format: /I"path"
    for (size_t i = 0; i < includes->length; i++) {
      String currInclude = VecAt((*includes), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mate_state.arena, "/I\"%s\"", currInclude.data);
        StringBuilderAppend(mate_state.arena, &builder, buffer);
        continue;
      }
      String buffer = F(mate_state.arena, " /I\"%s\"", currInclude.data);
      StringBuilderAppend(mate_state.arena, &builder, buffer);
    }
  } else {
    // GCC/Clang format: -I"path"
    for (size_t i = 0; i < includes->length; i++) {
      String currInclude = VecAt((*includes), i);
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mate_state.arena, "-I\"%s\"", currInclude.data);
        StringBuilderAppend(mate_state.arena, &builder, buffer);
        continue;
      }
      String buffer = F(mate_state.arena, " -I\"%s\"", currInclude.data);
      StringBuilderAppend(mate_state.arena, &builder, buffer);
    }
  }

  *targetIncludes = builder.buffer;
}

static void mate_add_framework_paths(String *targetIncludes, StringVector *includes) {
  Assert(isClang() || isGCC(), "AddFrameworkPaths: This function is only supported for GCC/Clang.");

  StringBuilder builder = StringBuilderCreate(mate_state.arena);

  if (targetIncludes->length) {
    StringBuilderAppend(mate_state.arena, &builder, *targetIncludes);
    StringBuilderAppend(mate_state.arena, &builder, S(" "));
  }

  // GCC/Clang format: -F"path"
  for (size_t i = 0; i < includes->length; i++) {
    String currInclude = VecAt((*includes), i);
    if (i == 0 && builder.buffer.length == 0) {
      String buffer = F(mate_state.arena, "-F\"%s\"", currInclude.data);
      StringBuilderAppend(mate_state.arena, &builder, buffer);
      continue;
    }
    String buffer = F(mate_state.arena, " -F\"%s\"", currInclude.data);
    StringBuilderAppend(mate_state.arena, &builder, buffer);
  }

  *targetIncludes = builder.buffer;
}

void EndBuild(void) {
  LogInfo("Build took: " FMT_I64 "ms", mate_state.total_time);
  ArenaFree(mate_state.arena);
}

/* --- Flag Builder Implementation --- */
FlagBuilder FlagBuilderCreate(void) {
  return StringBuilderCreate(mate_state.arena);
}

FlagBuilder FlagBuilderReserve(size_t count) {
  return StringBuilderReserve(mate_state.arena, count);
}

static void mate_flag_builder_add_string(FlagBuilder *builder, char *flag) {
  bool is_empty = builder->buffer.length == 0;
  if (mate_state.compiler == MSVC) {
    Assert(flag[0] != '/', "FlagBuilderAdd: flag should not contain '/'. Your flag:\n%s \n\ne.g usage FlagBuilderAdd(\"W4\")", flag);
    StringBuilderAppend(mate_state.arena, builder, is_empty ? S("/") : S(" /"));
  } else {
    Assert(flag[0] != '-', "FlagBuilderAdd: flag should not contain '-'. Your flag:\n%s \n\ne.g usage FlagBuilderAdd(\"Wall\")", flag);
    StringBuilderAppend(mate_state.arena, builder, is_empty ? S("-") : S(" -"));
  }
  StringBuilderAppend(mate_state.arena, builder, s(flag));
}

static void mate_flag_builder_add_list(FlagBuilder *fb, char **flags) {
  for (; *flags != NULL; flags++) {
    mate_flag_builder_add_string(fb, *flags);
  }
}

/* --- Path Utils Implementation --- */
static bool mate_is_valid_output(String output) {
  if (output.data[0] == '.') {
    return false;
  }

  for (size_t i = 0; i < output.length; i++) {
    char curr_char = output.data[i];
    if (curr_char == '/' || curr_char == '\\') {
      return false;
    }
  }
  return true;
}

static String mate_fix_path_exe(String str) {
  String path = NormalizeExePath(mate_state.arena, str);
#if defined(PLATFORM_WIN)
  return F(mate_state.arena, "%s\\%s", GetCwd(), path.data);
#else
  return F(mate_state.arena, "%s/%s", GetCwd(), path.data);
#endif
}

static String mate_fix_path(String str) {
  String path = NormalizePath(mate_state.arena, str);
#if defined(PLATFORM_WIN)
  return F(mate_state.arena, "%s\\%s", GetCwd(), path.data);
#else
  return F(mate_state.arena, "%s/%s", GetCwd(), path.data);
#endif
}

static String mate_convert_ninja_path(String str) {
#if defined(PLATFORM_WIN)
  String copy = StrNewSize(mate_state.arena, str.data, str.length + 1);
  memmove(&copy.data[2], &copy.data[1], str.length - 1);
  copy.data[1] = '$';
  copy.data[2] = ':';
  return copy;
#else
  return str;
#endif
}

// TODO: Create something like NormalizeOutput
static StringVector mate_output_transformer(StringVector vector) {
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

      String objOutput = StrNewSize(mate_state.arena, filenameStart, filenameLength + 2);
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
    String output = StrSlice(mate_state.arena, currentExecutable, lastCharIndex + 1, currentExecutable.length);
    output.data[output.length - 1] = 'o';
    VecPush(result, output);
  }
  return result;
}

static bool mate_global_match(String pattern, String text) {
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

/* --- Utils Implementation --- */
errno_t RunCommand(String command) {
#if defined(PLATFORM_LINUX) | defined(PLATFORM_MACOS) | defined(PLATFORM_FREEBSD)
  // https://stackoverflow.com/questions/36007390/why-to-shift-bits-8-when-using-perl-system-function-to-execute-command
  return system(command.data) >> 8;
#else
  return system(command.data);
#endif
}

String CompilerToStr(Compiler compiler) {
  switch (compiler) {
  case GCC:
    return S("gcc");
  case CLANG:
    return S("clang");
  case TCC:
    return S("tcc");
  case MSVC:
    return S("cl.exe");
  default:
    Unreachable("CompilerToStr: failed, should never get here, compiler given does not exist: %d", compiler);
    return S("");
  }
}

bool isMSVC(void) {
  return mate_state.compiler == MSVC;
}

bool isGCC(void) {
  return mate_state.compiler == GCC;
}

bool isClang(void) {
  return mate_state.compiler == CLANG;
}

bool isTCC(void) {
  return mate_state.compiler == TCC;
}
