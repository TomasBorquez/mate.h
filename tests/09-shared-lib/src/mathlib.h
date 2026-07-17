#pragma once

#if defined(_MSC_VER)
  #ifdef MATHLIB_BUILD
    #define MATHLIB_API __declspec(dllexport)
  #else
    #define MATHLIB_API __declspec(dllimport)
  #endif
#else
  #define MATHLIB_API
#endif

typedef struct {
  float x, y, z;
} Vec3;

MATHLIB_API Vec3 VectorAdd(Vec3 a, Vec3 b);
MATHLIB_API Vec3 VectorSub(Vec3 a, Vec3 b);
MATHLIB_API Vec3 VectorMul(Vec3 a, Vec3 b);
