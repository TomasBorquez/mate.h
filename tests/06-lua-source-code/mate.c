#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  StartBuild();
  {
    char *cflags = "-DLUA_USE_LINUX -fno-stack-protector -fno-common -march=native";
    CreateStaticLib((StaticLibOptions){.output = "liblua", .std = FLAG_STD_C99, .warnings = FLAG_WARNINGS_NONE, .flags = cflags});

    AddFile("./src/*.c");
    RemoveFile("./src/lua.c");
    RemoveFile("./src/onelua.c");

    InstallStaticLib();

    String ninjaPath = CreateExecutable((ExecutableOptions){.output = "lua", .std = FLAG_STD_C99, .warnings = FLAG_WARNINGS_NONE, .flags = cflags});
    AddFile("./src/lua.c");
    AddLibraryPaths("./build/");
    LinkSystemLibraries("lua", "m", "dl");

    String exePath = InstallExecutable();

    errno_t errExe = RunCommand(F(mateState.arena, "%s -e \"os.exit(69)\"", exePath.data));
    errno_t errCompileCommands = CreateCompileCommands(ninjaPath);
    Assert(errExe == 69 && errCompileCommands == SUCCESS, "Failed, errExe equal %d should be 69, errCompileCommands equal %d should be SUCCESS", errExe, errCompileCommands);
  }
  EndBuild();
}
