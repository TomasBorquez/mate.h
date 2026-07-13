#include "mathlib.h"

static long mathlib_call_count = 0;

Vec3 VectorAdd(Vec3 a, Vec3 b) {
  mathlib_call_count++;
  return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 VectorSub(Vec3 a, Vec3 b) {
  mathlib_call_count++;
  return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 VectorMul(Vec3 a, Vec3 b) {
  mathlib_call_count++;
  return (Vec3){a.x * b.x, a.y * b.y, a.z * b.z};
}
