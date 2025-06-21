#define MATE_IMPLEMENTATION
#include "../../mate.h"

typedef struct {
  errno_t code;
  const char *message;
} FileWriteErrorMsg;

static const FileWriteErrorMsg fileWriteErrors[] = {
  {FILE_WRITE_SUCCESS,       "File written successfully: %s"      },
  {FILE_WRITE_OPEN_FAILED,   "Failed to open file for writing: %s"},
  {FILE_WRITE_ACCESS_DENIED, "Access denied when writing file: %s"},
  {FILE_WRITE_NO_MEMORY,     "Not enough memory to write file: %s"},
  {FILE_WRITE_NOT_FOUND,     "File not found for writing: %s"     },
  {FILE_WRITE_DISK_FULL,     "Disk full when writing file: %s"    },
  {FILE_WRITE_IO_ERROR,      "I/O error when writing file: %s"    },
};

static const char *getFileWriteErrorMsg(errno_t code) {
  const size_t errorCount = sizeof(fileWriteErrors) / sizeof(fileWriteErrors[0]);
  for (size_t i = 0; i < errorCount; ++i) {
    if (fileWriteErrors[i].code == code) {
      return fileWriteErrors[i].message;
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 3 || (argc % 2) != 1) {
    LogError("Usage: %s <file1> <content1> [<file2> <content2> ...]", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; i += 2) {
    String filePath = s(argv[i]);
    String content = s(argv[i + 1]);

    errno_t err = FileWrite(filePath, content);
    const char *msg = getFileWriteErrorMsg(err);

    if (!msg) {
      LogError("Unknown error (%d) when writing file: %s", err, filePath.data);
      continue;
    }

    if (err == FILE_WRITE_SUCCESS) {
      LogSuccess(msg, filePath.data);
    } else {
      LogError(msg, filePath.data);
    }
    if (i + 2 < argc) LogInfo("");
  }

  return 0;
}