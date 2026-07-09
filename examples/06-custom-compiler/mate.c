#define MATE_IMPLEMENTATION
#include "../../mate.h"

// You cannot compile mate.c or samurai.c with C++ compilers but you can use them!
int main(void) {
  CreateConfig((MateOptions){
      .compiler = "g++",    // select whichever compiler or cross-compiler you want
      .compilerFamily = GCC // select the family, in this case g++ is GCC
  });

  StartBuild();
  {
    Executable executable = CreateExecutable((ExecutableOptions){.output = "main", .flags = "-Wall"});

    AddFile(executable, "./src/main.cpp");   // add the files with the corresponding extension
    AddFile(executable, "./src/t_math.cpp"); // add the files with the corresponding extension

    if (isLinux() || isFreeBSD()) {
      LinkSystemLibraries(executable, "m");
    }

    InstallExecutable(executable);

    errno_t errExe = RunCommand(executable.outputPath);
    errno_t errCompileCmd = CreateCompileCommands(executable);
    Assert(errExe == SUCCESS && errCompileCmd == SUCCESS, "Failed, RunCommand and errCompileCmd should be SUCCESS");
  }
  EndBuild();
}
