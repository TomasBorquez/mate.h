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
  IniParseResult parse_result = IniParse(mateCachePath);
  Assert(parse_result.error == SUCCESS, "MateReadCache: failed reading MateCache at %s, err: %s", mateCachePath.data, ErrToStr(parse_result.error).data);
  mate_state.cache = parse_result.data;

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

    errno_t run_error = RunCommand(compileCommand);
    Assert(run_error == SUCCESS, "MateReadCache: Error meanwhile compiling samurai at %s, if you are seeing this please make an issue at github.com/TomasBorquez/mate.h", sourcePath.data);

    LogSuccess("Successfully compiled samurai");
    mate_state.mate_cache.samurai_build = true;
    IniSet(&mate_state.cache, S("samurai-build"), S("true"));
  }
#endif

  errno_t write_error = IniWrite(mateCachePath, &mate_state.cache);
  Assert(write_error == SUCCESS, "MateReadCache: Failed writing cache, err: %d", write_error);
}

static bool mate_need_rebuild(void) {
  FileStatsResult stats_result = FileStats(mate_state.mate_source);
  Assert(stats_result.error == SUCCESS, "could not read fileStats for %s, error: %d", mate_state.mate_source.data, stats_result.error);

  File stats = stats_result.data;
  if ((uint64_t)stats.modifyTime <= mate_state.mate_cache.last_build) {
    return false;
  }

  String mateCachePath = F(mate_state.arena, "%s/mate-cache.ini", mate_state.build_directory.data);
  String modifyTime = F(mate_state.arena, FMT_I64, stats.modifyTime);
  IniSet(&mate_state.cache, S("modify-time"), modifyTime);

  Error write_error = IniWrite(mateCachePath, &mate_state.cache);
  Assert(write_error == SUCCESS, "could not write cache for path %s, error: %d", mateCachePath.data, write_error);

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
  errno_t rebuild_error = RunCommand(compileCommand);
  Assert(rebuild_error == SUCCESS, "MateRebuild: failed command %s, err: %d", compileCommand.data, rebuild_error);

  errno_t rename_error = FileRename(mateExe, mateExeOld);
  Assert(rename_error == SUCCESS, "MateRebuild: failed renaming original executable failed, err: %d", rename_error);

  rename_error = FileRename(mateExeNew, mateExe);
  Assert(rename_error == SUCCESS, "MateRebuild: failed renaming new executable into old: %d", rename_error);

  LogInfo("Rebuild finished, running %s", mateExe.data);
  exit(RunCommand(mateExe));
}

