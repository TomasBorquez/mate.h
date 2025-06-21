#define MATE_IMPLEMENTATION
#include "../../mate.h"

typedef struct {
  errno_t code;
  const char *message;
} FileAddErrorMsg;

static const FileAddErrorMsg fileAddErrors[] = {
  {FILE_ADD_SUCCESS,       "Line appended successfully: %s"          },
  {FILE_ADD_OPEN_FAILED,   "Failed to open file for appending: %s"   },
  {FILE_ADD_ACCESS_DENIED, "Access denied when appending to file: %s"},
  {FILE_ADD_NO_MEMORY,     "Not enough memory to append to file: %s" },
  {FILE_ADD_NOT_FOUND,     "File or path not found for appending: %s"},
  {FILE_ADD_DISK_FULL,     "Disk full when appending to file: %s"    },
  {FILE_ADD_IO_ERROR,      "I/O error when appending to file: %s"    },
};

static const char *getFileAddErrorMsg(errno_t code) {
  const size_t errorCount = sizeof(fileAddErrors) / sizeof(fileAddErrors[0]);
  for (size_t i = 0; i < errorCount; ++i) {
    if (fileAddErrors[i].code == code) {
      return fileAddErrors[i].message;
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
  }

  return 0;
}