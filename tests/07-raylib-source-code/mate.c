#define MATE_IMPLEMENTATION
#include "../../mate.h"

char *GetCFlags() {
  FlagBuilder flagsBuilder = FlagBuilderCreate();

  // NOTE: Default Raylib Flags
  FlagBuilderAddMany(&flagsBuilder,
                     "std=c99",
                     "D_GNU_SOURCE",
                     "DGL_SILENCE_DEPRECATION=199309L",
                     "fno-sanitize=undefined" // https://github.com/raysan5/raylib/issues/3674
  );

  if (isLinux()) {
    FlagBuilderAddMany(&flagsBuilder, "DPLATFORM_DESKTOP_GLFW", "D_GLFW_X11");
  }

  if (isWindows()) {
    FlagBuilderAdd(&flagsBuilder, "DPLATFORM_DESKTOP_GLFW");
  }

  LogInfo("Final flags: \"%s\"", flagsBuilder.buffer.data);
  return flagsBuilder.buffer.data;
}

i32 main() {
  StartBuild();
  {
    { // Compile static lib
      CreateStaticLib((StaticLibOptions){.output = "libraylib", .flags = GetCFlags()});

      AddFile("./src/rcore.c");
      AddFile("./src/utils.c");
      AddFile("./src/rglfw.c");

      AddFile("./src/rshapes.c");
      AddFile("./src/rtextures.c");
      AddFile("./src/rtext.c");
      AddFile("./src/rmodels.c");
      AddFile("./src/raudio.c");

      AddIncludePaths("./src/platforms");
      AddIncludePaths("./src/external/glfw/include");

      if (isLinux()) {
        LinkSystemLibraries("GL", "rt", "dl", "m", "X11", "Xcursor", "Xext", "Xfixes", "Xi", "Xinerama", "Xrandr", "Xrender");
      }
      if (isWindows()) {
        LinkSystemLibraries("winmm", "gdi32", "opengl32");
      }

      InstallStaticLib();
    }

    { // Run simple example
      CreateExecutable((ExecutableOptions){
          .output = "basic-example",
          .std = FLAG_STD_C99,
          .warnings = FLAG_WARNINGS_NONE,
      });

      AddFile("./src/basic-example.c");

      AddIncludePaths("./src");
      AddLibraryPaths("./build");

      if (isLinux()) {
        LinkSystemLibraries("raylib", "GL", "rt", "dl", "m", "X11");
      }
      if (isWindows()) {
        LinkSystemLibraries("raylib", "winmm", "gdi32", "opengl32");
      }

      String exePath = InstallExecutable();
      RunCommand(exePath);
    }
  }
  EndBuild();
}
