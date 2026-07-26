#define MATE_IMPLEMENTATION
#include "../../mate.h"

static Target GetHostCrossTarget(void) {
  Target host = HostTarget();

  if (host.compilerFamily == COMPILER_CLANG) {
    return (Target){
        .os = OS_WINDOWS,
        .arch = ARCH_X64,
        .compiler = host.compiler,
        .compilerFamily = COMPILER_CLANG,
    };
  }

  return (Target){
      .os = OS_WINDOWS,
      .arch = ARCH_X64,
      .compiler = "x86_64-w64-mingw32-gcc",
      .compilerFamily = COMPILER_GCC,
  };
}

static void AssertExists(String path) {
  Assert(FileStats(path).error == SUCCESS, "output-paths test: expected %s to exist", path.data);
}

static void AssertMissing(String path) {
  Assert(FileStats(path).error != SUCCESS, "output-paths test: expected %s to not exist", path.data);
}

int main(void) {
  Target t = HostTarget();
  Target cross = GetHostCrossTarget();

  StartBuild();
  {
    String hostDir = mate_target_dir(t);
    String crossDir = mate_target_dir(cross);

    StaticLib math = CreateStaticLib((StaticLibOptions){
        .output = "math",
        .warnings = FLAG_WARNINGS,
    });

    AddFile(math, "./src/math.c");

    InstallStaticLib(math);

    Executable app = CreateExecutable((ExecutableOptions){
        .output = "app",
        .warnings = FLAG_WARNINGS,
    });

    AddFile(app, "./src/app.c");

    LinkStaticLib(app, math);
    InstallExecutable(app);

    Executable cli = CreateExecutable((ExecutableOptions){
        .output = "cli",
        .warnings = FLAG_WARNINGS,
    });

    AddFile(cli, "./src/tools/cli.c");

    InstallExecutable(cli);

    AssertExists(F(mate_state.arena, "./build/bin/%s/%s", hostDir.data, app.output.data));
    AssertExists(F(mate_state.arena, "./build/bin/%s/%s", hostDir.data, cli.output.data));
    AssertExists(F(mate_state.arena, "./build/lib/%s/%s", hostDir.data, math.output.data));

    AssertMissing(F(mate_state.arena, "./build/%s", app.output.data));
    AssertMissing(F(mate_state.arena, "./build/%s", math.output.data));
    AssertMissing(F(mate_state.arena, "./build/bin/%s", app.output.data));
    AssertMissing(F(mate_state.arena, "./build/bin/%s", cli.output.data));
    AssertMissing(F(mate_state.arena, "./build/lib/%s", math.output.data));

    AssertExists(F(mate_state.arena, "./build/obj/%s/app/%s", hostDir.data, NormPathOutput(t, S("./src/app.c")).data));
    AssertExists(F(mate_state.arena, "./build/obj/%s/cli/%s", hostDir.data, NormPathOutput(t, S("./src/tools/cli.c")).data));
    AssertExists(F(mate_state.arena, "./build/obj/%s/%s/%s", hostDir.data, PathStem(math.output).data, NormPathOutput(t, S("./src/math.c")).data));

    AssertExists(app.ninjaBuildPath);
    AssertExists(S("./build/ninja/exe-app.ninja"));
    AssertMissing(S("./build/exe-app.ninja"));

    errno_t err_app = RunCommand(app.outputPath);
    Assert(err_app == SUCCESS, "output-paths test: failed running app at %s", app.outputPath.data);

    errno_t err_cli = RunCommand(cli.outputPath);
    Assert(err_cli == SUCCESS, "output-paths test: failed running cli at %s", cli.outputPath.data);

    StaticLib crossMath = CreateStaticLib((StaticLibOptions){
        .output = "math",
        .warnings = FLAG_WARNINGS,
        .target = cross,
    });

    AddFile(crossMath, "./src/math.c");

    InstallStaticLib(crossMath);

    Executable crossApp = CreateExecutable((ExecutableOptions){
        .output = "app",
        .warnings = FLAG_WARNINGS,
        .target = cross,
    });

    AddFile(crossApp, "./src/app.c");

    LinkStaticLib(crossApp, crossMath);
    InstallExecutable(crossApp);

    AssertExists(F(mate_state.arena, "./build/bin/%s/%s", crossDir.data, crossApp.output.data));
    AssertExists(F(mate_state.arena, "./build/lib/%s/%s", crossDir.data, crossMath.output.data));
    AssertExists(F(mate_state.arena, "./build/obj/%s/app/%s", crossDir.data, NormPathOutput(cross, S("./src/app.c")).data));

    AssertExists(F(mate_state.arena, "./build/bin/%s/%s", hostDir.data, app.output.data));
    AssertExists(F(mate_state.arena, "./build/obj/%s/app/%s", hostDir.data, NormPathOutput(t, S("./src/app.c")).data));
  }
  EndBuild();
}
