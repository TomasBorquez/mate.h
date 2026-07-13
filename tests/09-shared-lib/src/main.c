#include <math.h>
#include <stdio.h>

#include "mathlib.h"

#define EPSILON 1e-6f

static int failures = 0;

static void ExpectVec3(const char *name, Vec3 got, Vec3 want) {
  if (fabsf(got.x - want.x) > EPSILON ||
      fabsf(got.y - want.y) > EPSILON ||
      fabsf(got.z - want.z) > EPSILON) {
    fprintf(stderr, "FAIL %s: got (%f, %f, %f), want (%f, %f, %f)\n", name, got.x, got.y, got.z, want.x, want.y, want.z);
    failures++;
    return;
  }
  printf("PASS %s\n", name);
}

int main(void) {
  Vec3 a = {1.0f, 2.0f, 3.0f};
  Vec3 b = {4.0f, 5.0f, 6.0f};

  ExpectVec3("VectorAdd", VectorAdd(a, b), (Vec3){5.0f, 7.0f, 9.0f});
  ExpectVec3("VectorSub", VectorSub(a, b), (Vec3){-3.0f, -3.0f, -3.0f});
  ExpectVec3("VectorMul", VectorMul(a, b), (Vec3){4.0f, 10.0f, 18.0f});

  if (failures != 0) {
    fprintf(stderr, "%d test(s) failed\n", failures);
    return 1;
  }

  printf("all shared lib tests passed\n");
  return 0;
}
