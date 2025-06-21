#define MATE_IMPLEMENTATION
#include "../../mate.h"

typedef struct {
  errno_t code;
  const char *message;
} FileDeleteErrorMsg;

static const FileDeleteErrorMsg fileDeleteErrors[] = {
  {FILE_DELETE_SUCCESS,       "File deleted successfully: %s"       },
  {FILE_DELETE_ACCESS_DENIED, "Access denied when deleting file: %s"},
  {FILE_DELETE_NOT_FOUND,     "File not found for deletion: %s"     },
  {FILE_DELETE_IO_ERROR,      "I/O error when deleting file: %s"    },
};

static const char *getFileDeleteErrorMsg(errno_t code) {
  const size_t errorCount = sizeof(fileDeleteErrors) / sizeof(fileDeleteErrors[0]);
  for (size_t i = 0; i < errorCount; ++i) {
    if (fileDeleteErrors[i].code == code) {
      return fileDeleteErrors[i].message;
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    LogError("Usage: %s <files...>", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; ++i) {
    String filePath = s(argv[i]);
    errno_t err = FileDelete(filePath);
    const char *msg = getFileDeleteErrorMsg(err);

    if (!msg) {
      LogError("Unknown error (%d) when deleting file: %s", err, filePath.data);
    }

    if (err == FILE_DELETE_SUCCESS) {
      LogSuccess(msg, filePath.data);
    } else {
      LogError(msg, filePath.data);
    }
  }

  return 0;
}