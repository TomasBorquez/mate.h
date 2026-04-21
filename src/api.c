#include "api.h"

static MateConfig mate_state = {0};

/* --- Build System Implementation --- */
static void mate_set_default_state(void) {
  {
    mate_state.arena = ArenaCreate(20000 * sizeof(String));
    mate_state.compiler = GetCompiler();

    GetCwdResult cwd_result = GetCwd();
    Assert(cwd_result.error == SUCCESS, "mate_set_default_state: failed at getting current working directory, with error %s", ErrToStr(cwd_result.error).data);
    mate_state.cwd = cwd_result.data;

    mate_state.mate_exe = AbsoluteNormPathExe(S("./mate"));
    mate_state.mate_source = AbsoluteNormPath(S("./mate.c"));
    mate_state.build_directory = AbsoluteNormPath(S("./build"));
  }

  mate_state.init_config = true;
}

void CreateConfig(MateOptions options) {
  mate_set_default_state();
  if (options.compiler != 0) mate_state.compiler = options.compiler;
  if (options.mateExe != NULL) mate_state.mate_exe = AbsoluteNormPathExe(s(options.mateExe));
  if (options.mateSource != NULL) mate_state.mate_source = AbsoluteNormPath(s(options.mateSource));
  if (options.buildDirectory != NULL) mate_state.build_directory = AbsoluteNormPath(s(options.buildDirectory));
}

static void mate_read_cache(void) {
  String mate_cache_path = PathJoin(mate_state.build_directory, S("mate-cache.ini"));

  IniParseResult parse_result = IniParse(mate_cache_path);
  Assert(parse_result.error == SUCCESS, "MateReadCache: failed reading MateCache at %s, err: %s", mate_cache_path.data, ErrToStr(parse_result.error).data);
  mate_state.cache = parse_result.data;

  mate_state.mate_cache.last_build = IniGetLong(&mate_state.cache, S("modify-time"));
  if (mate_state.mate_cache.last_build == 0) {
    mate_state.mate_cache.first_build = true;
    mate_state.mate_cache.last_build = TimeNow() / 1000;

    String modify_time = F(mate_state.arena, FMT_I64, mate_state.mate_cache.last_build);
    IniSet(&mate_state.cache, S("modify-time"), modify_time);
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

    String samurai_amalgam = s(SAMURAI_AMALGAM);
    String source_path = PathJoin(mate_state.build_directory, S("samurai.c"));
    Error err_file_write = FileWrite(source_path, samurai_amalgam);
    Assert(err_file_write == SUCCESS, "MateReadCache: failed writing samurai source code to path %s", source_path.data);

    String output_path = PathJoin(mate_state.build_directory, S("samurai"));
    String compile_command = F(mate_state.arena, "%s \"%s\" -o \"%s\" -std=c99", CompilerToStr(mate_state.compiler).data, source_path.data, output_path.data);

    errno_t run_error = RunCommand(compile_command);
    Assert(run_error == SUCCESS, "MateReadCache: Error meanwhile compiling samurai at %s, if you are seeing this please make an issue at github.com/TomasBorquez/mate.h", source_path.data);

    LogSuccess("Successfully compiled samurai");
    mate_state.mate_cache.samurai_build = true;
    IniSet(&mate_state.cache, S("samurai-build"), S("true"));
  }
#endif

  errno_t write_error = IniWrite(mate_cache_path, &mate_state.cache);
  Assert(write_error == SUCCESS, "MateReadCache: Failed writing cache, err: %d", write_error);
}

static bool mate_need_rebuild(void) {
  FileStatsResult stats_result = FileStats(mate_state.mate_source);
  Assert(stats_result.error == SUCCESS, "could not read fileStats for %s, error: %d", mate_state.mate_source.data, stats_result.error);

  File stats = stats_result.data;
  if ((uint64_t)stats.modifyTime <= mate_state.mate_cache.last_build) {
    return false;
  }

  String mate_cache_path = PathJoin(mate_state.build_directory, S("mate-cache.ini"));
  String modify_time = F(mate_state.arena, FMT_I64, stats.modifyTime);
  IniSet(&mate_state.cache, S("modify-time"), modify_time);

  Error write_error = IniWrite(mate_cache_path, &mate_state.cache);
  Assert(write_error == SUCCESS, "could not write cache for path %s, error: %d", mate_cache_path.data, write_error);

  return true;
}

