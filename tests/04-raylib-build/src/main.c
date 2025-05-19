#include "raylib.h"
#include <stdlib.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 450

int main(void) {
  exit(0);

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Example window");
  SetTargetFPS(144);

  while (!WindowShouldClose()) {
    BeginDrawing();
    {
      ClearBackground(RAYWHITE);
      DrawText("HELLOPE", 300, 200, 40, LIGHTGRAY);
    }
    EndDrawing();
  }

  CloseWindow();
}
