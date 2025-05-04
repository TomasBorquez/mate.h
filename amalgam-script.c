// NOTE: Run simply with `gcc amalgam-script.c -o amalgam-script && ./amalgam-script`

#define BASE_IMPLEMENTATION
#include "vendor/base/base.h"

typedef struct {
  Arena arena;
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

  result.arena = ArenaInit(result.stats.size);
  err = FileRead(&result.arena, &path, &result.buffer);
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

i32 main() {
  resetAmalgamFile();

  // NOTE: Sources
  FileResult apiHeader = readSource(S("./src/api.h"));
  FileResult apiImp = readSource(S("./src/api.c"));
  FileResult vendorBase = readSource(S("./vendor/base/base.h"));

  Arena arena = ArenaInit((apiHeader.stats.size * 2) + (apiImp.stats.size * 2) + (vendorBase.stats.size * 2));

  StringVector apiHeaderSplit = StrSplitNewLine(&arena, &apiHeader.buffer);
  StringVector apiImpSplit = StrSplitNewLine(&arena, &apiImp.buffer);
  StringVector vendorBaseSplit = StrSplitNewLine(&arena, &vendorBase.buffer);

  String delimiterBase = S("#include \"../vendor/base/base.h\"");
  String delimiterMate = S("// NOTE: Here goes MATE_IMPLEMENTATION");
  // TODO: Change to `VecForEach()`
  for (size_t i = 0; i < apiHeaderSplit.length; i++) {
    String *currLine = VecAt(apiHeaderSplit, i);

    if (StrEqual(currLine, &delimiterBase)) {
      FileAdd(&resultPath, &S("// --- BASE.H START ---"));

      String pragmaOnce = S("#pragma once");
      for (size_t j = 0; j < vendorBaseSplit.length; j++) {
        currLine = VecAt(vendorBaseSplit, j);
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
        currLine = VecAt(apiImpSplit, j);
        FileAdd(&resultPath, currLine);
      }

      String mateImplementationEnd = S("#endif");
      FileAdd(&resultPath, &mateImplementationEnd);
      continue;
    }

    FileAdd(&resultPath, currLine);
  }

  // TODO: Free resources (not really necessary but..)
  LogSuccess("Successfully created amalgam %s", resultPath.data);
}
