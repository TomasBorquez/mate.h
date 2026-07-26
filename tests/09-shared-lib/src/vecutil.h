#pragma once

#if defined(_MSC_VER)
  #ifdef VECUTIL_BUILD
    #define VECUTIL_API __declspec(dllexport)
  #else
    #define VECUTIL_API __declspec(dllimport)
  #endif
#else
  #define VECUTIL_API
#endif

VECUTIL_API float ScaleValue(float value, float factor);
