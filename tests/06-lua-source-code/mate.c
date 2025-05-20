#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main(void) {
  StartBuild();
  {
    char *cflags = "-DLUA_USE_LINUX -fno-stack-protector -fno-common -march=native -Wno-tautological-compare";
    StaticLib staticLib = CreateStaticLib((StaticLibOptions){.output = "liblua", .std = FLAG_STD_C99, .warnings = FLAG_WARNINGS_NONE, .flags = cflags});

    AddFile(staticLib, "./src/*.c");
    RemoveFile(staticLib, "./src/lua.c");
    RemoveFile(staticLib, "./src/onelua.c");

    mateInstallStaticLib(&staticLib);

    Executable executable = CreateExecutable((ExecutableOptions){.output = "lua", .std = FLAG_STD_C99, .warnings = FLAG_WARNINGS_NONE, .flags = cflags});
    AddFile(executable, "./src/lua.c");
    AddLibraryPaths(executable, "./build/"); // TODO: LinkStaticLib()
    LinkSystemLibraries(executable, "lua", "m", "dl");

    InstallExecutable(executable);

    errno_t errExe = RunCommand(F(mateState.arena, "%s -e \"os.exit(69)\"", executable.outputPath.data));
    errno_t errCompileCommands = CreateCompileCommands(executable);
    Assert(errExe == 69 && errCompileCommands == SUCCESS, "Failed, errExe equal %d should be 69, errCompileCommands equal %d should be SUCCESS", errExe, errCompileCommands);
  }
  EndBuild();
}