static void mate_rebuild(void) {
  if (mate_state.mate_cache.first_build || !mate_need_rebuild()) {
    return;
  }

  String mate_exe     = mate_state.mate_exe;
  String mate_exe_new = NormPathExe(PathJoin(mate_state.build_directory, S("mate-new")));
  String mate_exe_old = NormPathExe(PathJoin(mate_state.build_directory, S("mate-old")));

  String compile_command;
  if (isMSVC()) {
    compile_command = F(mate_state.arena, "cl.exe \"%s\" /Fe:\"%s\"", mate_state.mate_source.data, mate_exe_new.data);
  } else {
    compile_command = F(mate_state.arena, "%s \"%s\" -o \"%s\"", CompilerToStr(mate_state.compiler).data, mate_state.mate_source.data, mate_exe_new.data);
  }

  LogWarn("%s changed rebuilding...", mate_state.mate_source.data);
  errno_t rebuild_error = RunCommand(compile_command);
  Assert(rebuild_error == SUCCESS, "MateRebuild: failed command %s, err: %d", compile_command.data, rebuild_error);

  errno_t rename_error = FileRename(mate_exe, mate_exe_old);
  Assert(rename_error == SUCCESS, "MateRebuild: failed renaming original executable failed, err: %d", rename_error);

  rename_error = FileRename(mate_exe_new, mate_exe);
  Assert(rename_error == SUCCESS, "MateRebuild: failed renaming new executable into old: %d", rename_error);

  LogInfo("Rebuild finished, running %s", mate_exe.data);
  exit(RunCommand(mate_exe));
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
  static char *general[][5] = {
      [FLAG_ERROR] = {"fdiagnostics-color=always", NULL},
      [FLAG_ERROR_MAX] = {"fdiagnostics-color=always", "fdiagnostics-show-caret", "fdiagnostics-show-option", "fdiagnostics-generate-patch", NULL},
  };
  static char *clang[][5] = {
      [FLAG_ERROR] = {"fcolor-diagnostics", NULL},
      [FLAG_ERROR_MAX] = {"fcolor-diagnostics", "fcaret-diagnostics", "fdiagnostics-fixit-info", "fdiagnostics-show-option", NULL},
  };
  static char *msvc[][3] = {
      [FLAG_ERROR] = {"nologo", NULL},
      [FLAG_ERROR_MAX] = {"nologo", "diagnostics:caret", NULL},
  };
  char **row = c == MSVC ? msvc[e] : c == CLANG ? clang[e] : general[e];
  mate_flag_builder_add_list(fb, row);
}

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
  Assert(mate_is_valid_output(s(opts.output)),
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
  result.output = NormPathExe(s(opts.output));

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

  String build_file_name = F(mate_state.arena, "exe-%s.ninja", PathStem(result.output).data);
  result.ninjaBuildPath = PathJoin(mate_state.build_directory, build_file_name);
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

  Assert(mate_is_valid_output(s(opts.output)),
         "MateParseStaticLibOptions: failed, StaticLibOptions.output shouldn't "
         "be a path, e.g: \n"
         "\n"
         "Correct:\n"
         "CreateStaticLib((StaticLibOptions) { .output = \"libexample\"});\n"
         "\n"
         "Incorrect:\n"
         "CreateStaticLib((StaticLibOptions) { .output = \"./output/libexample.a\"});");

  StaticLib result = {0};
  result.output = NormPathStaticLib(s(opts.output));
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

  String build_file_name = F(mate_state.arena, "static-%s.ninja", PathStem(result.output).data);
  result.ninjaBuildPath = PathJoin(mate_state.build_directory, build_file_name);
  return result;
}

