#define MATE_IMPLEMENTATION
#include "../../mate.h"

static char *GetCFlags(void) {
  FlagBuilder flagsBuilder = FlagBuilderCreate();
  FlagBuilderAdd(&flagsBuilder, "fno-stack-protector", "fno-common", "march=native", "Wno-tautological-compare");

  if (isWindows()) {
    FlagBuilderAdd(&flagsBuilder, "DLUA_USE_WINDOWS");
  } else if (isMacOs()) {
    FlagBuilderAdd(&flagsBuilder, "DLUA_USE_MACOSX");
  } else if (isLinux()) {
    FlagBuilderAdd(&flagsBuilder, "DLUA_USE_LINUX");
  }

  return flagsBuilder.buffer.data;
}

int main(void) {
  StartBuild();
  {
    StaticLib staticLib = CreateStaticLib((StaticLibOptions){.output = "liblua", .std = FLAG_STD_C99, .warnings = FLAG_WARNINGS_NONE, .flags = GetCFlags()});

    AddFile(staticLib, "./src/*.c");
    RemoveFile(staticLib, "./src/lua.c");
    RemoveFile(staticLib, "./src/onelua.c");

    InstallStaticLib(staticLib);

    Executable executable = CreateExecutable((ExecutableOptions){.output = "lua", .std = FLAG_STD_C99, .warnings = FLAG_WARNINGS_NONE, .flags = GetCFlags()});
    AddFile(executable, "./src/lua.c");
    AddLibraryPaths(executable, "./build/"); // TODO: LinkStaticLib()

    LinkSystemLibraries(executable, "lua", "m");
    if (isLinux()) {
      LinkSystemLibraries(executable, "dl");
    }

    InstallExecutable(executable);

    errno_t errExe = RunCommandF("%s -e \"os.exit(69)\"", executable.outputPath.data);
    errno_t errCompileCmd = CreateCompileCommands(executable);
    Assert(errExe == 69 && errCompileCmd == SUCCESS, "Failed, errExe equal %d should be 69, errCompileCommands equal %d should be SUCCESS", errExe, errCompileCmd);
  }
  EndBuild();
}