void StartBuild(void) {
  LogInit();
  if (!mate_state.init_config) {
    mate_set_default_state();
  }

  mate_state.start_time = TimeNow();

  Assert(Mkdir(mate_state.build_directory) == SUCCESS, "StartBuild: mkdir failed on making path %s", mate_state.build_directory.data);
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
  if (opts.flags != NULL) SBAdd(&fb, s(opts.flags));
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
  if (opts.flags != NULL) SBAdd(&fb, s(opts.flags));
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
  FILE *ninja_pipe;
  char buffer[4096];
  size_t bytes_read;

  String compdb_cmd;
  if (mate_state.mate_cache.samurai_build == true) {
    String samurai_output_path = F(mate_state.arena, "%s/samurai", mate_state.build_directory.data);
    compdb_cmd = NormalizePath(mate_state.arena, F(mate_state.arena, "%s -f %s -t compdb compile", samurai_output_path.data, ninjaBuildPath.data));
  }

  if (mate_state.mate_cache.samurai_build == false) {
    compdb_cmd = NormalizePath(mate_state.arena, F(mate_state.arena, "ninja -f %s -t compdb compile", ninjaBuildPath.data));
  }

  String compile_commands_path = NormalizePath(mate_state.arena, F(mate_state.arena, "%s/compile_commands.json", mate_state.build_directory.data));

  FILE *output_file = NULL;
#  if defined(PLATFORM_WIN)
  errno_t err = fopen_s(&output_file, compile_commands_path.data, "w");
  Assert(err == SUCCESS, "CreateCompileCommands: failed to fopen_s path %s", compile_commands_path.data);
#  else
  output_file = fopen(compile_commands_path.data, "w");
  Assert(output_file != NULL, "CreateCompileCommands: failed to open_s path %s, error: %s", compile_commands_path.data, strerror(errno));
#  endif

  ninja_pipe = popen(compdb_cmd.data, "r");
  if (ninja_pipe == NULL) {
    LogError("CreateCompileCommands: Failed to run compdb command, %s", compdb_cmd.data);
    fclose(output_file);
    return COMPILE_COMMANDS_FAILED_COMPDB;
  }

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), ninja_pipe)) > 0) {
    fwrite(buffer, 1, bytes_read, output_file);
  }

  fclose(output_file);
  errno_t status = pclose(ninja_pipe);
  if (status != SUCCESS) {
    LogError("CreateCompileCommands: Command failed with status %d\n", status);
    return COMPILE_COMMANDS_FAILED_COMPDB;
  }

  LogSuccess("Successfully created %s", NormalizePathEnd(mate_state.arena, compile_commands_path).data);
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
  int32_t lastSlash = -1;
  for (size_t i = 0; i < source.length; i++) {
    if (source.data[i] == '/') {
      lastSlash = i;
    }
  }

  directory = StrSlice(mate_state.arena, source, 0, lastSlash);
  String pattern = StrSlice(mate_state.arena, source, lastSlash + 1, source.length);

  ListDirResult list_dir_result = ListDir(mate_state.arena, directory);
  Error error = list_dir_result.error;
  Assert(list_dir_result.error == SUCCESS, "AddFile: failed at getting files in directory %s, error %s", directory.data, ErrToStr(error).data);

  StringVector files = list_dir_result.data;
  for (size_t i = 0; i < files.length; i++) {
    String file = VecAt(files, i);

    if (mate_global_match(pattern, file)) {
      String finalSource = F(mate_state.arena, "%s/%s", directory.data, file.data);
      VecPush((*sources), finalSource);
    }
  }
  VecFree(files);
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

  StringBuilder builder = SBReserve(mate_state.arena, 1024);

  { // Variables
    SBAddF(&builder, "cc = %S\n", CompilerToStr(mate_state.compiler));
    if (executable->linkerFlags.length > 0) SBAddF(&builder, "linker_flags = %S\n", executable->linkerFlags);
    if (executable->flags.length > 0)       SBAddF(&builder, "flags = %S\n", executable->flags);
    if (executable->includes.length > 0)    SBAddF(&builder, "includes = %S\n", executable->includes);
    if (executable->libs.length > 0)        SBAddF(&builder, "libs = %S\n", executable->libs);

    GetCwdResult cwd_result = GetCwd();
    Assert(cwd_result.error == SUCCESS, "InstallExecutable: failed at getting current working directory, with error %d", cwd_result.error);
    String cwd_path = mate_convert_ninja_path(cwd_result.data);

    SBAddF(&builder, "cwd = %S\n", cwd_path);
    SBAddF(&builder, "builddir = %S\n", mate_convert_ninja_path(mate_state.build_directory));
    SBAddF(&builder, "target = $builddir/%S\n\n", executable->output);
  }

  { // Link command
    SBAddS(&builder, "rule link\n"
                      "  command = $cc");
    if (executable->flags.length > 0)       SBAddS(&builder, " $flags");
    if (executable->linkerFlags.length > 0) SBAddS(&builder, " $linker_flags");

    if (!isMSVC()) SBAddS(&builder, " -o $out $in");
    else           SBAddS(&builder, " /Fe:$out $in");

    if (executable->libs.length > 0) SBAddS(&builder, " $libs");
    SBAddS(&builder, "\n");
    SBAddS(&builder, "  description = Linking C executable $target\n\n");
  }

  { // Compile command
    SBAddS(&builder, "rule compile\n"
                      "  command = $cc");
    if (executable->flags.length > 0)    SBAddS(&builder, " $flags");
    if (executable->includes.length > 0) SBAddS(&builder, " $includes");

    if (isMSVC()) {
      SBAddS(&builder, " /showIncludes /c $in /Fo:$out\n");
      SBAddS(&builder, "  deps = msvc\n\n");
    } else {
      if (mate_state.compiler != GCC && mate_state.compiler != CLANG)
        SBAddS(&builder, " -c $cwd/$in -o $out\n");
      else {
        SBAddS(&builder, " -c $cwd/$in -o $out -MMD -MF $depfile\n");
        SBAddS(&builder, "  depfile = $depfile\n");
        SBAddS(&builder, "  deps = gcc\n"); // INFO: clang replicates this
      }
      SBAddS(&builder, "  description = Building C object ./$in\n\n");
    }
  }

  { // Build individual source files
    StringBuilder output_builder = SBCreate(mate_state.arena);
    StringVector output_files = mate_output_transformer(executable->sources);
    for (size_t i = 0; i < executable->sources.length; i++) {
      String curr_source = VecAt(executable->sources, i);
      if (StrIsNull(curr_source)) continue;

      String output_file = VecAt(output_files, i);
      String source_file = NormalizePathStart(mate_state.arena, curr_source);
      SBAddF(&builder, "build $builddir/%S: compile %S\n", output_file, source_file);

      if (mate_state.compiler == GCC || mate_state.compiler == CLANG) {
        String dep_file = StrNew(mate_state.arena, output_file.data);
        dep_file.data[dep_file.length - 1] = 'd';
        SBAddF(&builder, "  depfile = $builddir/%S\n", dep_file);
      }

      bool is_empty = output_builder.buffer.length == 0;
      if (is_empty) SBAddF(&output_builder, "$builddir/%S", output_file);
      else          SBAddF(&output_builder, " $builddir/%S", output_file);
    }
    VecFree(output_files);
    SBAddF(&builder, "build $target: link %S\n\n", output_builder.buffer);
  }

  SBAddS(&builder, "default $target\n");

  // Build
  String ninja_build_path = executable->ninjaBuildPath;
  Error write_error = FileWrite(ninja_build_path, builder.buffer);
  Assert(write_error == SUCCESS, "InstallExecutable: failed to write build.ninja for %s, err: %d", ninja_build_path.data, write_error);

  String build_command;
  if (mate_state.mate_cache.samurai_build) {
    String samurai_output_path = F(mate_state.arena, "%s/samurai", mate_state.build_directory.data);
    build_command = F(mate_state.arena, "%s -f %s", samurai_output_path.data, ninja_build_path.data);
  } else {
    build_command = F(mate_state.arena, "ninja -f %s", ninja_build_path.data);
  }

  errno_t run_error = RunCommand(build_command);
  Assert(run_error == SUCCESS, "InstallExecutable: Ninja file compilation failed with code: " FMT_I32, run_error);

  mate_state.total_time = TimeNow() - mate_state.start_time;