static CreateCompileCommandsError mate_create_compile_commands(String ninjaBuildPath) {
  FILE *ninja_pipe;
  char buffer[4096];
  size_t bytes_read;

  String compdb_cmd = {0};
  if (mate_state.mate_cache.samurai_build == true) {
    String samurai_output_path = PathJoin(mate_state.build_directory, S("samurai"));
    compdb_cmd = NormPath(F(mate_state.arena, "%s -f %s -t compdb compile", samurai_output_path.data, ninjaBuildPath.data));
  }

  if (mate_state.mate_cache.samurai_build == false) {
    compdb_cmd = NormPath(F(mate_state.arena, "ninja -f %s -t compdb compile", ninjaBuildPath.data));
  }

  String compile_commands_path = NormPath(PathJoin(mate_state.build_directory, S("compile_commands.json")));

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

  LogSuccess("Successfully created %s", NormPathEnd(compile_commands_path).data);
  return COMPILE_COMMANDS_SUCCESS;
}

static void mate_add_files(StringVector *sources, char **files, size_t size) {
  for (size_t i = 0; i < size; i++) {
    String currFile = s(files[i]);
    mate_add_file(sources, currFile);
  }
}

static bool mate_global_match(String pattern, String text) {
  if (pattern.length == 1 && pattern.data[0] == '*') {
    return true;
  }

  size_t p = 0;
  size_t t = 0;
  size_t star_p = -1;
  size_t start_t = -1;
  while (t < text.length) {
    if (p < pattern.length && pattern.data[p] == text.data[t]) {
      p++;
      t++;
    } else if (p < pattern.length && pattern.data[p] == '*') {
      star_p = p;
      start_t = t;
      p++;
    } else if (star_p != (size_t)-1) {
      p = star_p + 1;
      t = ++start_t;
    } else {
      return false;
    }
  }

  while (p < pattern.length && pattern.data[p] == '*') {
    p++;
  }

  return p == pattern.length;
}

static void mate_add_file(StringVector *sources, String source) {
  bool is_glob = false;
  for (size_t i = 0; i < source.length; i++) {
    if (source.data[i] == '*') {
      is_glob = true;
      break;
    }
  }

  Assert(source.length > 2 && source.data[0] == '.' && source.data[1] == '/',
         "AddFile: failed to a add file, to add a file it has to "
         "contain the relative path, for example AddFile(\"./main.c\")");

  Assert(source.data[source.length - 1] != '/',
         "AddFile: failed to add a file, you can't add a slash at the end of a path.\n"
         "For example, valid: AddFile(\"./main.c\"), invalid: AddFile(\"./main.c/\")");

  if (!is_glob) {
    VecPush((*sources), source);
    return;
  }

  String directory = {0};
  int32_t last_slash = -1;
  for (size_t i = 0; i < source.length; i++) {
    if (source.data[i] == '/') {
      last_slash = i;
    }
  }

  directory = StrSlice(mate_state.arena, source, 0, last_slash);
  String pattern = StrSlice(mate_state.arena, source, last_slash + 1, source.length);

  ListDirResult list_dir_result = ListDir(mate_state.arena, directory);
  Error error = list_dir_result.error;
  Assert(list_dir_result.error == SUCCESS, "AddFile: failed at getting files in directory %s, error %s", directory.data, ErrToStr(error).data);

  StringVector files = list_dir_result.data;
  for (size_t i = 0; i < files.length; i++) {
    String file = VecAt(files, i);

    if (mate_global_match(pattern, file)) {
      String final_source = PathJoin(directory, file);
      VecPush((*sources), final_source);
    }
  }
  VecFree(files);
}

