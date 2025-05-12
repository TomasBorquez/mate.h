// WARNING: Still not finished

#define MATE_IMPLEMENTATION
#include "../../mate.h"

i32 main() {
  errno_t result;

  StartBuild();
  {
    char *cflags = "-Wall -O2 -std=c99 -DLUA_USE_LINUX"
                   "-fno-stack-protector -fno-common -march=native"
                   // Warning flags from the makefile
                   "-Wfatal-errors -Wextra -Wshadow -Wundef"
                   "-Wwrite-strings -Wredundant-decls"
                   "-Wdisabled-optimization -Wdouble-promotion"
                   "-Wmissing-declarations -Wconversion"
                   "-Wstrict-overflow=2"
                   // C-specific warnings
                   "-Wdeclaration-after-statement"
                   "-Wmissing-prototypes"
                   "-Wnested-externs"
                   "-Wstrict-prototypes"
                   "-Wc++-compat"
                   "-Wold-style-definition";

    // First, create the static library (liblua.a)
    CreateStaticLibrary((StaticLibraryOptions){.output = "liblua.a", .flags = cflags});

    // Core files
    AddFile("./src/lapi.c");
    AddFile("./src/lcode.c");
    AddFile("./src/lctype.c");
    AddFile("./src/ldebug.c");
    AddFile("./src/ldo.c");
    AddFile("./src/ldump.c");
    AddFile("./src/lfunc.c");
    AddFile("./src/lgc.c");
    AddFile("./src/llex.c");
    AddFile("./src/lmem.c");
    AddFile("./src/lobject.c");
    AddFile("./src/lopcodes.c");
    AddFile("./src/lparser.c");
    AddFile("./src/lstate.c");
    AddFile("./src/lstring.c");
    AddFile("./src/ltable.c");
    AddFile("./src/ltm.c");
    AddFile("./src/lundump.c");
    AddFile("./src/lvm.c");
    AddFile("./src/lzio.c");
    AddFile("./src/ltests.c");

    // Auxiliary files
    AddFile("./src/lauxlib.c");

    // Library files
    AddFile("./src/lbaselib.c");
    AddFile("./src/ldblib.c");
    AddFile("./src/liolib.c");
    AddFile("./src/lmathlib.c");
    AddFile("./src/loslib.c");
    AddFile("./src/ltablib.c");
    AddFile("./src/lstrlib.c");
    AddFile("./src/lutf8lib.c");
    AddFile("./src/loadlib.c");
    AddFile("./src/lcorolib.c");
    AddFile("./src/linit.c");

    // Build the static library
    String libPath = InstallStaticLibrary();

    CreateExecutable((ExecutableOptions){.output = "lua", .flags = cflags});

    AddFile("./src/lua.c");

    // Link the static library and system libraries
    AddLibraryPaths(libPath.data);
    LinkSystemLibraries("m", "dl");

    String exePath = InstallExecutable();

    // Cun the Lua interpreter
    errno_t errExe = RunCommand(exePath);
    errno_t errCompileCommands = CreateCompileCommands();
    result = (errExe == SUCCESS && errCompileCommands == SUCCESS) ? SUCCESS : 1;
    LogInfo("result: %d", result);
  }
  EndBuild();
}
