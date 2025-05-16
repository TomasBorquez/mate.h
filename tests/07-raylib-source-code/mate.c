#define MATE_IMPLEMENTATION
#include "mate.h"

i32 main() {
  StartBuild();
  {
    {
      char *cflags = "-fno-sanitize=undefined -D_GNU_SOURCE -DGL_SILENCE_DEPRECATION=199309L -DPLATFORM_DESKTOP -DPLATFORM_DESKTOP_GLFW -D_GLFW_X11";
      CreateStaticLib((StaticLibOptions){.output = "libraylib", .std = FLAG_STD_C99, .warnings = FLAG_WARNINGS, .flags = cflags});

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

      LinkSystemLibraries("GL", "rt", "dl", "m", "X11", "Xcursor", "Xext", "Xfixes", "Xi", "Xinerama", "Xrandr", "Xrender");

      InstallStaticLib();
    }

    {
      CreateExecutable((ExecutableOptions){.output = "basic-example", .std = FLAG_STD_C99, .warnings = FLAG_WARNINGS});

      AddFile("./src/basic-example.c");

      AddIncludePaths("./src");
      AddLibraryPaths("./build");
      LinkSystemLibraries("raylib", "GL", "rt", "dl", "m", "X11");

      String exePath = InstallExecutable();
      RunCommand(exePath);
    }
  }
  EndBuild();
}