static bool mate_remove_file(StringVector *sources, String source) {
  Assert(sources->length > 0, "RemoveFile: Before removing a file you must first add a file, use: AddFile()");

  for (size_t i = 0; i < sources->length; i++) {
    String *curr_val = VecAtPtr((*sources), i);
    if (StrEq(source, *curr_val)) {
      curr_val->data = NULL;
      curr_val->length = 0;
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

    SBAddF(&builder, "cwd = %S\n", NormPathNinja(mate_state.cwd));
    SBAddF(&builder, "builddir = %S\n", NormPathNinja(mate_state.build_directory));
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
    for (size_t i = 0; i < executable->sources.length; i++) {
      String curr_source = VecAt(executable->sources, i);
      if (StrIsNull(curr_source)) continue;

      String output_file = NormPathOutput(curr_source);
      String source_file = NormPathStart(curr_source);
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
    SBAddF(&builder, "build $target: link %S\n\n", output_builder.buffer);
  }

  SBAddS(&builder, "default $target\n");

  // Build
  String ninja_build_path = executable->ninjaBuildPath;
  Error write_error = FileWrite(ninja_build_path, builder.buffer);
  Assert(write_error == SUCCESS, "InstallExecutable: failed to write build.ninja for %s, err: %d", ninja_build_path.data, write_error);

  String build_command;
  if (mate_state.mate_cache.samurai_build) {
    String samurai_output_path = PathJoin(mate_state.build_directory, S("samurai"));
    build_command = F(mate_state.arena, "%s -f %s", samurai_output_path.data, ninja_build_path.data);
  } else {
    build_command = F(mate_state.arena, "ninja -f %s", ninja_build_path.data);
  }

  errno_t run_error = RunCommand(build_command);
  Assert(run_error == SUCCESS, "InstallExecutable: Ninja file compilation failed with code: " FMT_I32, run_error);

  mate_state.total_time = TimeNow() - mate_state.start_time;
  executable->outputPath = PathJoin(mate_state.build_directory, executable->output);

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

    SBAddF(&builder, "cwd = %S\n", NormPathNinja(mate_state.cwd));
    SBAddF(&builder, "builddir = %S\n", NormPathNinja(mate_state.build_directory));
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
    StringBuilder output_builder = SBCreate(mate_state.arena);
    for (size_t i = 0; i < static_lib->sources.length; i++) {
      String curr_source_file = VecAt(static_lib->sources, i);
      if (StrIsNull(curr_source_file)) continue;

      String output_file = NormPathOutput(curr_source_file);
      String source_file = NormPathStart(curr_source_file);

      SBAddF(&builder, "build $builddir/%S: compile %S\n", output_file, source_file);

      if (mate_state.compiler == GCC || mate_state.compiler == CLANG) {
        String depFile = StrNewSize(mate_state.arena, output_file.data, output_file.length);
        depFile.data[depFile.length - 1] = 'd';
        SBAddF(&builder, "  depfile = $builddir/%S\n", depFile);
      }

      if (output_builder.buffer.length == 0) {
        SBAddF(&output_builder, "$builddir/%S", output_file);
        continue;
      }

      SBAddF(&output_builder, " $builddir/%S", output_file);
    }

    SBAddF(&builder, "build $target: archive %S\n\n",output_builder.buffer);
  }

  SBAddS(&builder, "default $target\n");

  String ninja_build_path = static_lib->ninjaBuildPath;
  Error write_error = FileWrite(ninja_build_path, builder.buffer);
  Assert(write_error == SUCCESS, "InstallStaticLib: failed to write build-static-library.ninja for %s, err: %d", ninja_build_path.data, write_error);

  String build_command;
  if (mate_state.mate_cache.samurai_build) {
    String samurai_output_path = PathJoin(mate_state.build_directory, S("samurai"));
    build_command = F(mate_state.arena, "%s -f %s", samurai_output_path.data, ninja_build_path.data);
  } else {
    build_command = F(mate_state.arena, "ninja -f %s", ninja_build_path.data);
  }

  errno_t run_error = RunCommand(build_command);
  Assert(run_error == SUCCESS, "InstallStaticLib: Ninja file compilation failed with code: " FMT_I32, run_error);

  mate_state.total_time = TimeNow() - mate_state.start_time;
  static_lib->outputPath = PathJoin(mate_state.build_directory, static_lib->output);

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
      SBAddF(&builder, " /LIBPATH:\"%s\"", curr_lib);
    }
  } else {
    // GCC/Clang format: -L"path"
    for (size_t i = 0; i < libs_size; i++) {
      char *curr_lib = libs[i];
      if (builder.buffer.length == 0) {
        SBAddF(&builder, "-L\"%s\"", curr_lib);
        continue;
      }
      SBAddF(&builder, " -L\"%s\"", curr_lib);
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
      char *curr_lib = libs[i];
      SBAddF(&builder, " %s.lib", curr_lib);
    }
  } else {
    // GCC/Clang format: -llib
    for (size_t i = 0; i < libs_size; i++) {
      char *curr_lib = libs[i];
      if (builder.buffer.length == 0) {
        SBAddF(&builder, "-l%s", curr_lib);
        continue;
      }
      SBAddF(&builder, " -l%s", curr_lib);
    }
  }

  *targetLibs = builder.buffer;
}

