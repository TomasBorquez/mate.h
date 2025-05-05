#pragma once
#include <math.h>

float calculate_depth(float sin_i, float cos_j, float sin_A, float sin_j, float cos_A);

int calculate_x_coordinate(float depth, float sin_i, float cos_i, float cos_j, float sin_A, float sin_j, float cos_A, float cos_B, float sin_B);

int calculate_y_coordinate(float depth, float sin_i, float cos_i, float cos_j, float sin_A, float sin_j, float cos_A, float cos_B, float sin_B);

int calculate_luminance(float sin_i, float cos_j, float sin_A, float sin_j, float cos_A, float cos_i, float cos_B, float sin_B);

int calculate_array_index(int x, int y);

char select_character(int luminance);

int is_within_bounds(int x, int y, float depth, float current_depth);

float increment_A(float A);

float increment_B(float B);
