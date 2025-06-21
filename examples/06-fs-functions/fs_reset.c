#define MATE_IMPLEMENTATION
#include "../../mate.h"

typedef struct {
  errno_t code;
  const char *message;
} FileResetErrorMsg;

static const FileResetErrorMsg fileResetErrors[] = {
  {FILE_WRITE_SUCCESS,       "File reset (emptied) successfully: %s"},
  {FILE_WRITE_OPEN_FAILED,   "Failed to open file for reset: %s"    },
  {FILE_WRITE_ACCESS_DENIED, "Access denied when resetting file: %s"},
  {FILE_WRITE_NO_MEMORY,     "Not enough memory to reset file: %s"  },
  {FILE_WRITE_NOT_FOUND,     "File not found for reset: %s"         },
  {FILE_WRITE_DISK_FULL,     "Disk full when resetting file: %s"    },
  {FILE_WRITE_IO_ERROR,      "I/O error when resetting file: %s"    },
};

static const char *getFileResetErrorMsg(errno_t code) {
  for (size_t i = 0; i < sizeof(fileResetErrors) / sizeof(fileResetErrors[0]); ++i) {
    if (fileResetErrors[i].code == code) return fileResetErrors[i].message;
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    LogError("Usage: %s <file> [file...]", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; ++i) {
    String filePath = s(argv[i]);
    errno_t err = FileReset(filePath);
    const char *msg = getFileResetErrorMsg(err);

    if (!msg) {
      LogError("Unknown error (%d) when resetting file: %s", err, filePath.data);
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