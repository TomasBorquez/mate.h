#define MATE_IMPLEMENTATION
#include "../../mate.h"

typedef struct {
  errno_t code;
  const char *message;
} FileRenameErrorMsg;

static const FileRenameErrorMsg fileRenameErrors[] = {
  {FILE_RENAME_SUCCESS,       "File renamed successfully: %s -> %s" },
  {FILE_RENAME_ACCESS_DENIED, "Access denied when renaming file: %s"},
  {FILE_RENAME_NOT_FOUND,     "File not found for renaming: %s"     },
  {FILE_RENAME_IO_ERROR,      "I/O error when renaming file: %s"    },
};

static const char *getFileRenameErrorMsg(errno_t code) {
  const size_t errorCount = sizeof(fileRenameErrors) / sizeof(fileRenameErrors[0]);
  for (size_t i = 0; i < errorCount; ++i) {
    if (fileRenameErrors[i].code == code) {
      return fileRenameErrors[i].message;
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 3 || (argc % 2) != 1) {
    LogError("Usage: %s <old1> <new1> [<old2> <new2> ...]", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; i += 2) {
    String oldPath = s(argv[i]);
    String newPath = s(argv[i + 1]);

    errno_t err = FileRename(oldPath, newPath);
    const char *msg = getFileRenameErrorMsg(err);

    if (!msg) {
      LogError("Unknown error (%d) when renaming file: %s", err, oldPath.data);
      continue;
    }

    if (err == FILE_RENAME_SUCCESS) {
      LogSuccess(msg, oldPath.data, newPath.data);
    } else {
      LogError(msg, oldPath.data);
    }
  }

  return 0;
}