static void mate_link_frameworks_with_options(String *targetLibs, LinkFrameworkOptions options, char **frameworks, size_t frameworks_size) {
  Assert(isClang(), "LinkFrameworks: This function is only supported for Clang.");

  char *framework_flag = NULL;
  switch (options) {
    case NONE:
      framework_flag = "-framework";
      break;
    case NEEDED:
      framework_flag = "-needed_framework";
      break;
    case WEAK:
      framework_flag = "-weak_framework";
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
    if (builder.buffer.length == 0) {
      SBAddF(&builder, "%s %s", framework_flag, curr_framework);
      continue;
    }
    SBAddF(&builder, " %s %s", framework_flag, curr_framework);
  }

  *targetLibs = builder.buffer;
}

static void mate_link_frameworks(String *targetLibs, char **frameworks, size_t frameworks_size) {
  mate_link_frameworks_with_options(targetLibs, NONE, frameworks, frameworks_size);
}

static void mate_add_include_paths(String *target_includes, char **includes, size_t includes_size) {
  StringBuilder builder = SBCreate(mate_state.arena);

  if (target_includes->length) {
    SBAdd(&builder, *target_includes);
  }

  if (isMSVC()) {
    // MSVC format: /I"path"
    for (size_t i = 0; i < includes_size; i++) {
      char *curr_include = includes[i];
      if (builder.buffer.length == 0) {
        SBAddF(&builder, "/I\"%s\"", curr_include);
        continue;
      }
      SBAddF(&builder, " /I\"%s\"", curr_include);
    }
  } else {
    // GCC/Clang format: -I"path"
    for (size_t i = 0; i < includes_size; i++) {
      char *curr_include = includes[i];
      if (builder.buffer.length == 0) {
        SBAddF(&builder, "-I\"%s\"", curr_include);
        continue;
      }
      SBAddF(&builder, " -I\"%s\"", curr_include);
    }
  }

  *target_includes = builder.buffer;
}

