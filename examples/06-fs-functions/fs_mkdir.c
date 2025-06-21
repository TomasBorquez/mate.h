#define MATE_IMPLEMENTATION
#include "../../mate.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    LogError("Usage: %s <directory> [directory...]", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; ++i) {
    String dirName = s(argv[i]);
    bool created = Mkdir(dirName);

    if (created) {
      LogSuccess("Directory created or already exists: %s", dirName.data);
    } else {
      LogError("Failed to create directory: %s", dirName.data);
    }
  }
  return 0;
}