#define MATE_IMPLEMENTATION
#include "../../mate.h"

typedef struct {
  errno_t code;
  const char *message;
} FileCopyErrorMsg;

static const FileCopyErrorMsg fileCopyErrors[] = {
  {FILE_COPY_SUCCESS,              "File copied successfully: %s -> %s"},
  {FILE_COPY_SOURCE_NOT_FOUND,     "Source file not found: %s"         },
  {FILE_COPY_DEST_ACCESS_DENIED,   "Access denied to destination: %s"  },
  {FILE_COPY_SOURCE_ACCESS_DENIED, "Access denied to source: %s"       },
  {FILE_COPY_DISK_FULL,            "Disk full when copying file: %s"   },
  {FILE_COPY_IO_ERROR,             "I/O error when copying file: %s"   },
};

static const char *getFileCopyErrorMsg(errno_t code) {
  for (size_t i = 0; i < sizeof(fileCopyErrors) / sizeof(fileCopyErrors[0]); ++i) {
    if (fileCopyErrors[i].code == code) return fileCopyErrors[i].message;
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 3 || (argc % 2) != 1) {
    LogError("Usage: %s <src1> <dst1> [<src2> <dst2> ...]", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; i += 2) {
    String srcPath = s(argv[i]);
    String destPath = s(argv[i + 1]);

    errno_t err = FileCopy(srcPath, destPath);
    const char *msg = getFileCopyErrorMsg(err);

    if (!msg) {
      LogError("Unknown error (%d) when copying file: %s", err, srcPath.data);
      continue;
    }

    if (err == FILE_COPY_SUCCESS) {
      LogSuccess(msg, srcPath.data, destPath.data);
    } else {
      LogError(msg, srcPath.data);
    }
  }

  return 0;
}