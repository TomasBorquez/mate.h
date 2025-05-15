#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  errno_t result;
  StartBuild();
  {
    char *cflags = "-DLUA_USE_LINUX -fno-stack-protector -fno-common -march=native";
    CreateStaticLib((StaticLibOptions){.output = "liblua", .std = FLAG_STD_C99, .warnings = FLAG_WARNINGS, .flags = cflags});

    AddFile("./src/*.c");
    RemoveFile("./src/lua.c");
    RemoveFile("./src/onelua.c");

    InstallStaticLib();

    String ninjaPath = CreateExecutable((ExecutableOptions){.output = "lua", .std = FLAG_STD_C99, .warnings = FLAG_WARNINGS, .flags = cflags});
    AddFile("./src/lua.c");
    AddLibraryPaths("./build/");
    LinkSystemLibraries("lua", "m", "dl");

    String exePath = InstallExecutable();

    errno_t errExe = RunCommand(F(state.arena, "%s -e \"os.exit(69)\"", exePath.data));
    errno_t errCompileCommands = CreateCompileCommands(ninjaPath);
    result = (errExe == 69 && errCompileCommands == SUCCESS) ? SUCCESS : 1;
  }
  EndBuild();

  return result;
}