static void mate_add_framework_paths(String *target_includes, char **frameworks, size_t frameworks_size) {
  Assert(isClang() || isGCC(), "AddFrameworkPaths: This function is only supported for GCC/Clang.");

  StringBuilder builder = SBCreate(mate_state.arena);

  if (target_includes->length) {
    SBAdd(&builder, *target_includes);
  }

  // GCC/Clang format: -F"path"
  for (size_t i = 0; i < frameworks_size; i++) {
    char *curr_include = frameworks[i];
    if (builder.buffer.length == 0) {
      SBAddF(&builder, "-F\"%s\"", curr_include);
      continue;
    }
    SBAddF(&builder, " -F\"%s\"", curr_include);
  }

  *target_includes = builder.buffer;
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
static String mate_path_strip_dot_slash(String path) {
  if (path.length >= 2 && path.data[0] == '.' && (path.data[1] == '/' || path.data[1] == '\\')) {
    return (String){ .data = path.data + 2, .length = path.length - 2 };
  }
  return path;
}

static String mate_path_get_ext(String path) {
  size_t last_dot = 0, filename_start = 0;
  for (size_t i = 0; i < path.length; i++) {
    if (path.data[i] == '/' || path.data[i] == '\\') filename_start = i + 1;
    if (path.data[i] == '.') last_dot = i;
  }
  if (last_dot <= filename_start) return S("");
  return (String){ .data = path.data + last_dot, .length = path.length - last_dot };
}

static String mate_path_strip_ext(String path) {
  String ext = mate_path_get_ext(path);
  return (String){ .data = path.data, .length = path.length - ext.length };
}

static String mate_path_fix_slashes(String path) {
#  if defined(PLATFORM_WIN)
  for (size_t i = 0; i < path.length; i++) {
    if (path.data[i] == '/') {
      path.data[i] = '\\';
    }
  }
#  else
  for (size_t i = 0; i < path.length; i++) {
    if (path.data[i] == '\\') {
      path.data[i] = '/';
    }
  }
#  endif
  return path;
}

static String mate_path_with_platform_ext(Arena *arena, String path, String unix_ext, String win_ext) {
  String result = mate_path_strip_dot_slash(path);
  String current_ext = mate_path_get_ext(result);
  String target_ext = (GetPlatform() == WINDOWS) ? win_ext : unix_ext;

  if (StrEq(current_ext, unix_ext) || StrEq(current_ext, win_ext)) {
    result = mate_path_strip_ext(result);
  }

  result = target_ext.length > 0
    ? StrConcat(arena, StrNewSize(arena, result.data, result.length), target_ext)
    : StrNewSize(arena, result.data, result.length);

  return mate_path_fix_slashes(result);
}

String PathJoin(String base, String tail) {
#if defined(PLATFORM_WIN)
  return F(mate_state.arena, "%s\\%s", base.data, tail.data);
#else
  return F(mate_state.arena, "%s/%s", base.data, tail.data);
#endif
}

String PathStem(String path) {
  String stripped = mate_path_strip_dot_slash(path);
  String no_ext = mate_path_strip_ext(stripped);
  size_t filename_start = 0;
  for (size_t i = 0; i < no_ext.length; i++) {
    if (no_ext.data[i] == '/' || no_ext.data[i] == '\\') filename_start = i + 1;
  }

  String result = StrNewSize(mate_state.arena, no_ext.data + filename_start, no_ext.length - filename_start);
  return mate_path_fix_slashes(result);
}

String NormPath(String path) {
  String stripped = mate_path_strip_dot_slash(path);
  return mate_path_fix_slashes(StrNewSize(mate_state.arena, stripped.data, stripped.length));
}

String NormPathStart(String path) {
  String stripped = mate_path_strip_dot_slash(path);
  return StrNewSize(mate_state.arena, stripped.data, stripped.length);
}

String NormPathEnd(String path) {
  size_t last_slash = 0;
  for (size_t i = 0; i < path.length; i++) {
    if (path.data[i] == '/' || path.data[i] == '\\') {
      last_slash = i + 1;
    }
  }

  return StrNewSize(mate_state.arena, path.data + last_slash, path.length - last_slash);
}

String NormPathExe(String str) {
  return mate_path_with_platform_ext(mate_state.arena, str, S(""), S(".exe"));
}

String NormPathStaticLib(String str) {
  return mate_path_with_platform_ext(mate_state.arena, str, S(".a"), S(".lib"));
}

String NormPathNinja(String str) {
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

String NormPathOutput(String str) {
  String ext = isMSVC() ? S(".obj") : S(".o");
  String stem = PathStem(str);
  Assert(stem.length > 0, "NormPathOutput: failed to get stem from %s", str.data);
  return StrConcat(mate_state.arena, stem, ext);
}

String AbsoluteNormPath(String str) {
  return PathJoin(mate_state.cwd, NormPath(str));
}

String AbsoluteNormPathExe(String str) {
  return PathJoin(mate_state.cwd, NormPathExe(str));
}

String AbsoluteNormPathStaticLib(String str) {
  return PathJoin(mate_state.cwd, NormPathStaticLib(str));
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
