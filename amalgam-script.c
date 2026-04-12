#define BASE_IMPLEMENTATION
#include "vendor/base/base.h"

typedef struct {
  Arena *arena;
  File stats;
  String buffer;
} FileResult;

String result_path = S("./mate.h");

static FileResult read_source(String path) {
  FileResult result = {0};
  errno_t err = FileStats(path, &result.stats);
  if (err != SUCCESS) {
    LogError("Error on FileStats: %d", err);
    abort();
  }

  result.arena = ArenaCreate(result.stats.size);
  err = FileRead(result.arena, path, &result.buffer);
  if (err != SUCCESS) {
    LogError("Error on FileRead: %d", err);
    abort();
  }

  return result;
}

static void reset_amalgam_file(void) {
  errno_t err = FileWrite(result_path, S(""));
  if (err != SUCCESS) {
    LogError("Error on FileWrite: %d", err);
  }
}

static String escape_string(Arena *arena, String *str) {
  if (str->length == 0) {
    return S("");
  }

  char *buffer = ArenaAlloc(arena, str->length * 2);
  size_t buffer_index = 0;

  for (size_t i = 0; i < str->length; i++) {
    char c = str->data[i];

    switch (c) {
    case '\\':
      buffer[buffer_index++] = '\\';
      buffer[buffer_index++] = '\\';
      break;
    case '"':
      buffer[buffer_index++] = '\\';
      buffer[buffer_index++] = '"';
      break;
    case '\n':
      buffer[buffer_index++] = '\\';
      buffer[buffer_index++] = 'n';
      break;
    case '\r':
      buffer[buffer_index++] = '\\';
      buffer[buffer_index++] = 'r';
      break;
    case '\t':
      buffer[buffer_index++] = '\\';
      buffer[buffer_index++] = 't';
      break;
    default:
      buffer[buffer_index++] = c;
      break;
    }
  }

  String result;
  result.data = buffer;
  result.length = buffer_index;
  return result;
}

Arena *builder_arena;
StringBuilder builder;
#define SB_STR(str)                                       \
  do {                                                     \
    StringBuilderAppend(builder_arena, &builder, S(str));  \
    StringBuilderAppend(builder_arena, &builder, S("\n")); \
  } while (0)
#define SB_VAR(var)                                       \
  do {                                                     \
    StringBuilderAppend(builder_arena, &builder, var);     \
    StringBuilderAppend(builder_arena, &builder, S("\n")); \
  } while (0)

i32 main(void) {
  reset_amalgam_file();

  FileResult api_header = read_source(S("./src/api.h"));
  FileResult api_impl = read_source(S("./src/api.c"));
  FileResult vendor_base = read_source(S("./vendor/base/base.h"));
  FileResult samurai_source = read_source(S("./vendor/samurai/samurai.c"));

  size_t total_size = (api_header.stats.size + api_impl.stats.size + vendor_base.stats.size + samurai_source.stats.size) * 2;
  Arena *arena = ArenaCreate(total_size);
  builder_arena = ArenaCreate(total_size);
  builder = StringBuilderReserve(builder_arena, total_size);

  StringVector api_header_split = StrSplitNewLine(arena, api_header.buffer);
  StringVector api_impl_split = StrSplitNewLine(arena, api_impl.buffer);
  StringVector vendor_base_split = StrSplitNewLine(arena, vendor_base.buffer);
  StringVector samurai_source_split = StrSplitNewLine(arena, samurai_source.buffer);

  String delim_base = S("#include \"../vendor/base/base.h\"");
  String delim_mate = S("// --- MATE.H END ---");
  String samurai_macro = S("#define SAMURAI_AMALGAM \"SAMURAI SOURCE\"");
  VecForEach(api_header_split, api_header_curr_line) {
    if (StrEq(*api_header_curr_line, delim_base)) {
      SB_STR("// --- BASE.H START ---");

      String pragma_once = S("#pragma once");
      for (size_t j = 0; j < vendor_base_split.length; j++) {
        String curr_line = VecAt(vendor_base_split, j);
        if (StrEq(curr_line, pragma_once)) {
          continue;
        }
        SB_VAR(curr_line);
      }

      SB_STR("// --- BASE.H END ---");
      continue;
    }

    if (StrEq(*api_header_curr_line, delim_mate)) {
      String impl_comment = S("/* MIT License\n"
                                       "   mate.h - Mate Implementations start here\n"
                                       "   Guide on the `README.md`\n"
                                       "*/");
      String mate_impl_start = S("#ifdef MATE_IMPLEMENTATION");
      SB_VAR(impl_comment);
      SB_VAR(mate_impl_start);

      for (size_t j = 2; j < api_impl_split.length; j++) {
        String curr_line = VecAt(api_impl_split, j);
        SB_VAR(curr_line);
      }

      SB_STR("#endif");
      SB_STR("// --- MATE.H END ---");
      continue;
    }

    if (StrEq(*api_header_curr_line, samurai_macro)) {
      SB_STR("// clang-format off");
      SB_STR("// --- SAMURAI START ---");
      SB_STR("/* This code comes from Samurai (https://github.com/michaelforney/samurai)");
      SB_STR("*  Copyright © 2017-2021 Michael Forney");
      SB_STR("*  Licensed under ISC license, with portions under Apache License 2.0 and MIT licenses.");
      SB_STR("*  See LICENSE-SAMURAI.txt for the full license text.");
      SB_STR("*/");
      for (size_t j = 0; j < samurai_source_split.length; j++) {
        String curr_line = VecAt(samurai_source_split, j);

        if (j == 0) {
          String first_line = F(arena, "#define SAMURAI_AMALGAM \"%s\\n\"  \\", curr_line.data);
          SB_VAR(first_line);
          continue;
        }

        if (j == samurai_source_split.length - 1) {
          String escaped_line = escape_string(arena, &curr_line);
          String last_line = F(arena, "            \"%s\"", escaped_line.data);
          SB_VAR(last_line);
          continue;
        }

        if (curr_line.length == 0 || (curr_line.data[0] == '/' && curr_line.data[1] == '/')) {
          continue;
        }

        String escaped_line = escape_string(arena, &curr_line);
        String middle_line = F(arena, "            \"%s\\n\"\\", escaped_line.data);
        SB_VAR(middle_line);
      }

      SB_STR("// --- SAMURAI END ---");
      SB_STR("// clang-format on");
      continue;
    }

    SB_VAR(*api_header_curr_line);
  }

  FileWriteError result = FileWrite(result_path, builder.buffer);
  if (result != 0) {
    LogError("FileWrite on path %s, failed with error %d", result_path.data, result);
    return -result;
  }

  LogSuccess("created amalgam %s", result_path.data);
}