#if defined(PLATFORM_WIN)
  executable->outputPath = F(mate_state.arena, "%s\\%s", mate_state.build_directory.data, executable->output.data);
#else
  executable->outputPath = F(mate_state.arena, "%s/%s", mate_state.build_directory.data, executable->output.data);
#endif

  VecFree(executable->sources);
}

static void mate_install_static_lib(StaticLib *static_lib) {
  Assert(static_lib->sources.length != 0, "InstallStaticLib: Static Library has zero sources, add at least one with AddFile(\"./main.c\")");
  Assert(!StrIsNull(static_lib->output), "InstallStaticLib: Before installing static library you must first CreateStaticLib()");

  StringBuilder builder = SBReserve(mate_state.arena, 1024);

  { // Variables
    SBAddF(&builder, "cc = %S\n", CompilerToStr(mate_state.compiler));
    SBAddS(&builder, "ar = ar\n"); // TODO: Add different ar for MSVC

    if (static_lib->flags.length > 0)    SBAddF(&builder, "flags = %S\n", static_lib->flags);
    if (static_lib->arFlags.length > 0)  SBAddF(&builder, "ar_flags = %S\n", static_lib->arFlags);
    if (static_lib->includes.length > 0) SBAddF(&builder, "includes = %S\n", static_lib->includes);

    GetCwdResult cwd_result = GetCwd();
    Assert(cwd_result.error == SUCCESS, "InstallStaticLib: failed at getting current working directory, with error %s", ErrToStr(cwd_result.error).data);
    String cwd_path = mate_convert_ninja_path(cwd_result.data);

    SBAddF(&builder, "cwd = %S\n", cwd_path);
    SBAddF(&builder, "builddir = %S\n", mate_convert_ninja_path(mate_state.build_directory));
    SBAddF(&builder, "target = $builddir/%S\n\n", static_lib->output);
  }

  { // Archive command
    SBAddS(&builder, "rule archive\n"
                     "  command = $ar $ar_flags $out $in\n"
                     "  description = Archiving $out\n\n");
  }

  { // Compile command
    SBAddS(&builder, "rule compile\n"
                     "  command = $cc");
    if (static_lib->flags.length > 0)    SBAddS(&builder, " $flags");
    if (static_lib->includes.length > 0) SBAddS(&builder, " $includes");

    if (mate_state.compiler != GCC && mate_state.compiler != CLANG)
      SBAddS(&builder, " -c $cwd/$in -o $out\n");
    else {
      SBAddS(&builder, " -c $cwd/$in -o $out -MMD -MF $depfile\n");
      SBAddS(&builder, "  depfile = $depfile\n");
      SBAddS(&builder, "  deps = gcc\n"); // INFO: clang replicates this
    }
    SBAddS(&builder, "  description = Building C object ./$in\n\n");
  }

  { // Build individual source files
    StringVector output_files = mate_output_transformer(static_lib->sources);
    StringBuilder output_builder = SBCreate(mate_state.arena);
    for (size_t i = 0; i < static_lib->sources.length; i++) {
      String curr_source_file = VecAt(static_lib->sources, i);
      if (StrIsNull(curr_source_file)) continue;

      String output_file = VecAt(output_files, i);
      String source_file = NormalizePathStart(mate_state.arena, curr_source_file);

      SBAddF(&builder, "build $builddir/%S: compile %S\n", output_file, source_file);

      if (mate_state.compiler == GCC || mate_state.compiler == CLANG) {
        String depFile = StrNewSize(mate_state.arena, output_file.data, output_file.length);
        depFile.data[depFile.length - 1] = 'd';
        SBAddF(&builder, "  depfile = $builddir/%S\n", depFile);
      }

      if (output_builder.buffer.length == 0) 
        SBAddF(&output_builder, "$builddir/%S", output_file);
      else
        SBAddF(&output_builder, " $builddir/%S", output_file);
    }
    VecFree(output_files);

    SBAddF(&builder, "build $target: archive %S\n\n",output_builder.buffer);
  }

  SBAddS(&builder, "default $target\n");

  String ninja_build_path = static_lib->ninjaBuildPath;
  Error write_error = FileWrite(ninja_build_path, builder.buffer);
  Assert(write_error == SUCCESS, "InstallStaticLib: failed to write build-static-library.ninja for %s, err: %d", ninja_build_path.data, write_error);

  String build_command;
  if (mate_state.mate_cache.samurai_build) {
    String samuraiOutputPath = F(mate_state.arena, "%s/samurai", mate_state.build_directory.data);
    build_command = F(mate_state.arena, "%s -f %s", samuraiOutputPath.data, ninja_build_path.data);
  } else {
    build_command = F(mate_state.arena, "ninja -f %s", ninja_build_path.data);
  }

  errno_t run_error = RunCommand(build_command);
  Assert(run_error == SUCCESS, "InstallStaticLib: Ninja file compilation failed with code: " FMT_I32, run_error);

  LogSuccess("Ninja file compilation done for %s", NormalizePathEnd(mate_state.arena, ninja_build_path).data);
  mate_state.total_time = TimeNow() - mate_state.start_time;

#if defined(PLATFORM_WIN)
  static_lib->outputPath = F(mate_state.arena, "%s\\%s", mate_state.build_directory.data, static_lib->output.data);
#else
  static_lib->outputPath = F(mate_state.arena, "%s/%s", mate_state.build_directory.data, static_lib->output.data);
#endif

  VecFree(static_lib->sources);
}

