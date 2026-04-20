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
  FileStatsResult stats_result = FileStats(path);
  if (stats_result.error != SUCCESS) {
    Unreachable("Error on FileStats: %d", stats_result.error);
  }

  File stats = stats_result.data;
  result.stats = stats;
  result.arena = ArenaCreate(stats.size);
  FileReadResult read_result = FileRead(result.arena, path, stats.size);
  if (read_result.error != SUCCESS) {
    Unreachable("Error on FileRead: %d", read_result.error);
  }

  result.buffer = read_result.data;
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

static StringVector str_split_new_line(Arena *arena, String str) {
  Assert(!StrIsNull(str), "SplitNewLine: str should never be NULL");

  char *start = str.data;
  const char *end = str.data + str.length;
  char *curr = start;
  StringVector result = {0};

  while (curr < end) {
    char *pos = curr;

    while (pos < end && *pos != '\n') {
      pos++;
    }

    size_t len = pos - curr;

    if (pos < end && pos > curr && *(pos - 1) == '\r') {
      len--;
    }

    String currString = StrNewSize(arena, curr, len);
    VecPush(result, currString);

    if (pos < end) {
      curr = pos + 1;
    } else {
      break;
    }
  }

  return result;
}

StringBuilder builder;
#define SB_STR(str)                                       \
  do {                                                     \
    SBAdd(&builder, S(str));  \
    SBAdd(&builder, S("\n")); \
  } while (0)
#define SB_VAR(var)                                       \
  do {                                                     \
    SBAdd(&builder, var);     \
    SBAdd(&builder, S("\n")); \
  } while (0)

int main(void) {
  reset_amalgam_file();

  FileResult api_header = read_source(S("./src/api.h"));
  FileResult api_impl = read_source(S("./src/api.c"));
  FileResult vendor_base = read_source(S("./vendor/base/base.h"));
  FileResult samurai_source = read_source(S("./vendor/samurai/samurai.c"));

  size_t total_size = (api_header.stats.size + api_impl.stats.size + vendor_base.stats.size + samurai_source.stats.size) * 2;
  Arena *arena = ArenaCreate(total_size);
  Arena *builder_arena = ArenaCreate(total_size);
  builder = SBReserve(builder_arena, total_size);

  StringVector api_header_split = str_split_new_line(arena, api_header.buffer);
  StringVector api_impl_split = str_split_new_line(arena, api_impl.buffer);
  StringVector vendor_base_split = str_split_new_line(arena, vendor_base.buffer);
  StringVector samurai_source_split = str_split_new_line(arena, samurai_source.buffer);

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

  Error err = FileWrite(result_path, builder.buffer);
  if (err != SUCCESS) {
    Unreachable("FileWrite on path %s, failed with error %d", result_path.data, err);
  }

  LogSuccess("created amalgam %s", result_path.data);
}
