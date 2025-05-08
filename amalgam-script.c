// NOTE: Run simply with `gcc amalgam-script.c -o amalgam-script && ./amalgam-script`

#define BASE_IMPLEMENTATION
#include "vendor/base/base.h"

typedef struct {
  Arena *arena;
  File stats;
  String buffer;
} FileResult;

String resultPath = S("./mate.h");

FileResult readSource(String path) {
  FileResult result = {0};
  errno_t err = FileStats(&path, &result.stats);
  if (err != SUCCESS) {
    LogError("Error on FileStats: %d", err);
    abort();
  }

  result.arena = ArenaCreate(result.stats.size);
  err = FileRead(result.arena, &path, &result.buffer);
  if (err != SUCCESS) {
    LogError("Error on FileRead: %d", err);
    abort();
  }

  return result;
}

// NOTE: Set file empty before writing
void resetAmalgamFile() {
  errno_t err = FileWrite(&resultPath, &S(""));
  if (err != SUCCESS) {
    LogError("Error on FileWrite: %d", err);
  }
}

static String escapeString(Arena *arena, String *str) {
  if (str->length == 0) {
    return S("");
  }

  char *buffer = ArenaAlloc(arena, str->length * 2);
  size_t bufferIndex = 0;

  for (size_t i = 0; i < str->length; i++) {
    char c = str->data[i];

    switch (c) {
    case '\\':
      buffer[bufferIndex++] = '\\';
      buffer[bufferIndex++] = '\\';
      break;
    case '"':
      buffer[bufferIndex++] = '\\';
      buffer[bufferIndex++] = '"';
      break;
    case '\n':
      buffer[bufferIndex++] = '\\';
      buffer[bufferIndex++] = 'n';
      break;
    case '\r':
      buffer[bufferIndex++] = '\\';
      buffer[bufferIndex++] = 'r';
      break;
    case '\t':
      buffer[bufferIndex++] = '\\';
      buffer[bufferIndex++] = 't';
      break;
    default:
      buffer[bufferIndex++] = c;
      break;
    }
  }

  String result;
  result.data = buffer;
  result.length = bufferIndex;
  return result;
}

i32 main() {
  resetAmalgamFile();

  // NOTE: Sources
  FileResult apiHeader = readSource(S("./src/api.h"));
  FileResult apiImp = readSource(S("./src/api.c"));
  FileResult vendorBase = readSource(S("./vendor/base/base.h"));
  FileResult samuraiSource = readSource(S("./vendor/samurai/samurai.c"));

  Arena *arena = ArenaCreate((apiHeader.stats.size * 2) + (apiImp.stats.size * 2) + (vendorBase.stats.size * 2) + (samuraiSource.stats.size * 2));

  StringVector apiHeaderSplit = StrSplitNewLine(arena, &apiHeader.buffer);
  StringVector apiImpSplit = StrSplitNewLine(arena, &apiImp.buffer);
  StringVector vendorBaseSplit = StrSplitNewLine(arena, &vendorBase.buffer);
  StringVector samuraiSourceSplit = StrSplitNewLine(arena, &samuraiSource.buffer);

  String delimiterBase = S("#include \"../vendor/base/base.h\"");
  String delimiterMate = S("// NOTE: Here goes MATE_IMPLEMENTATION");
  String samuraiMacro = S("#define SAMURAI_AMALGAM \"SAMURAI SOURCE\"");
  VecForEach(apiHeaderSplit, currLine) {
    if (StrEqual(currLine, &delimiterBase)) {
      FileAdd(&resultPath, &S("// --- BASE.H START ---"));

      String pragmaOnce = S("#pragma once");
      for (size_t j = 0; j < vendorBaseSplit.length; j++) {
        String *currLine = VecAt(vendorBaseSplit, j);
        if (StrEqual(currLine, &pragmaOnce)) {
          continue;
        }
        FileAdd(&resultPath, currLine);
      }

      FileAdd(&resultPath, &S("// --- BASE.H END ---"));
      continue;
    }

    if (StrEqual(currLine, &delimiterMate)) {
      String implementationComment = S("/* MIT License\n"
                                       "   mate.h - Mate Implementations start here\n"
                                       "   Guide on the `README.md`\n"
                                       "*/");
      String mateImplementationStart = S("#ifdef MATE_IMPLEMENTATION");
      FileAdd(&resultPath, &implementationComment);
      FileAdd(&resultPath, &mateImplementationStart);

      for (size_t j = 2; j < apiImpSplit.length; j++) {
        String *currLine = VecAt(apiImpSplit, j);
        FileAdd(&resultPath, currLine);
      }

      String mateImplementationEnd = S("#endif");
      FileAdd(&resultPath, &mateImplementationEnd);
      continue;
    }

    if (StrEqual(currLine, &samuraiMacro)) {
      FileAdd(&resultPath, &S("// --- SAMURAI START ---"));
      FileAdd(&resultPath, &S("/*"));
      FileAdd(&resultPath, &S("* This code comes from Samurai (https://github.com/michaelforney/samurai)"));
      FileAdd(&resultPath, &S("* Copyright © 2017-2021 Michael Forney"));
      FileAdd(&resultPath, &S("* Licensed under ISC license, with portions under Apache License 2.0 and MIT licenses."));
      FileAdd(&resultPath, &S("* See LICENSE-SAMURAI.txt for the full license text."));
      FileAdd(&resultPath, &S("*/"));
      for (size_t j = 0; j < samuraiSourceSplit.length; j++) {
        String *currLine = VecAt(samuraiSourceSplit, j);

        if (j == 0) {
          String firstLine = F(arena, "#define SAMURAI_AMALGAM \"%s\\n\"  \\", currLine->data);
          FileAdd(&resultPath, &firstLine);
          continue;
        }

        if (j == samuraiSourceSplit.length - 1) {
          String escapedLine = escapeString(arena, currLine);
          String lastLine = F(arena, "            \"%s\"", escapedLine.data);
          FileAdd(&resultPath, &lastLine);
          continue;
        }

        if (currLine->length == 0 || (currLine->data[0] == '/' && currLine->data[1] == '/')) {
          continue;
        }

        String escapedLine = escapeString(arena, currLine);
        String middleLine = F(arena, "            \"%s\\n\"\\", escapedLine.data);
        FileAdd(&resultPath, &middleLine);
      }

      FileAdd(&resultPath, &S("// --- SAMURAI END ---"));
      continue;
    }

    FileAdd(&resultPath, currLine);
  }

  // TODO: Free resources (not really necessary but..)
  LogSuccess("Successfully created amalgam %s", resultPath.data);
}

// LogInfo("before");
// String samuraiAmalgam = SAMURAI_AMALGAM;
// LogInfo("samuraiAmalgam: %d", samuraiAmalgam.length);
// LogInfo("samuraiAmalgam by us: %d", strlen(samuraiAmalgam.data));
