// WARNING: Still not finished

#define MATE_IMPLEMENTATION
#include "mate.h"

i32 main() {
  StartBuild();
  {
    char *cflags = "-Wall -O2 -std=c99 -DLUA_USE_LINUX"
                   "-fno-stack-protector -fno-common -march=native"
                   // Warning flags from the makefile (subset to avoid potential compiler-specific issues)
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
    AddFile("lapi.c");
    AddFile("lcode.c");
    AddFile("lctype.c");
    AddFile("ldebug.c");
    AddFile("ldo.c");
    AddFile("ldump.c");
    AddFile("lfunc.c");
    AddFile("lgc.c");
    AddFile("llex.c");
    AddFile("lmem.c");
    AddFile("lobject.c");
    AddFile("lopcodes.c");
    AddFile("lparser.c");
    AddFile("lstate.c");
    AddFile("lstring.c");
    AddFile("ltable.c");
    AddFile("ltm.c");
    AddFile("lundump.c");
    AddFile("lvm.c");
    AddFile("lzio.c");
    AddFile("ltests.c");

    // Auxiliary files
    AddFile("lauxlib.c");

    // Library files
    AddFile("lbaselib.c");
    AddFile("ldblib.c");
    AddFile("liolib.c");
    AddFile("lmathlib.c");
    AddFile("loslib.c");
    AddFile("ltablib.c");
    AddFile("lstrlib.c");
    AddFile("lutf8lib.c");
    AddFile("loadlib.c");
    AddFile("lcorolib.c");
    AddFile("linit.c");

    // Build the static library
    String libPath = InstallStaticLibrary();

    // Now create the Lua executable
    CreateExecutable((ExecutableOptions){.output = "lua", .flags = cflagsStr.data});

    // Add lua.c for the executable
    AddFile("lua.c");

    // Link the static library and system libraries
    AddLibraryPaths(libPath.data);
    LinkSystemLibraries("m", "dl");

    String exePath = InstallExecutable();

    // Uncomment to run the Lua interpreter
    RunCommand(exePath);

    CreateCompileCommands();
  }
  EndBuild();
}
