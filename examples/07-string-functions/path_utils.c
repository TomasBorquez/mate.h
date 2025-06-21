#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(void) {
  Arena *arena = ArenaCreate(256);

  String paths[] = {
    S("foo\\bar\\baz.txt"),
    S("./foo/bar/../baz/"),
    S("libfoo.a"),
    S("C:\\Program Files\\app\\app.exe"),
    S("/usr/local/bin/script.sh"),
    S("relative/path/file"),
    S("archive.tar.gz"),
    S("no_extension_file"),
    S("./folder/.hiddenfile"),
  };
  size_t path_count = sizeof(paths) / sizeof(paths[0]);

  LogInfo("=== Path Normalization Examples ===");
  for (size_t i = 0; i < path_count; ++i) {
    String path = paths[i];
    LogInfo("Path %zu: %.*s", i + 1, (int)path.length, path.data);
    LogInfo("  NormalizePath:         %.*s", (int)NormalizePath(arena, path).length, NormalizePath(arena, path).data);
    LogInfo("  NormalizeExePath:      %.*s", (int)NormalizeExePath(arena, path).length, NormalizeExePath(arena, path).data);
    LogInfo("  NormalizeExtension:    %.*s", (int)NormalizeExtension(arena, path).length, NormalizeExtension(arena, path).data);
    LogInfo("  NormalizeStaticLibPath:%.*s", (int)NormalizeStaticLibPath(arena, path).length, NormalizeStaticLibPath(arena, path).data);
    LogInfo("  NormalizePathStart:    %.*s", (int)NormalizePathStart(arena, path).length, NormalizePathStart(arena, path).data);
    LogInfo("  NormalizePathEnd:      %.*s", (int)NormalizePathEnd(arena, path).length, NormalizePathEnd(arena, path).data);
    LogInfo("");
  }

  ArenaFree(arena);
  return 0;
}