#define MATE_IMPLEMENTATION
#include "../../mate.h"

static char *GetCFlags(Target t) {
  FlagBuilder fb = FlagBuilderCreate();

  // NOTE: Default Raylib Flags
  FlagBuilderAdd(t, &fb,
                 "std=c99",
                 "D_GNU_SOURCE",
                 "DGL_SILENCE_DEPRECATION=199309L",
                 "fno-sanitize=undefined" // https://github.com/raysan5/raylib/issues/3674
  );

  if (isLinux(t)) {
    FlagBuilderAdd(t, &fb, "DPLATFORM_DESKTOP_GLFW", "D_GLFW_X11");
  }

  if (isFreeBSD(t)) {
    FlagBuilderAdd(t, &fb, "DPLATFORM_DESKTOP_GLFW", "D_GLFW_X11");
    // Required due to raylib's use of X11 on FreeBSD
    FlagBuilderAdd(t, &fb, "isystem /usr/local/include");
  }

  if (isMacOS(t)) {
    FlagBuilderAdd(t, &fb, "DPLATFORM_DESKTOP_GLFW", "D_GL_SILENCE_DEPRECATION=1");
    // Required due to raylib's use of Objective-C types on macOS.
    FlagBuilderAdd(t, &fb, "x objective-c");
  }

  if (isWindows(t)) {
    FlagBuilderAdd(t, &fb, "DPLATFORM_DESKTOP_GLFW");
  }

  return fb.buffer.data;
}

int main(void) {
  Target t = HostTarget();

  StartBuild();
  {
    // static lib:
    StaticLib staticLib = CreateStaticLib((StaticLibOptions){
        .output = "raylib",
        .flags = GetCFlags(t),
    });

    AddFile(staticLib, "./src/rcore.c", "./src/utils.c", "./src/rglfw.c");
    AddFile(staticLib, "./src/rshapes.c");
    AddFile(staticLib, "./src/rtextures.c");
    AddFile(staticLib, "./src/rtext.c");
    AddFile(staticLib, "./src/rmodels.c");
    AddFile(staticLib, "./src/raudio.c");

    AddIncludePaths(staticLib, "./src/platforms");
    AddIncludePaths(staticLib, "./src/external/glfw/include");

    if (isLinux(t)) {
      LinkSystemLibraries(staticLib, "GL", "rt", "dl", "m", "X11", "Xcursor", "Xext", "Xfixes", "Xi", "Xinerama", "Xrandr", "Xrender");
    }
    if (isFreeBSD(t)) {
      // Link regular system libraries.
      AddLibraryPaths(staticLib, "/usr/local/lib");
      LinkSystemLibraries(staticLib, "GL", "rt", "dl", "m", "X11", "Xcursor", "Xext", "Xfixes", "Xi", "Xinerama", "Xrandr", "Xrender");
    }
    if (isMacOS(t)) {
      // Link regular system libraries.
      LinkSystemLibraries(staticLib, "m");
      // Link macOS system frameworks.
      LinkFrameworks(staticLib, "Foundation", "AppKit", "IOKit", "OpenGL", "CoreVideo");
    }
    if (isWindows(t)) {
      LinkSystemLibraries(staticLib, "winmm", "gdi32", "opengl32");
    }

    InstallStaticLib(staticLib);

    // executable:
    Executable executable = CreateExecutable((ExecutableOptions){
        .output = "basic-example",
        .std = FLAG_STD_C99,
        .warnings = FLAG_WARNINGS_NONE,
    });

    AddFile(executable, "./src/basic-example.c");

    AddIncludePaths(executable, "./src");

    LinkStaticLib(executable, staticLib);
    if (isLinux(t)) {
      LinkSystemLibraries(executable, "GL", "rt", "dl", "m", "X11");
    }
    if (isFreeBSD(t)) {
      // Link regular system libraries.
      AddLibraryPaths(executable, "/usr/local/lib");
      LinkSystemLibraries(executable, "GL", "rt", "dl", "m", "X11");
    }
    if (isMacOS(t)) {
      // Link regular system libraries.
      LinkSystemLibraries(executable, "m");
      // Link macOS system frameworks.
      LinkFrameworks(executable, "CoreVideo", "IOKit", "Cocoa", "GLUT", "OpenGL");
    }
    if (isWindows(t)) {
      LinkSystemLibraries(executable, "winmm", "gdi32", "opengl32");
    }

    InstallExecutable(executable);
    errno_t errExe = RunCommand(executable.outputPath);
    if (errExe != SUCCESS) {
      LogError("Running the executable failed with err: %d", errExe);
    }
  }
  EndBuild();
}