static void mate_add_library_paths(String *targetLibs, char **libs, size_t libs_size) {
  StringBuilder builder = SBCreate(mate_state.arena);

  if (isMSVC() && targetLibs->length == 0) {
    SBAddS(&builder, "/link");
  }

  if (targetLibs->length) {
    SBAdd(&builder, *targetLibs);
  }

  if (isMSVC()) {
    // MSVC format: /LIBPATH:"path"
    for (size_t i = 0; i < libs_size; i++) {
      char *curr_lib = libs[i];
      String buffer = F(mate_state.arena, " /LIBPATH:\"%s\"", curr_lib);
      SBAdd(&builder, buffer);
    }
  } else {
    // GCC/Clang format: -L"path"
    for (size_t i = 0; i < libs_size; i++) {
      char *curr_lib = libs[i];
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mate_state.arena, "-L\"%s\"", curr_lib);
        SBAdd(&builder, buffer);
        continue;
      }
      String buffer = F(mate_state.arena, " -L\"%s\"", curr_lib);
      SBAdd(&builder, buffer);
    }
  }

  *targetLibs = builder.buffer;
}

static void mate_link_system_libraries(String *targetLibs, char **libs, size_t libs_size) {
  StringBuilder builder = SBCreate(mate_state.arena);

  if (isMSVC() && targetLibs->length == 0) {
    SBAddS(&builder, "/link");
  }

  if (targetLibs->length) {
    SBAdd(&builder, *targetLibs);
  }

  if (isMSVC()) {
    // MSVC format: library.lib
    for (size_t i = 0; i < libs_size; i++) {
      char *currLib = libs[i];
      String buffer = F(mate_state.arena, " %s.lib", currLib);
      SBAdd(&builder, buffer);
    }
  } else {
    // GCC/Clang format: -llib
    for (size_t i = 0; i < libs_size; i++) {
      char *currLib = libs[i];
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mate_state.arena, "-l%s", currLib);
        SBAdd(&builder, buffer);
        continue;
      }
      String buffer = F(mate_state.arena, " -l%s", currLib);
      SBAdd(&builder, buffer);
    }
  }

  *targetLibs = builder.buffer;
}

