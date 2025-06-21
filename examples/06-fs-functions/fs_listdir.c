#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(int argc, char **argv) {
  Arena *arena = ArenaCreate(1024);

  if (argc < 2) {
    LogError("Usage: %s <dir> [dir...]", argv[0]);
    ArenaFree(arena);
    return 1;
  }

  for (int i = 1; i < argc; ++i) {
    String dirPath = s(argv[i]);
    StringVector entries = ListDir(arena, dirPath);

    if (entries.length == 0) {
      LogWarn("No entries found or failed to list directory: %s", dirPath.data);
    } else {
      LogSuccess("Directory listing for: %s", dirPath.data);

      for (size_t j = 0; j < entries.length; ++j) {
        String file = entries.data[j];
        LogInfo("  %s", file.data);
      }
    }
  }

  ArenaFree(arena);
  return 0;
}