#include "t_math.h"

// Distance/Depth calculation function
float calculate_depth(float sin_i, float cos_j, float sin_A, float sin_j, float cos_A) {
  // Calculates the distance factor D
  float c = sin_i; // sin(i)
  float d = cos_j; // cos(j)
  float e = sin_A; // sin(A)
  float f = sin_j; // sin(j)
  float g = cos_A; // cos(A)
  float h = d + 2; // cos(j) + 2

  // Calculate D = 1 / (c * h * e + f * g + 5)
  float D = 1 / (c * h * e + f * g + 5);
  return D;
}

// Screen coordinate calculation functions
int calculate_x_coordinate(float depth, float sin_i, float cos_i, float cos_j, float sin_A, float sin_j, float cos_A, float cos_B, float sin_B) {
  float c = sin_i;     // sin(i)
  float h = cos_j + 2; // cos(j) + 2
  float g = cos_A;     // cos(A)
  float e = sin_A;     // sin(A)
  float f = sin_j;     // sin(j)
  float l = cos_i;     // cos(i)
  float m = cos_B;     // cos(B)
  float n = sin_B;     // sin(B)

  float t = c * h * g - f * e;
  int x = 40 + 30 * depth * (l * h * m - t * n);
  return x;
}

int calculate_y_coordinate(float depth, float sin_i, float cos_i, float cos_j, float sin_A, float sin_j, float cos_A, float cos_B, float sin_B) {
  float c = sin_i;     // sin(i)
  float h = cos_j + 2; // cos(j) + 2
  float g = cos_A;     // cos(A)
  float e = sin_A;     // sin(A)
  float f = sin_j;     // sin(j)
  float l = cos_i;     // cos(i)
  float m = cos_B;     // cos(B)
  float n = sin_B;     // sin(B)

  float t = c * h * g - f * e;
  int y = 12 + 15 * depth * (l * h * n + t * m);
  return y;
}

// Lighting/shading calculation function
int calculate_luminance(float sin_i, float cos_j, float sin_A, float sin_j, float cos_A, float cos_i, float cos_B, float sin_B) {
  float c = sin_i; // sin(i)
  float d = cos_j; // cos(j)
  float e = sin_A; // sin(A)
  float f = sin_j; // sin(j)
  float g = cos_A; // cos(A)
  float l = cos_i; // cos(i)
  float n = sin_B; // sin(B)
  float m = cos_B; // cos(B)

  int N = 8 * ((f * e - c * d * g) * m - c * d * e - f * g - l * d * n);
  return N;
}

// Array indexing function
int calculate_array_index(int x, int y) {
  return x + 80 * y;
}

// Character selection function
char select_character(int luminance) {
  const char *chars = ".,-~:;=!*#$@";
  int N = luminance > 0 ? luminance : 0;
  if (N >= 12) N = 11; // Ensure we don't exceed array bounds
  return chars[N];
}

// Bounds checking function
int is_within_bounds(int x, int y, float depth, float current_depth) {
  return (22 > y && y > 0 && x > 0 && 80 > x && depth > current_depth);
}

// Angle increment functions
float increment_A(float A) {
  return A + 0.00004;
}

float increment_B(float B) {
  return B + 0.00002;
}