static void mate_link_frameworks(String *targetLibs, char **frameworks, size_t frameworks_size) {
  mate_link_frameworks_with_options(targetLibs, NONE, frameworks, frameworks_size);
}

static void mate_link_frameworks_with_options(String *targetLibs, LinkFrameworkOptions options, char **frameworks, size_t frameworks_size) {
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

  StringBuilder builder = SBCreate(mate_state.arena);

  if (targetLibs->length) {
    SBAdd(&builder, *targetLibs);
  }

  for (size_t i = 0; i < frameworks_size; i++) {
    char *curr_framework = frameworks[i];
    if (i == 0 && builder.buffer.length == 0) {
      String buffer = F(mate_state.arena, "%s %s", frameworkFlag, curr_framework);
      SBAdd(&builder, buffer);
      continue;
    }
    String buffer = F(mate_state.arena, " %s %s", frameworkFlag, curr_framework);
    SBAdd(&builder, buffer);
  }

  *targetLibs = builder.buffer;
}

static void mate_add_include_paths(String *targetIncludes, char **includes, size_t includes_size) {
  StringBuilder builder = SBCreate(mate_state.arena);

  if (targetIncludes->length) {
    SBAdd(&builder, *targetIncludes);
    SBAddS(&builder, " ");
  }

  if (isMSVC()) {
    // MSVC format: /I"path"
    for (size_t i = 0; i < includes_size; i++) {
      char *curr_include = includes[i];
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mate_state.arena, "/I\"%s\"", curr_include);
        SBAdd(&builder, buffer);
        continue;
      }
      String buffer = F(mate_state.arena, " /I\"%s\"", curr_include);
      SBAdd(&builder, buffer);
    }
  } else {
    // GCC/Clang format: -I"path"
    for (size_t i = 0; i < includes_size; i++) {
      char *curr_include = includes[i];
      if (i == 0 && builder.buffer.length == 0) {
        String buffer = F(mate_state.arena, "-I\"%s\"", curr_include);
        SBAdd(&builder, buffer);
        continue;
      }
      String buffer = F(mate_state.arena, " -I\"%s\"", curr_include);
      SBAdd(&builder, buffer);
    }
  }

  *targetIncludes = builder.buffer;
}

