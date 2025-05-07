// NOTE: Run simply with `gcc amalgam-script.c -o amalgam && ./amalgam`
// NOTE: Compile samurai.c `gcc samurai.c -o samurai -lrt`

#define BASE_IMPLEMENTATION
#include "../../vendor/base/base.h"

typedef struct {
  Arena *arena;
  File stats;
  String buffer;
} FileResult;

String resultPath = S("./samurai.c");

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

  String initialHeaders = S("// Base include headers\n"
                            "#define _POSIX_C_SOURCE 200809L\n"
                            "#include <errno.h>\n"
                            "#include <fcntl.h>\n"
                            "#include <inttypes.h>\n"
                            "#include <stdbool.h>\n"
                            "#include <stdint.h>\n"
                            "#include <ctype.h>\n"
                            "#include <stdio.h>\n"
                            "#include <stdlib.h>\n"
                            "#include <assert.h>\n"
                            "#include <stdarg.h>\n"
                            "#include <string.h>\n"
                            "\n"
                            "// Linux specific headers\n"
                            "#include <sys/stat.h>\n"
                            "#include <poll.h>\n"
                            "#include <signal.h>\n"
                            "#include <spawn.h>\n"
                            "#include <sys/wait.h>\n"
                            "#include <time.h>\n"
                            "#include <unistd.h>\n"
                            "\n");

  errno_t err = FileWrite(&resultPath, &initialHeaders);
  if (err != SUCCESS) {
    LogError("Error on FileWrite: %d", err);
  }
}

i32 main() {
  resetAmalgamFile();

  // NOTE: Headers
  FileResult headers[13] = {
      readSource(S("./src/build.h")),
      readSource(S("./src/deps.h")),
      readSource(S("./src/env.h")),
      readSource(S("./src/graph.h")),
      readSource(S("./src/htab.h")),
      readSource(S("./src/log.h")),
      readSource(S("./src/parse.h")),
      readSource(S("./src/arg.h")),
      readSource(S("./src/scan.h")),
      readSource(S("./src/tool.h")),
      readSource(S("./src/tree.h")),
      readSource(S("./src/util.h")),
      readSource(S("./src/os.h")),
  };

  // NOTE: Implementations
  FileResult implementations[13] = {
      readSource(S("./src/build.c")),
      readSource(S("./src/deps.c")),
      readSource(S("./src/env.c")),
      readSource(S("./src/graph.c")),
      readSource(S("./src/htab.c")),
      readSource(S("./src/log.c")),
      readSource(S("./src/parse.c")),
      readSource(S("./src/samu.c")),
      readSource(S("./src/scan.c")),
      readSource(S("./src/tool.c")),
      readSource(S("./src/tree.c")),
      readSource(S("./src/util.c")),
      readSource(S("./src/os-posix.c")),
  };

  String delimiter = S("// --- INCLUDES END ---");
  for (size_t i = 0; i < 13; i++) {
    FileResult currHeader = headers[i];
    Arena *arena = ArenaCreate(currHeader.stats.size * 2);
    StringVector headerSplit = StrSplitNewLine(arena, &currHeader.buffer);
    {
      bool startInsert = false;
      String startComment = F(arena, "// --- %s ---", currHeader.stats.name);
      FileAdd(&resultPath, &startComment);
      VecForEach(headerSplit, currLine) {
        if (startInsert) {
          FileAdd(&resultPath, currLine);
          continue;
        }

        if (StrEqual(&delimiter, currLine)) {
          startInsert = true;
          continue;
        }
      }
      FileAdd(&resultPath, &S(""));
    }
    LogSuccess("Added header header %s", currHeader.stats.name);
    VecFree(headerSplit);
    ArenaFree(arena);
    ArenaFree(currHeader.arena);
  }

  for (size_t i = 0; i < 13; i++) {
    FileResult currImplementation = implementations[i];
    Arena *arena = ArenaCreate(currImplementation.stats.size * 2);
    StringVector headerSplit = StrSplitNewLine(arena, &currImplementation.buffer);
    {
      bool startInsert = false;
      String startComment = F(arena, "// --- %s ---", currImplementation.stats.name);
      FileAdd(&resultPath, &startComment);
      VecForEach(headerSplit, currLine) {
        if (startInsert) {
          FileAdd(&resultPath, currLine);
          continue;
        }

        if (StrEqual(&delimiter, currLine)) {
          startInsert = true;
          continue;
        }
      }
      FileAdd(&resultPath, &S(""));
    }
    LogSuccess("Added implementation file %s", currImplementation.stats.name);
    VecFree(headerSplit);
    ArenaFree(arena);
    ArenaFree(currImplementation.arena);
  }

  // TODO: Free resources (not really necessary but..)
  LogSuccess("Successfully created amalgam %s", resultPath.data);
}
