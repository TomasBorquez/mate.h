#define MATE_IMPLEMENTATION
#include "../../mate.h"
#include <math.h>

typedef struct {
  errno_t code;
  const char *message;
} FileStatsErrorMsg;

static const FileStatsErrorMsg fileStatsErrors[] = {
  {FILE_STATS_SUCCESS,         "File stats retrieved: %s"         },
  {FILE_GET_ATTRIBUTES_FAILED, "Failed to get file attributes: %s"},
  {FILE_STATS_FILE_NOT_EXIST,  "File does not exist: %s"          },
};

static const char *getFileStatsErrorMsg(errno_t code) {
  for (size_t i = 0; i < sizeof(fileStatsErrors) / sizeof(fileStatsErrors[0]); ++i) {
    if (fileStatsErrors[i].code == code) return fileStatsErrors[i].message;
  }
  return NULL;
}

static void printTime(const char *label, i64 timestamp) {
  if (timestamp <= 0) {
    LogInfo("%s: (unknown)", label);
    return;
  }
  time_t t = (time_t)timestamp;
  struct tm *tm_info = localtime(&t);
  char buf[64];
  if (tm_info && strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info)) {
    LogInfo("%s: %s (%lld)", label, buf, timestamp);
  } else {
    LogInfo("%s: %lld", label, timestamp);
  }
}

static void printSize(i64 bytes) {
  static const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
  int max_unit = 4;
  double size = (double)bytes;
  int unit = 0;
  double display = size;

  while (display >= 1024.0 && unit < max_unit) {
    display /= 1024.0;
    unit++;
  }

  if (unit == 0) {
    LogInfo("Size: %lld B", bytes);
  } else {
    LogInfo("Size: %.2f %s", display, units[unit]);
    for (int i = unit - 1; i >= 0; --i) {
      double val = size / pow(1024.0, i);
      LogInfo("  = %.2f %s", val, units[i]);
    }
    LogInfo("  = %lld B", bytes);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    LogError("Usage: %s <file> [file...]", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; ++i) {
    String filePath = s(argv[i]);
    File fileInfo = {0};

    errno_t err = FileStats(filePath, &fileInfo);
    const char *msg = getFileStatsErrorMsg(err);

    if (!msg) {
      LogError("Unknown error (%d) when getting stats for file: %s", err, filePath.data);
      continue;
    }

    if (err != FILE_STATS_SUCCESS) {
      LogError(msg, filePath.data);
      continue;
    }

    LogSuccess(msg, filePath.data);
    LogInfo("Name: %s", fileInfo.name);
    LogInfo("Extension: %s", (fileInfo.extension && fileInfo.extension[0]) ? fileInfo.extension : "None");
    printSize(fileInfo.size);
    printTime("Created", fileInfo.createTime);
    printTime("Modified", fileInfo.modifyTime);
  }

  return 0;
}