static void mate_add_framework_paths(String *targetIncludes, char **frameworks, size_t frameworks_size) {
  Assert(isClang() || isGCC(), "AddFrameworkPaths: This function is only supported for GCC/Clang.");

  StringBuilder builder = SBCreate(mate_state.arena);

  if (targetIncludes->length) {
    SBAdd(&builder, *targetIncludes);
    SBAddS(&builder, " ");
  }

  // GCC/Clang format: -F"path"
  for (size_t i = 0; i < frameworks_size; i++) {
    char *curr_include = frameworks[i];
    if (i == 0 && builder.buffer.length == 0) {
      String buffer = F(mate_state.arena, "-F\"%s\"", curr_include);
      SBAdd(&builder, buffer);
      continue;
    }
    String buffer = F(mate_state.arena, " -F\"%s\"", curr_include);
    SBAdd(&builder, buffer);
  }

  *targetIncludes = builder.buffer;
}

void EndBuild(void) {
  LogInfo("Build took: " FMT_I64 "ms", mate_state.total_time);
  ArenaFree(mate_state.arena);
}

/* --- Flag Builder Implementation --- */
FlagBuilder FlagBuilderCreate(void) {
  return SBCreate(mate_state.arena);
}

FlagBuilder FlagBuilderReserve(size_t count) {
  return SBReserve(mate_state.arena, count);
}

static void mate_flag_builder_add_string(FlagBuilder *builder, char *flag) {
  bool is_empty = builder->buffer.length == 0;
  if (mate_state.compiler == MSVC) {
    Assert(flag[0] != '/', "FlagBuilderAdd: flag should not contain '/'. Your flag:\n%s \n\ne.g usage FlagBuilderAdd(\"W4\")", flag);
    SBAdd(builder, is_empty ? S("/") : S(" /"));
  } else {
    Assert(flag[0] != '-', "FlagBuilderAdd: flag should not contain '-'. Your flag:\n%s \n\ne.g usage FlagBuilderAdd(\"Wall\")", flag);
    SBAdd(builder, is_empty ? S("-") : S(" -"));
  }
  SBAdd(builder, s(flag));
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

  GetCwdResult cwd_result = GetCwd();
  Assert(cwd_result.error == SUCCESS, "mate_fix_path_exe: failed at getting current working directory, with error %s", ErrToStr(cwd_result.error).data);
  String cwd_path = cwd_result.data;
#if defined(PLATFORM_WIN)
  return F(mate_state.arena, "%s\\%s", cwd_path.data, path.data);
#else
  return F(mate_state.arena, "%s/%s", cwd_path.data, path.data);
#endif
}

static String mate_fix_path(String str) {
  String path = NormalizePath(mate_state.arena, str);

  GetCwdResult cwd_result = GetCwd();
  Assert(cwd_result.error == SUCCESS, "mate_fix_path: failed at getting current working directory, with error %s", ErrToStr(cwd_result.error).data);
  String cwd_path = cwd_result.data;
#if defined(PLATFORM_WIN)
  return F(mate_state.arena, "%s\\%s", cwd_path.data, path.data);
#else
  return F(mate_state.arena, "%s/%s", cwd_path.data, path.data);
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
