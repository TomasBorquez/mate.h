#define MATE_IMPLEMENTATION
#include "../../mate.h"

typedef struct {
  errno_t code;
  const char *message;
} FileReadErrorMsg;

static const FileReadErrorMsg fileReadErrors[] = {
  {FILE_READ_SUCCESS,    "File read successfully: %s"         },
  {FILE_NOT_EXIST,       "File does not exist: %s"            },
  {FILE_OPEN_FAILED,     "Failed to open file for reading: %s"},
  {FILE_GET_SIZE_FAILED, "Failed to get file size: %s"        },
  {FILE_READ_FAILED,     "Failed to read file: %s"            },
};

static const char *getFileReadErrorMsg(errno_t code) {
  const size_t errorCount = sizeof(fileReadErrors) / sizeof(fileReadErrors[0]);
  for (size_t i = 0; i < errorCount; ++i) {
    if (fileReadErrors[i].code == code) {
      return fileReadErrors[i].message;
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    LogError("Usage: %s <file> [file...]", argv[0]);
    return 1;
  }

  Arena *arena = ArenaCreate(1024);

  for (int i = 1; i < argc; ++i) {
    String filePath = s(argv[i]);
    String content = {0};

    errno_t err = FileRead(arena, filePath, &content);
    const char *msg = getFileReadErrorMsg(err);

    if (!msg) {
      LogError("Unknown error (%d) when reading file: %s", err, filePath.data);
      continue;
    }

    if (err != FILE_READ_SUCCESS) {
      LogError(msg, filePath.data);
      continue;
    }

    LogSuccess(msg, filePath.data);
    if (content.data && content.length > 0) {
      LogInfo("Content:\n%s", content.data);
    } else {
      LogInfo("Content: (empty)");
    }
  }

  ArenaFree(arena);
  return 0;
}