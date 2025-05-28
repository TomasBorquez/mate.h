#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "t_math.h"

int main() {
  float A = 0, B = 0;
  float i, j;
  int k;
  float z[1760]; // Depth buffer
  char b[1760];  // Character buffer

  //printf("\x1b[2J"); // Clear screen

  while (1) {
    // Clear buffers
    memset(b, 32, 1760); // Fill with spaces
    memset(z, 0, 7040);  // Clear depth buffer

    // Iterate through all points on the donut surface
    for (j = 0; j < 6.28; j += 0.07) {
      for (i = 0; i < 6.28; i += 0.02) {
        // Calculate trigonometric values
        float sin_i = sin(i);
        float cos_i = cos(i);
        float sin_j = sin(j);
        float cos_j = cos(j);
        float sin_A = sin(A);
        float cos_A = cos(A);
        float sin_B = sin(B);
        float cos_B = cos(B);

        // Calculate depth/distance factor
        float depth = calculate_depth(sin_i, cos_j, sin_A, sin_j, cos_A);

        // Calculate screen coordinates
        int x = calculate_x_coordinate(depth, sin_i, cos_i, cos_j, sin_A, sin_j, cos_A, cos_B, sin_B);
        int y = calculate_y_coordinate(depth, sin_i, cos_i, cos_j, sin_A, sin_j, cos_A, cos_B, sin_B);

        // Calculate array index for screen buffer
        int o = calculate_array_index(x, y);

        // Calculate luminance/shading
        int luminance = calculate_luminance(sin_i, cos_j, sin_A, sin_j, cos_A, cos_i, cos_B, sin_B);

        // Check if point should be drawn
        if (is_within_bounds(x, y, depth, z[o])) {
          z[o] = depth;                       // Update depth buffer
          b[o] = select_character(luminance); // Select character based on luminance
        }
      }
    }

    // Move cursor to home position
    // printf("\x1b[H");

    // Display the frame and update rotation angles
    for (k = 0; k < 1761; k++) {
      putchar(k % 80 ? b[k] : 10); // Print character or newline
      A = increment_A(A);          // Increment rotation angle A
      B = increment_B(B);          // Increment rotation angle B
    }

    exit(0); // Remove if you want to see the example working
  }

  return 0;
}
