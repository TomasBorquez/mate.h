#pragma once

typedef struct {
  float x, y, z;
} Vec3;

Vec3 VectorAdd(Vec3 a, Vec3 b);
Vec3 VectorSub(Vec3 a, Vec3 b);
Vec3 VectorMul(Vec3 a, Vec3 b);
