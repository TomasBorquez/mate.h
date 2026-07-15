#define MATE_IMPLEMENTATION
#include "../../mate.h"

static char *GetCFlags(Target t) {
  FlagBuilder flagsBuilder = FlagBuilderCreate();
  FlagBuilderAdd(t, &flagsBuilder, "fno-stack-protector", "fno-common", "march=native", "Wno-tautological-compare");

  if (isWindows(t)) {
    FlagBuilderAdd(t, &flagsBuilder, "DLUA_USE_WINDOWS");
  }

  if (isMacOS(t)) {
    FlagBuilderAdd(t, &flagsBuilder, "DLUA_USE_MACOSX");
  }

  if (isLinux(t)) {
    FlagBuilderAdd(t, &flagsBuilder, "DLUA_USE_LINUX");
  }

  return flagsBuilder.buffer.data;
}

int main(void) {
  Target t = HostTarget();

  StartBuild();
  {
    StaticLib staticLib = CreateStaticLib((StaticLibOptions){
        .output = "liblua",
        .std = FLAG_STD_C99,
        .warnings = FLAG_WARNINGS_NONE,
        .flags = GetCFlags(t)
    });

    AddFile(staticLib, "./src/*.c");
    RemoveFile(staticLib, "./src/lua.c");
    RemoveFile(staticLib, "./src/onelua.c");

    InstallStaticLib(staticLib);

    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "lua",
        .std = FLAG_STD_C99,
        .warnings = FLAG_WARNINGS_NONE,
        .flags = GetCFlags(t)
    });
    AddFile(executable, "./src/lua.c");

    LinkStaticLib(executable, staticLib);
    LinkSystemLibraries(executable, "m");
    if (isLinux(t)) {
      LinkSystemLibraries(executable, "dl");
    }

    InstallExecutable(executable);

    errno_t errExe = RunCommandF("%s -e \"os.exit(69)\"", executable.outputPath.data);
    errno_t errCompileCmd = CreateCompileCommands(executable);
    Assert(errExe == 69 && errCompileCmd == SUCCESS, "Failed, errExe equal %d should be 69, errCompileCommands equal %d should be SUCCESS", errExe, errCompileCmd);
  }
  EndBuild();
}
