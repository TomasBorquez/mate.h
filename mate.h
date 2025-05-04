/* MIT License

  mate.h - A single-header library for compiling your C code in C
  Version - 2025-05-03 (0.1.5):
  https://github.com/TomasBorquez/mate.h

  Guide on the `README.md`
*/
#pragma once

#ifdef MATE_IMPLEMENTATION
#  define BASE_IMPLEMENTATION
#endif

// --- BASE.H START ---
/* MIT License

  base.h - Better cross-platform STD
  Version - 2025-05-03 (0.1.10):
  https://github.com/TomasBorquez/base.h

  Usage:
    #define BASE_IMPLEMENTATION
    #include "base.h"

  More on the the `README.md`
*/

/* --- Platform MACROS and includes --- */
#if defined(__clang__)
#  define COMPILER_CLANG
#elif defined(_MSC_VER)
#  define COMPILER_MSVC
#elif defined(__GNUC__)
#  define COMPILER_GCC
#else
#  error "The codebase only supports Clang, MSVC and GCC. TCC soon"
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#  define PLATFORM_WIN
#elif defined(__linux__) || defined(__gnu_linux__)
#  define PLATFORM_LINUX
#else
#  error "The codebase only supports windows and linux, macos soon"
#endif

#if defined(COMPILER_CLANG)
#  define FILE_NAME __FILE_NAME__
#else
#  define FILE_NAME __FILE__
#endif

#if defined(PLATFORM_WIN)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#elif defined(PLATFORM_LINUX)
#  include <dirent.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#if defined(__STDC_VERSION__)
#  if (__STDC_VERSION__ >= 202311L)
#    define C_STANDARD_C23
#    define C_STANDARD "C23"
#  elif (__STDC_VERSION__ >= 201710L)
#    define C_STANDARD_C17
#    define C_STANDARD "C17"
#  elif (__STDC_VERSION__ >= 201112L)
#    define C_STANDARD_C11
#    define C_STANDARD "C11"
#  elif (__STDC_VERSION__ >= 199901L)
#    define C_STANDARD_C99
#    define C_STANDARD "C99"
#  else
#    error "you are cooked" // ???
#  endif
#endif

#if defined(COMPILER_MSVC)
#  if _MSVC_LANG >= 202000L
#    define C_STANDARD_C23
#    define C_STANDARD "C23"
#  elif _MSVC_LANG >= 201704L
#    define C_STANDARD_C17
#    define C_STANDARD "C17"
#  elif _MSVC_LANG >= 201103L
#    define C_STANDARD_C11
#    define C_STANDARD "C11"
#  else
#    error "you are cooked" // ???
#  endif
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* --- Platform Specific --- */
#if defined(PLATFORM_WIN)
/* Process functions */
#  define popen _popen
#  define pclose _pclose

/* File I/O functions */
#  define fileno _fileno
#  define fdopen _fdopen
#  define access _access
#  define unlink _unlink
#  define isatty _isatty
#  define dup _dup
#  define dup2 _dup2
#  define ftruncate _chsize
#  define fsync _commit

/* Directory functions */
#  define mkdir(path, mode) _mkdir(path)
#  define rmdir _rmdir
#  define getcwd _getcwd
#  define chdir _chdir

/* Process/Threading */
#  define getpid _getpid
#  define execvp _execvp
#  define execve _execve
#  define sleep(x) Sleep((x) * 1000)
#  define usleep(x) Sleep((x) / 1000)

/* String functions */
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#  define strdup _strdup

/* File modes */
#  define R_OK 4
#  define W_OK 2
#  define X_OK 0 /* Windows doesn't have explicit X_OK */
#  define F_OK 0

/* File descriptors */
#  define STDIN_FILENO 0
#  define STDOUT_FILENO 1
#  define STDERR_FILENO 2

/* Some functions need complete replacements */
#  if defined(COMPILER_MSVC)
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf
#  endif

#endif

/* --- Types and MACRO types --- */
// Unsigned int types
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Signed int types
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// Regular int types
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean defines
#define bool _Bool
#define false 0
#define true 1

typedef struct {
  size_t length; // Does not include null terminator
  char *data;
} String;

// Maximum values for integer types
#define U8_MAX UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define S8_MAX INT8_MAX
#define S8_MIN INT8_MIN
#define S16_MAX INT16_MAX
#define S16_MIN INT16_MIN
#define S32_MAX INT32_MAX
#define S32_MIN INT32_MIN
#define S64_MAX INT64_MAX
#define S64_MIN INT64_MIN

#define I8_MAX INT8_MAX
#define I8_MIN INT8_MIN
#define I16_MAX INT16_MAX
#define I16_MIN INT16_MIN
#define I32_MAX INT32_MAX
#define I32_MIN INT32_MIN
#define I64_MAX INT64_MAX
#define I64_MIN INT64_MIN

#define TYPE_INIT(type) (type)

/* --- Vector Macros --- */
// TODO: Add MSVC like vector macros
// TODO: `VecInit` for adding an initial capacity at the beginning
// TODO: `VecSort` implement some sorting algorithm for sorting the vector
#define VEC_TYPE(typeName, valueType) \
  typedef struct {                    \
    valueType *data;                  \
    i32 length;                       \
    i32 capacity;                     \
  } typeName

// WARNING: Vector must always be initialized to zero `Vector vector = {0}`
#define VecPush(vector, value)                                                                                   \
  ({                                                                                                             \
    assert(vector.length <= vector.capacity && "VecPush: Possible memory corruption or vector not initialized"); \
    if (vector.length >= vector.capacity) {                                                                      \
      if (vector.capacity == 0) vector.capacity = 128;                                                           \
      else vector.capacity *= 2;                                                                                 \
      vector.data = Realloc(vector.data, vector.capacity * sizeof(*vector.data));                                \
    }                                                                                                            \
    vector.data[vector.length++] = value;                                                                        \
    &vector.data[vector.length - 1];                                                                             \
  })

#define VecPop(vector)                                                   \
  ({                                                                     \
    assert(vector.length > 0 && "VecPop: Cannot pop from empty vector"); \
    typeof(vector.data[0]) value = vector.data[vector.length - 1];       \
    vector.length--;                                                     \
    &value;                                                              \
  })

#define VecShift(vector)                                                                   \
  ({                                                                                       \
    assert(vector.length != 0 && "VecShift: Length should at least be >= 1");              \
    typeof(vector.data[0]) value = vector.data[0];                                         \
    memmove(&vector.data[0], &vector.data[1], (vector.length - 1) * sizeof(*vector.data)); \
    vector.length--;                                                                       \
    &value;                                                                                \
  })

#define VecUnshift(vector, value)                                                      \
  ({                                                                                   \
    if (vector.length >= vector.capacity) {                                            \
      if (vector.capacity == 0) vector.capacity = 2;                                   \
      else vector.capacity *= 2;                                                       \
      vector.data = Realloc(vector.data, vector.capacity * sizeof(*vector.data));      \
    }                                                                                  \
                                                                                       \
    if (vector.length > 0) {                                                           \
      memmove(&vector.data[1], &vector.data[0], vector.length * sizeof(*vector.data)); \
    }                                                                                  \
                                                                                       \
    vector.data[0] = value;                                                            \
    vector.length++;                                                                   \
    &value;                                                                            \
  })

#define VecInsert(vector, value, index)                                                                    \
  ({                                                                                                       \
    assert(index <= vector.length && "VecInsert: Index out of bounds for insertion");                      \
    if (vector.length >= vector.capacity) {                                                                \
      if (vector.capacity == 0) vector.capacity = 2;                                                       \
      else vector.capacity *= 2;                                                                           \
      vector.data = Realloc(vector.data, vector.capacity * sizeof(*vector.data));                          \
    }                                                                                                      \
    memmove(&vector.data[index + 1], &vector.data[index], (vector.length - index) * sizeof(*vector.data)); \
    vector.data[index] = value;                                                                            \
    vector.length++;                                                                                       \
    &value;                                                                                                \
  })

#define VecAt(vector, index)                                                     \
  ({                                                                             \
    assert(index >= 0 && index < vector.length && "VecAt: Index out of bounds"); \
    &vector.data[index];                                                         \
  })

#define VecFree(vector)                                                         \
  ({                                                                            \
    assert(vector.data != NULL && "VecFree: Vector data should never be NULL"); \
    Free(vector.data);                                                          \
    vector.data = NULL;                                                         \
  })

/* --- Time and Platforms --- */
i64 TimeNow();
void WaitTime(i64 ms);
String GetCompiler();
String GetPlatform();

/* --- Errors --- */
typedef i32 errno_t;

enum GeneralError {
  SUCCESS,
  MEMORY_ALLOCATION_FAILED,
};

/* --- Arena --- */
typedef struct {
  int8_t *buffer;
  size_t bufferLength;
  size_t prevOffset;
  size_t currOffset;
} Arena;

// This makes sure right alignment on 86/64 bits
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))

Arena ArenaInit(size_t size);
void *ArenaAlloc(Arena *arena, size_t size);
void ArenaFree(Arena *arena);
void ArenaReset(Arena *arena);

/* --- Memory Allocations --- */
void *Realloc(void *block, size_t size);
void *Malloc(size_t size);
void Free(void *address);

/* --- String and Macros --- */
#define STRING_LENGTH(s) ((sizeof(s) / sizeof((s)[0])) - sizeof((s)[0])) // NOTE: Inspired from clay.h
#define ENSURE_STRING_LITERAL(x) ("" x "")

// NOTE: If an error led you here, it's because `S` can only be used with string literals, i.e. `S("SomeString")` and not `S(yourString)` - for that use `s()`
#define S(string) (TYPE_INIT(String){.length = STRING_LENGTH(ENSURE_STRING_LITERAL(string)), .data = (string)})
String s(char *msg);

#if defined(COMPILER_CLANG)
#  define FORMAT_CHECK(fmt_pos, args_pos) __attribute__((format(printf, fmt_pos, args_pos))) // NOTE: Printf like warnings on format
#else
#  define FORMAT_CHECK(fmt_pos, args_pos)
#endif

String F(Arena *arena, const char *format, ...) FORMAT_CHECK(2, 3);

VEC_TYPE(StringVector, String);
#define StringVectorPushMany(vector, ...)              \
  ({                                                   \
    char *values[] = {__VA_ARGS__};                    \
    size_t count = sizeof(values) / sizeof(values[0]); \
    for (size_t i = 0; i < count; i++) {               \
      VecPush(vector, s(values[i]));                   \
    }                                                  \
  })

void SetMaxStrSize(size_t size);
String StrNew(Arena *arena, char *str);
String StrNewSize(Arena *arena, char *str, size_t len); // Without null terminator
void StrCopy(String *destination, String *source);
StringVector StrSplit(Arena *arena, String *string, String *delimiter);
bool StrEqual(String *string1, String *string2);
String StrConcat(Arena *arena, String *string1, String *string2);
void StrToUpper(String *string1);
void StrToLower(String *string1);
bool StrIsNull(String *string);
void StrTrim(String *string);
String StrSlice(Arena *arena, String *str, size_t start, size_t end);
String ConvertExe(Arena *arena, String path);
String ConvertPath(Arena *arena, String path);

/* --- Random --- */
void RandomInit(); // NOTE: Must init before using
u64 RandomGetSeed();
void RandomSetSeed(u64 newSeed);
i32 RandomInteger(i32 min, i32 max);
f32 RandomFloat(f32 min, f32 max);

/* --- File System --- */
#define MAX_FILES 200

typedef struct {
  char *name;
  char *extension;
  int64_t size;
  int64_t createTime;
  int64_t modifyTime;
} File;

typedef struct {
  char *name;
} Folder;

typedef struct {
  Folder *folders;
  size_t folderCount;

  File *files;
  size_t fileCount;

  size_t totalCount;
} FileData;

char *GetCwd();
void SetCwd(char *destination);
FileData *GetDirFiles();
FileData *NewFileData();

enum FileStatsError { FILE_GET_ATTRIBUTES_FAILED = 1 };
errno_t FileStats(String *path, File *file);

enum FileReadError { FILE_NOT_EXIST = 1, FILE_OPEN_FAILED, FILE_GET_SIZE_FAILED, FILE_READ_FAILED };
errno_t FileRead(Arena *arena, String *path, String *result);

enum FileWriteError { FILE_WRITE_OPEN_FAILED = 1, FILE_WRITE_ACCESS_DENIED, FILE_WRITE_NO_MEMORY, FILE_WRITE_NOT_FOUND, FILE_WRITE_DISK_FULL, FILE_WRITE_IO_ERROR };
errno_t FileWrite(String *path, String *data);

enum FileAddError { FILE_ADD_OPEN_FAILED = 1, FILE_ADD_ACCESS_DENIED, FILE_ADD_NO_MEMORY, FILE_ADD_NOT_FOUND, FILE_ADD_DISK_FULL, FILE_ADD_IO_ERROR };
errno_t FileAdd(String *path, String *data); // NOTE: Adds `\n` at the end always

enum FileDeleteError { FILE_DELETE_ACCESS_DENIED = 1, FILE_DELETE_NOT_FOUND, FILE_DELETE_IO_ERROR };
errno_t FileDelete(String *path);

enum FileRenameError { FILE_RENAME_ACCESS_DENIED = 1, FILE_RENAME_NOT_FOUND, FILE_RENAME_EXISTS, FILE_RENAME_IO_ERROR };
errno_t FileRename(String *oldPath, String *newPath);

bool Mkdir(String path); // NOTE: Mkdir if not exist

/* --- Logger --- */
#define _RESET "\x1b[0m"
#define _GRAY "\x1b[38;2;192;192;192m"
#define _RED "\x1b[0;31m"
#define _GREEN "\x1b[0;32m"
#define _ORANGE "\x1b[0;33m"

void LogInfo(const char *format, ...) FORMAT_CHECK(1, 2);
void LogWarn(const char *format, ...) FORMAT_CHECK(1, 2);
void LogError(const char *format, ...) FORMAT_CHECK(1, 2);
void LogSuccess(const char *format, ...) FORMAT_CHECK(1, 2);
void LogInit();

/* --- Math --- */
#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Clamp(a, x, b) (((x) < (a)) ? (a) : ((b) < (x)) ? (b) : (x))
#define Swap(a, b)      \
  ({                    \
    typeof(a) temp = a; \
    a = b;              \
    b = temp;           \
  })

/* --- Defer Macros --- */
#if defined(DEFER_MACRO) // NOTE: Optional since not all compilers support it and not all C versions do either
/* - GCC implementation -
  NOTE: Must use C23 (depending on the platform)
*/
#  if defined(COMPILER_GCC)
#    define defer __DEFER(__COUNTER__)
#    define __DEFER(N) __DEFER_(N)
#    define __DEFER_(N) __DEFER__(__DEFER_FUNCTION_##N, __DEFER_VARIABLE_##N)
#    define __DEFER__(F, V)      \
      auto void F(int *);        \
      [[gnu::cleanup(F)]] int V; \
      auto void F(int *)

/* - Clang implementation -
  NOTE: Must compile with flag `-fblocks`
*/
#  elif defined(COMPILER_CLANG)
typedef void (^const __df_t)(void);

[[maybe_unused]]
static inline void __df_cb(__df_t *__fp) {
  (*__fp)();
}

#    define defer __DEFER(__COUNTER__)
#    define __DEFER(N) __DEFER_(N)
#    define __DEFER_(N) __DEFER__(__DEFER_VARIABLE_##N)
#    define __DEFER__(V) [[gnu::cleanup(__df_cb)]] __df_t V = ^void(void)

/* -- MSVC implementation --
  NOTE: Not available yet in MSVC, use `_try/_finally`
*/
#  elif defined(COMPILER_MSVC)
#    error "Not available yet in MSVC, use `_try/_finally`"
#  endif
#endif

/* MIT License
   base.h - Implementation of base.h
   https://github.com/TomasBorquez/base.h
*/
#if defined(BASE_IMPLEMENTATION)
// --- Platform specific functions ---
#  if !defined(PLATFORM_WIN) && !defined(C_STANDARD_C11)

#    if !defined(EINVAL)
#      define EINVAL 22 // Invalid argument
#    endif

#    if !defined(ERANGE)
#      define ERANGE 34 // Result too large
#    endif

errno_t memcpy_s(void *dest, size_t destSize, const void *src, size_t count) {
  if (dest == NULL) {
    return EINVAL;
  }

  if (src == NULL || destSize < count) {
    memset(dest, 0, destSize);
    return EINVAL;
  }

  memcpy(dest, src, count);
  return 0;
}

inline errno_t fopen_s(FILE **streamptr, const char *filename, const char *mode) {
  if (streamptr == NULL) {
    return EINVAL;
  }

  *streamptr = NULL;
  if (filename == NULL || mode == NULL) {
    return EINVAL;
  }

  if (*filename == '\0') {
    return EINVAL;
  }

  const char *valid_modes = "rwa+btx";
  size_t mode_len = strlen(mode);

  if (mode_len == 0) {
    return EINVAL;
  }

  for (size_t i = 0; i < mode_len; i++) {
    if (strchr(valid_modes, mode[i]) == NULL) {
      return EINVAL;
    }
  }

  *streamptr = fopen(filename, mode);
  if (*streamptr == NULL) {
    return errno;
  }

  return 0;
}
#  endif

String GetCompiler() {
#  if defined(COMPILER_CLANG)
  return S("clang");
#  elif defined(COMPILER_GCC)
  return S("gcc");
#  elif defined(COMPILER_MSVC)
  return S("MSVC");
#  endif
}

String GetPlatform() {
#  if defined(PLATFORM_WIN)
  return S("windows");
#  elif defined(PLATFORM_LINUX)
  return S("linux");
#  endif
}

i64 TimeNow() {
#  if defined(PLATFORM_WIN)
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  LARGE_INTEGER li;
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  // Convert Windows FILETIME (100-nanosecond intervals since January 1, 1601)
  // to UNIX timestamp in milliseconds
  i64 currentTime = (li.QuadPart - 116444736000000000LL) / 10000;
#  else
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  i64 currentTime = (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
#  endif
  assert(currentTime != -1 && "TimeNow: currentTime should never be -1");
  return currentTime;
}

void WaitTime(i64 ms) {
#  if defined(PLATFORM_WIN)
  Sleep(ms);
#  else
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&ts, NULL);
#  endif
}

/* Arena Implemenation */
// https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
static intptr_t alignForward(const intptr_t ptr) {
  intptr_t p, a, modulo;

  p = ptr;
  a = (intptr_t)DEFAULT_ALIGNMENT;
  // Same as (p % a) but faster as 'a' is a power of two
  modulo = p & (a - 1);

  if (modulo != 0) {
    // If 'p' address is not aligned, push the address to the
    // next value which is aligned
    p += a - modulo;
  }
  return p;
}

void *ArenaAlloc(Arena *arena, const size_t size) {
  // Align 'currPtr' forward to the specified alignment
  intptr_t currPtr = (intptr_t)arena->buffer + (intptr_t)arena->currOffset;
  intptr_t offset = alignForward(currPtr);
  offset -= (intptr_t)arena->buffer; // Change to relative offset

  if (offset + size > arena->bufferLength) {
    LogError("Arena ran out of space left, bufferLength: %zu", arena->bufferLength);
    return NULL;
  }

  void *ptr = &arena->buffer[offset];
  arena->prevOffset = offset;
  arena->currOffset = offset + size;

  memset(ptr, 0, size);
  return ptr;
}

void ArenaFree(Arena *arena) {
  free(arena->buffer);
}

void ArenaReset(Arena *arena) {
  arena->currOffset = 0;
  arena->prevOffset = 0;
}

Arena ArenaInit(size_t size) {
  return (Arena){
      .buffer = (i8 *)Malloc(size),
      .bufferLength = size,
      .prevOffset = 0,
      .currOffset = 0,
  };
}

/* Memory Allocations */
// TODO: Add hashmap that checks for unfreed values only on DEBUG, __FILE__ and __LINE__
void *Malloc(size_t size) {
  assert(size > 0 && "Malloc: size cant be negative");

  void *address = malloc(size);
  assert(address != NULL && "Malloc: failed, returned address should never be NULL");
  return address;
}

void *Realloc(void *block, size_t size) {
  assert(size > 0 && "Realloc: size cant be negative");

  void *address = realloc(block, size);
  assert(address != NULL && "Realloc: failed, returned address should never be NULL");
  return address;
}

void Free(void *address) {
  free(address);
}

/* String Implementation */
static size_t maxStringSize = 10000;

static size_t strLength(char *str, size_t maxSize) {
  if (str == NULL) {
    return 0;
  }

  size_t len = 0;
  while (len < maxSize && str[len] != '\0') {
    len++;
  }

  return len;
}

static void addNullTerminator(char *str, size_t len) {
  str[len] = '\0';
}

bool StrIsNull(String *str) {
  return str == NULL || str->data == NULL;
}

void SetMaxStrSize(size_t size) {
  maxStringSize = size;
}

String StrNewSize(Arena *arena, char *str, size_t len) {
  const size_t memorySize = sizeof(char) * len + 1; // NOTE: Includes null terminator
  char *allocatedString = ArenaAlloc(arena, memorySize);

  memcpy(allocatedString, str, memorySize);
  addNullTerminator(allocatedString, len);
  return (String){len, allocatedString};
}

String StrNew(Arena *arena, char *str) {
  const size_t len = strLength(str, maxStringSize);
  if (len == 0) {
    return (String){0, NULL};
  }
  const size_t memorySize = sizeof(char) * len + 1; // NOTE: Includes null terminator
  char *allocatedString = ArenaAlloc(arena, memorySize);

  memcpy(allocatedString, str, memorySize);
  addNullTerminator(allocatedString, len);
  return (String){len, allocatedString};
}

String s(char *msg) {
  return (String){
      .length = strlen(msg),
      .data = msg,
  };
}

String StrConcat(Arena *arena, String *string1, String *string2) {
  assert(!StrIsNull(string1) && "StrConcat: string1 should never be NULL");
  assert(!StrIsNull(string2) && "StrConcat: string2 should never be NULL");

  const size_t len = string1->length + string2->length;
  const size_t memorySize = sizeof(char) * len + 1; // NOTE: Includes null terminator
  char *allocatedString = ArenaAlloc(arena, memorySize);

  memcpy_s(allocatedString, memorySize, string1->data, string1->length);
  memcpy_s(allocatedString + string1->length, memorySize, string2->data, string2->length);
  addNullTerminator(allocatedString, len);
  return (String){len, allocatedString};
}

void StrCopy(String *destination, String *source) {
  assert(!StrIsNull(destination) && "StrCopy: destination should never be NULL");
  assert(!StrIsNull(source) && "StrCopy: source should never be NULL");
  assert(destination->length >= source->length && "destination length should never smaller than source length");

  const errno_t result = memcpy_s(destination->data, destination->length, source->data, source->length);

  assert(result == 0 && "StrCopy: result should never be anything but 0");
  destination->length = source->length;
  addNullTerminator(destination->data, destination->length);
}

bool StrEqual(String *string1, String *string2) {
  if (string1->length != string2->length) {
    return false;
  }

  if (memcmp(string1->data, string2->data, string1->length) != 0) {
    return false;
  }
  return true;
}

// TODO: Change to (String *str, String delimiter);
// ^ Malloc strings instead, update StringVecFree
StringVector StrSplit(Arena *arena, String *str, String *delimiter) {
  assert(!StrIsNull(str) && "StrSplit: str should never be NULL");
  assert(!StrIsNull(delimiter) && "StrSplit: delimiter should never be NULL");

  char *start = str->data;
  const char *end = str->data + str->length;
  char *curr = start;
  StringVector result = {0};
  if (delimiter->length == 0) {
    for (size_t i = 0; i < str->length; i++) {
      String currString = StrNewSize(arena, str->data + i, 1);
      VecPush(result, currString);
    }
    return result;
  }

  while (curr < end) {
    char *match = NULL;
    for (char *search = curr; search <= end - delimiter->length; search++) {
      if (memcmp(search, delimiter->data, delimiter->length) == 0) {
        match = search;
        break;
      }
    }

    if (!match) {
      String currString = StrNewSize(arena, curr, end - curr);
      VecPush(result, currString);
      break;
    }

    size_t len = match - curr;
    String currString = StrNewSize(arena, curr, len);
    VecPush(result, currString);

    curr = match + delimiter->length;
  }

  return result;
}

void StringToUpper(String *str) {
  for (size_t i = 0; i < str->length; ++i) {
    char currChar = str->data[i];
    str->data[i] = toupper(currChar);
  }
}

void StrToLower(String *str) {
  for (size_t i = 0; i < str->length; ++i) {
    char currChar = str->data[i];
    str->data[i] = tolower(currChar);
  }
}

bool isSpace(char character) {
  return character == ' ' || character == '\n' || character == '\t' || character == '\r';
}

void StrTrim(String *str) {
  char *firstChar = NULL;
  char *lastChar = NULL;
  if (str->length == 0) {
    return;
  }

  if (str->length == 1) {
    if (isSpace(str->data[0])) {
      str->data[0] = '\0';
      str->length = 0;
    }
    return;
  }

  for (size_t i = 0; i < str->length; ++i) {
    char *currChar = &str->data[i];
    if (isSpace(*currChar)) {
      continue;
    }

    if (firstChar == NULL) {
      firstChar = currChar;
    }
    lastChar = currChar;
  }

  if (firstChar == NULL || lastChar == NULL) {
    str->data[0] = '\0';
    str->length = 0;
    addNullTerminator(str->data, 0);
    return;
  }

  size_t len = (lastChar - firstChar) + 1;
  memcpy_s(str->data, str->length, firstChar, len);
  str->length = len;
  addNullTerminator(str->data, len);
}

String StrSlice(Arena *arena, String *str, size_t start, size_t end) {
  assert(start >= 0 && "StrSlice: start index must be non-negative");
  assert(start <= str->length && "StrSlice: start index out of bounds");

  if (end < 0) {
    end = str->length + end;
  }

  assert(end >= start && "StrSlice: end must be greater than or equal to start");
  assert(end <= str->length && "StrSlice: end index out of bounds");

  size_t len = end - start;
  return StrNewSize(arena, str->data + start, len);
}

String F(Arena *arena, const char *format, ...) {
  va_list args;
  va_start(args, format);
  size_t size = vsnprintf(NULL, 0, format, args) + 1; // +1 for null terminator
  va_end(args);

  char *buffer = (char *)ArenaAlloc(arena, size);
  va_start(args, format);
  vsnprintf(buffer, size, format, args);
  va_end(args);

  return (String){.length = size - 1, .data = buffer};
}

String ConvertPath(Arena *arena, String path) {
  String platform = GetPlatform();
  String result;

  if (path.length >= 2 && path.data[0] == '.' && (path.data[1] == '/' || path.data[1] == '\\')) {
    result = StrNewSize(arena, path.data + 2, path.length - 2);
    memcpy(result.data, path.data + 2, path.length - 2);
  } else {
    result = StrNewSize(arena, path.data, path.length);
  }

  if (StrEqual(&platform, &S("linux")) || StrEqual(&platform, &S("macos"))) {
    return result;
  }

  for (size_t i = 0; i < result.length; i++) {
    if (result.data[i] == '/') {
      result.data[i] = '\\';
    }
  }

  return result;
}

String ParsePath(Arena *arena, String path) {
  String result;

  if (path.length >= 2 && path.data[0] == '.' && (path.data[1] == '/' || path.data[1] == '\\')) {
    result = StrNewSize(arena, path.data + 2, path.length - 2);
    memcpy(result.data, path.data + 2, path.length - 2);
  } else {
    result = StrNewSize(arena, path.data, path.length);
  }

  return result;
}

String ConvertExe(Arena *arena, String path) {
  String platform = GetPlatform();
  String exeExtension = S(".exe");

  bool hasExe = false;
  if (path.length >= exeExtension.length) {
    String pathEnd = StrSlice(arena, &path, path.length - exeExtension.length, path.length);
    if (StrEqual(&pathEnd, &exeExtension)) {
      hasExe = true;
    }
  }

  if (StrEqual(&platform, &S("windows"))) {
    if (hasExe) {
      return path;
    }
    return StrConcat(arena, &path, &exeExtension);
  }

  if (StrEqual(&platform, &S("linux")) || StrEqual(&platform, &S("macos"))) {
    if (hasExe) {
      return StrSlice(arena, &path, 0, path.length - exeExtension.length);
    }
    return path;
  }

  return path;
}

/* Random Implemenation */
static u64 seed = 0;
void RandomInit() {
  seed = TimeNow();
  srand(seed);
}

u64 RandomGetSeed() {
  return seed;
}

void RandomSetSeed(u64 newSeed) {
  seed = newSeed;
}

i32 RandomInteger(i32 min, i32 max) {
  assert(min <= max && "RandomInteger: min should always be less than or equal to max");
  assert(max - min <= INT32_MAX - 1 && "RandomInteger: range too large");

  i32 range = max - min + 1;

  // Calculate scaling factor to avoid modulo bias
  u32 buckets = RAND_MAX / range;
  u32 limit = buckets * range;

  // Reject numbers that would create bias
  u32 r;
  do {
    r = rand();
  } while (r >= limit);

  return min + (r / buckets);
}

f32 RandomFloat(f32 min, f32 max) {
  assert(min <= max && "RandomFloat: min must be less than or equal to max");
  f32 normalized = (f32)rand() / (f32)RAND_MAX;
  return min + normalized * (max - min);
}

/* File System Implementation */
#  if defined(PLATFORM_WIN)
char *GetCwd() {
  static _Thread_local char currentPath[MAX_PATH];
  DWORD length = GetCurrentDirectory(MAX_PATH, currentPath);
  if (length == 0) {
    LogError("Error getting current directory: %lu", GetLastError());
    currentPath[0] = '\0';
  }
  return currentPath;
}

void SetCwd(char *destination) {
  bool result = SetCurrentDirectory(destination);
  if (!result) {
    LogError("Error setting cwd: %lu", GetLastError());
  }
  GetCwd();
}

FileData *GetDirFiles() {
  WIN32_FIND_DATA findData;
  HANDLE hFind;
  char searchPath[MAX_PATH];
  FileData *fileData = NewFileData();
  i32 result = snprintf(searchPath, MAX_PATH - 2, "%s\\*", GetCwd());
  assert(result >= 0 && "GetDirFiles: sprint should not return error");

  hFind = FindFirstFile(searchPath, &findData);
  if (hFind == INVALID_HANDLE_VALUE) {
    LogError("Error finding files: %lu", GetLastError());
    return NULL;
  }

  do {
    if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
      continue;
    }

    if (fileData->totalCount >= MAX_FILES) {
      LogError("Warning: Maximum file count reached (%d). Some files might be skipped.", MAX_FILES);
      break;
    }

    bool isDirectory = findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    File *currFile = &fileData->files[fileData->fileCount];
    Folder *currFolder = &fileData->folders[fileData->folderCount];

    if (isDirectory) {
      currFolder->name = strdup(findData.cFileName);
      fileData->folderCount++;
    }

    if (!isDirectory) {
      char *dot = strrchr(findData.cFileName, '.');
      if (dot != NULL) {
        currFile->extension = strdup(dot + 1);

        size_t baseNameLength = dot - findData.cFileName;
        char *baseName = (char *)Malloc(baseNameLength + 1);
        memcpy(baseName, findData.cFileName, baseNameLength);
        baseName[baseNameLength] = '\0';
        currFile->name = baseName;
      }

      if (dot == NULL) {
        currFile->extension = strdup("");
        currFile->name = strdup(findData.cFileName);
      }

      LARGE_INTEGER createTime, modifyTime;
      createTime.LowPart = findData.ftCreationTime.dwLowDateTime;
      createTime.HighPart = findData.ftCreationTime.dwHighDateTime;
      modifyTime.LowPart = findData.ftLastWriteTime.dwLowDateTime;
      modifyTime.HighPart = findData.ftLastWriteTime.dwHighDateTime;

      const i64 WINDOWS_TICK = 10000000;
      const i64 SEC_TO_UNIX_EPOCH = 11644473600LL;

      currFile->createTime = createTime.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH;
      currFile->modifyTime = modifyTime.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH;
      currFile->size = (((i64)findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
      fileData->fileCount++;
    }

    fileData->totalCount++;
  } while (FindNextFile(hFind, &findData) != 0);

  DWORD dwError = GetLastError();
  if (dwError != ERROR_NO_MORE_FILES) {
    LogError("Error searching for files: %lu", dwError);
  }

  FindClose(hFind);
  return fileData;
}

FileData *NewFileData() {
  FileData *fileData = (FileData *)Malloc(sizeof(FileData));
  fileData->files = (File *)Malloc(MAX_FILES * sizeof(File));
  fileData->fileCount = 0;
  fileData->folders = (Folder *)Malloc(MAX_FILES * sizeof(Folder));
  fileData->folderCount = 0;
  fileData->totalCount = 0;
  return fileData;
};

void FreeFileData(FileData *fileData) {
  if (fileData->files == NULL && fileData->folders == NULL) return;

  for (size_t i = 0; i < fileData->fileCount; i++) {
    File currentFile = fileData->files[i];
    Free(currentFile.name);
    Free(currentFile.extension);
  }

  Free(fileData->files);

  for (size_t i = 0; i < fileData->folderCount; i++) {
    Folder currentFolder = fileData->folders[i];
    Free(currentFolder.name);
  }

  Free(fileData->folders);
  Free(fileData);
}

errno_t FileStats(String *path, File *result) {
  WIN32_FILE_ATTRIBUTE_DATA fileAttr = {0};

  if (!GetFileAttributesExA(path->data, GetFileExInfoStandard, &fileAttr)) {
    LogError("Failed to get file attributes: %lu", GetLastError());
    return FILE_GET_ATTRIBUTES_FAILED;
  }

  char *nameStart = strrchr(path->data, '\\');
  if (!nameStart) {
    nameStart = strrchr(path->data, '/');
  }

  if (nameStart) {
    nameStart++;
  } else {
    nameStart = path->data;
  }

  result->name = strdup(nameStart);

  char *extStart = strrchr(nameStart, '.');
  if (extStart) {
    result->extension = strdup(extStart + 1);
  } else {
    result->extension = strdup("");
  }

  LARGE_INTEGER fileSize;
  fileSize.HighPart = fileAttr.nFileSizeHigh;
  fileSize.LowPart = fileAttr.nFileSizeLow;
  result->size = fileSize.QuadPart;

  LARGE_INTEGER createTime, modifyTime;
  createTime.LowPart = fileAttr.ftCreationTime.dwLowDateTime;
  createTime.HighPart = fileAttr.ftCreationTime.dwHighDateTime;
  modifyTime.LowPart = fileAttr.ftLastWriteTime.dwLowDateTime;
  modifyTime.HighPart = fileAttr.ftLastWriteTime.dwHighDateTime;

  const i64 WINDOWS_TICK = 10000000;
  const i64 SEC_TO_UNIX_EPOCH = 11644473600LL;

  result->createTime = createTime.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH;
  result->modifyTime = modifyTime.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH;

  return SUCCESS;
}

errno_t FileRead(Arena *arena, String *path, String *result) {
  HANDLE hFile = INVALID_HANDLE_VALUE;

  hFile = CreateFileA(path->data, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();

    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
      return FILE_NOT_EXIST;
    }

    LogError("File open failed, err: %lu", error);
    return FILE_OPEN_FAILED;
  }

  LARGE_INTEGER fileSize;
  if (!GetFileSizeEx(hFile, &fileSize)) {
    LogError("Failed to get file size: %lu\n", GetLastError());
    CloseHandle(hFile);
    return FILE_GET_SIZE_FAILED;
  }

  DWORD bytesRead;
  char *buffer = (char *)ArenaAlloc(arena, fileSize.QuadPart);
  if (!ReadFile(hFile, buffer, (DWORD)fileSize.QuadPart, &bytesRead, NULL) || bytesRead != fileSize.QuadPart) {
    LogError("Failed to read file: %lu", GetLastError());
    CloseHandle(hFile);
    return FILE_READ_FAILED;
  }

  *result = (String){.length = bytesRead, .data = buffer};

  CloseHandle(hFile);
  return SUCCESS;
}

errno_t FileWrite(String *path, String *data) {
  HANDLE hFile = INVALID_HANDLE_VALUE;

  hFile = CreateFileA(path->data, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    LogError("File open failed, err: %lu", error);

    switch (error) {
    case ERROR_ACCESS_DENIED:
      return FILE_WRITE_ACCESS_DENIED;
    case ERROR_FILE_NOT_FOUND:
      return FILE_WRITE_NOT_FOUND;
    default:
      return FILE_WRITE_OPEN_FAILED;
    }
  }

  DWORD bytesWritten;
  if (!WriteFile(hFile, data->data, (DWORD)data->length, &bytesWritten, NULL) || bytesWritten != data->length) {
    DWORD error = GetLastError();
    CloseHandle(hFile);

    switch (error) {
    case ERROR_DISK_FULL:
      return FILE_WRITE_DISK_FULL;
    default:
      return FILE_WRITE_IO_ERROR;
    }
  }

  CloseHandle(hFile);

  return SUCCESS;
}

errno_t FileAdd(String *path, String *data) {
  HANDLE hFile = INVALID_HANDLE_VALUE;
  hFile = CreateFileA(path->data, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    LogError("File open failed, err: %lu", error);

    switch (error) {
    case ERROR_ACCESS_DENIED:
      return FILE_ADD_ACCESS_DENIED;
    case ERROR_FILE_NOT_FOUND:
      return FILE_ADD_NOT_FOUND;
    default:
      return FILE_ADD_OPEN_FAILED;
    }
  }

  if (SetFilePointer(hFile, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) {
    DWORD error = GetLastError();
    CloseHandle(hFile);
    return FILE_ADD_IO_ERROR;
  }

  char *newData = Malloc(data->length + 1); // NOTE: +1 for \n
  memcpy(newData, data->data, data->length);
  newData[data->length] = '\n';
  DWORD newLength = (DWORD)data->length + 1;
  DWORD bytesWritten;
  if (!WriteFile(hFile, newData, newLength, &bytesWritten, NULL) || bytesWritten != newLength) {
    DWORD error = GetLastError();
    CloseHandle(hFile);
    Free(newData);
    switch (error) {
    case ERROR_DISK_FULL:
      return FILE_ADD_DISK_FULL;
    default:
      return FILE_ADD_IO_ERROR;
    }
  }

  CloseHandle(hFile);
  Free(newData);
  return SUCCESS;
}

errno_t FileDelete(String *path) {
  if (!DeleteFileA(path->data)) {
    DWORD error = GetLastError();
    switch (error) {
    case ERROR_ACCESS_DENIED:
      return FILE_DELETE_ACCESS_DENIED;
    case ERROR_FILE_NOT_FOUND:
      return FILE_DELETE_NOT_FOUND;
    default:
      return FILE_DELETE_IO_ERROR;
    }
  }

  return SUCCESS;
}

errno_t FileRename(String *oldPath, String *newPath) {
  if (!MoveFileEx(oldPath->data, newPath->data, MOVEFILE_REPLACE_EXISTING)) {
    DWORD error = GetLastError();
    switch (error) {
    case ERROR_ACCESS_DENIED:
      return FILE_RENAME_ACCESS_DENIED;
    case ERROR_FILE_NOT_FOUND:
      return FILE_RENAME_NOT_FOUND;
    case ERROR_ALREADY_EXISTS:
      return FILE_RENAME_EXISTS;
    default:
      return FILE_RENAME_IO_ERROR;
    }
  }

  return SUCCESS;
}

bool Mkdir(String path) {
  bool result = CreateDirectory(path.data, NULL);
  if (result != false) {
    return true;
  }

  u64 error = GetLastError();
  if (error == ERROR_ALREADY_EXISTS) {
    return true;
  }

  LogError("Error meanwhile Mkdir() %llu", error);
  return false;
}
#  elif defined(PLATFORM_LINUX)
char *GetCwd() {
  static _Thread_local char currentPath[PATH_MAX];
  if (getcwd(currentPath, PATH_MAX) == NULL) {
    LogError("Error getting current directory: %s", strerror(errno));
    currentPath[0] = '\0';
  }
  return currentPath;
}

void SetCwd(char *destination) {
  if (chdir(destination) != 0) {
    LogError("Error setting cwd: %s", strerror(errno));
  }
  GetCwd();
}

errno_t FileStats(String *path, File *result) {
  struct stat fileStat;

  if (stat(path->data, &fileStat) != 0) {
    LogError("Failed to get file attributes: %d", errno);
    return FILE_GET_ATTRIBUTES_FAILED;
  }

  char *nameStart = strrchr(path->data, '/');
  if (nameStart) {
    nameStart++;
  } else {
    nameStart = path->data;
  }

  result->name = strdup(nameStart);

  char *extStart = strrchr(nameStart, '.');
  if (extStart) {
    result->extension = strdup(extStart + 1);
  } else {
    result->extension = strdup("");
  }

  result->size = fileStat.st_size;
  result->createTime = fileStat.st_ctime; // Creation time (may be change time on some Unix systems)
  result->modifyTime = fileStat.st_mtime; // Modification time

  return SUCCESS;
}

errno_t FileRead(Arena *arena, String *path, String *result) {
  i32 fd = -1;

  fd = open(path->data, O_RDONLY);
  if (fd == -1) {
    int error = errno;

    if (error == ENOENT) {
      return FILE_NOT_EXIST;
    }

    LogError("File open failed, err: %d", error);
    return FILE_OPEN_FAILED;
  }

  struct stat fileStat;
  if (fstat(fd, &fileStat) != 0) {
    LogError("Failed to get file size: %d", errno);
    close(fd);
    return FILE_GET_SIZE_FAILED;
  }

  off_t fileSize = fileStat.st_size;
  char *buffer = (char *)ArenaAlloc(arena, fileSize);
  ssize_t bytesRead = read(fd, buffer, fileSize);
  if (bytesRead != fileSize) {
    LogError("Failed to read file: %d", errno);
    close(fd);
    return FILE_READ_FAILED;
  }

  *result = (String){.length = bytesRead, .data = buffer};

  close(fd);
  return SUCCESS;
}

errno_t FileWrite(String *path, String *data) {
  i32 fd = -1;

  fd = open(path->data, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    i32 error = errno;
    LogError("File open failed, err: %d", error);

    switch (error) {
    case EACCES:
      return FILE_WRITE_ACCESS_DENIED;
    case ENOENT:
      return FILE_WRITE_NOT_FOUND;
    default:
      return FILE_WRITE_OPEN_FAILED;
    }
  }

  ssize_t bytesWritten = write(fd, data->data, data->length);
  if (bytesWritten != data->length) {
    int error = errno;
    close(fd);

    switch (error) {
    case ENOSPC:
      return FILE_WRITE_DISK_FULL;
    default:
      return FILE_WRITE_IO_ERROR;
    }
  }

  close(fd);

  return SUCCESS;
}

errno_t FileAdd(String *path, String *data) {
  i32 fd = -1;

  fd = open(path->data, O_WRONLY | O_APPEND | O_CREAT, 0644);
  if (fd == -1) {
    int error = errno;
    LogError("File open failed, err: %d", error);

    switch (error) {
    case EACCES:
      return FILE_ADD_ACCESS_DENIED;
    case ENOENT:
      return FILE_ADD_NOT_FOUND;
    default:
      return FILE_ADD_OPEN_FAILED;
    }
  }

  char *newData = Malloc(data->length + 1); // +1 for \n
  memcpy(newData, data->data, data->length);
  newData[data->length] = '\n';
  size_t newLength = data->length + 1;

  ssize_t bytesWritten = write(fd, newData, newLength);
  if (bytesWritten != newLength) {
    int error = errno;
    close(fd);
    Free(newData);

    switch (error) {
    case ENOSPC:
      return FILE_ADD_DISK_FULL;
    default:
      return FILE_ADD_IO_ERROR;
    }
  }

  close(fd);
  Free(newData);
  return SUCCESS;
}

errno_t FileDelete(String *path) {
  if (unlink(path->data) != 0) {
    int error = errno;

    switch (error) {
    case EACCES:
      return FILE_DELETE_ACCESS_DENIED;
    case ENOENT:
      return FILE_DELETE_NOT_FOUND;
    default:
      return FILE_DELETE_IO_ERROR;
    }
  }

  return SUCCESS;
}

errno_t FileRename(String *oldPath, String *newPath) {
  if (rename(oldPath->data, newPath->data) != 0) {
    errno_t error = errno;

    switch (error) {
    case EACCES:
      return FILE_RENAME_ACCESS_DENIED;
    case ENOENT:
      return FILE_RENAME_NOT_FOUND;
    case EEXIST:
      return FILE_RENAME_EXISTS;
    default:
      return FILE_RENAME_IO_ERROR;
    }
  }

  return SUCCESS;
}

bool Mkdir(String path) {
  struct stat st;
  if (stat(path.data, &st) == 0 && S_ISDIR(st.st_mode)) {
    return true; // Directory already exists
  }

  if (mkdir(path.data, 0755) != 0) {
    if (errno != EEXIST) {
      LogError("Failed to create directory: %s", strerror(errno));
      return false;
    }
  }

  return true;
}

FileData *NewFileData() {
  FileData *data = (FileData *)Malloc(sizeof(FileData));

  data->folders = (Folder *)Malloc(MAX_FILES * sizeof(Folder));
  data->files = (File *)Malloc(MAX_FILES * sizeof(File));

  data->folderCount = 0;
  data->fileCount = 0;
  data->totalCount = 0;

  return data;
}

void FreeFileData(FileData *data) {
  if (!data) return;

  for (size_t i = 0; i < data->folderCount; i++) {
    Free(data->folders[i].name);
  }

  for (size_t i = 0; i < data->fileCount; i++) {
    Free(data->files[i].name);
    Free(data->files[i].extension);
  }

  Free(data->folders);
  Free(data->files);
  Free(data);
}

FileData *GetDirFiles() {
  FileData *data = NewFileData();
  if (!data) {
    return NULL;
  }

  DIR *dir = opendir(".");
  if (!dir) {
    LogError("Failed to open directory: %s", strerror(errno));
    FreeFileData(data);
    return NULL;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    if (data->totalCount >= MAX_FILES) {
      LogError("Maximum file count (%d) reached", MAX_FILES);
      break;
    }

    struct stat entryStat;
    if (stat(entry->d_name, &entryStat) != 0) {
      LogError("Failed to stat entry %s: %s", entry->d_name, strerror(errno));
      continue;
    }

    if (S_ISDIR(entryStat.st_mode)) {
      data->folders[data->folderCount].name = strdup(entry->d_name);
      if (!data->folders[data->folderCount].name) {
        LogError("Failed to allocate memory for folder name");
        continue;
      }
      data->folderCount++;
    } else if (S_ISREG(entryStat.st_mode)) {
      File *currentFile = &data->files[data->fileCount];

      currentFile->name = strdup(entry->d_name);
      if (!currentFile->name) {
        LogError("Failed to allocate memory for file name");
        continue;
      }

      char *extStart = strrchr(entry->d_name, '.');
      if (extStart) {
        currentFile->extension = strdup(extStart + 1);
      } else {
        currentFile->extension = strdup("");
      }

      if (!currentFile->extension) {
        LogError("Failed to allocate memory for extension");
        Free(currentFile->name);
        continue;
      }

      currentFile->size = entryStat.st_size;
      currentFile->createTime = entryStat.st_ctime;
      currentFile->modifyTime = entryStat.st_mtime;

      data->fileCount++;
    }

    data->totalCount++;
  }

  closedir(dir);
  return data;
}
#  endif

/* Logger Implemenation */
void LogInfo(const char *format, ...) {
  printf("%s[INFO]: ", _GRAY);
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("%s\n", _RESET);
}

void LogWarn(const char *format, ...) {
  printf("%s[WARN]: ", _ORANGE);
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("%s\n", _RESET);
}

void LogError(const char *format, ...) {
  printf("%s[ERROR]: ", _RED);
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("%s\n", _RESET);
}

void LogSuccess(const char *format, ...) {
  printf("%s[SUCCESS]: ", _GREEN);
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("%s\n", _RESET);
}

void LogInit() {
#  if defined(PLATFORM_WIN)
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
#  endif
}
#endif
// --- BASE.H END ---

// --- MATE.H START ---
/* MIT License
   mate.h - Mate Definitions start here
   Guide on the `README.md`
*/
typedef struct {
  i64 lastBuild;
  bool firstBuild;
} MateCache;

typedef struct {
  char *compiler;
  char *buildDirectory;
  char *mateSource;
  char *mateExe;
  char *mateCachePath;
} MateOptions;

typedef struct {
  String compiler;

  // Paths
  String buildDirectory;
  String mateSource;
  String mateExe;
  String mateCachePath;

  // Cache
  MateCache mateCache;

  // Misc
  Arena arena;
  bool customConfig;
  i64 startTime;
  i64 totalTime;
} MateConfig;

typedef struct {
  String output;
  String flags;
  String linkerFlags;
  // TODO: add target
  // TODO: optimize
  // TODO: warnings
  // TODO: debugSymbols
  String includes;
  String libs;
  StringVector sources;
} Executable;

typedef struct {
  char *output;
  char *flags;
  char *linkerFlags;
  char *includes;
  char *libs;
} ExecutableOptions;

void CreateConfig(MateOptions options);
void StartBuild();
void reBuild();
void CreateExecutable(ExecutableOptions executableOptions);

#define AddLibraryPaths(...)                   \
  ({                                           \
    StringVector vector = {0};                 \
    StringVectorPushMany(vector, __VA_ARGS__); \
    addLibraryPaths(&vector);                  \
  })
static void addLibraryPaths(StringVector *vector);

#define AddIncludePaths(...)                   \
  ({                                           \
    StringVector vector = {0};                 \
    StringVectorPushMany(vector, __VA_ARGS__); \
    addIncludePaths(&vector);                  \
  })
static void addIncludePaths(StringVector *vector);

#define LinkSystemLibraries(...)               \
  ({                                           \
    StringVector vector = {0};                 \
    StringVectorPushMany(vector, __VA_ARGS__); \
    linkSystemLibraries(&vector);              \
  })
static void linkSystemLibraries(StringVector *vector);

#define AddFile(source) addFile(S(source));
static void addFile(String source);
String InstallExecutable();
i32 RunCommand(String command);
void EndBuild();

static bool needRebuild();
static void setDefaultState();

/* MIT License
   mate.h - Mate Implementations start here
   Guide on the `README.md`
*/
#ifdef MATE_IMPLEMENTATION
static MateConfig state = {0};
static Executable executable = {0};

String FixPathExe(String *str) {
  String path = ConvertPath(&state.arena, ConvertExe(&state.arena, *str));
#  if defined(PLATFORM_WIN)
  return F(&state.arena, "%s\\%s", GetCwd(), path.data);
#  elif defined(PLATFORM_LINUX)
  return F(&state.arena, "%s/%s", GetCwd(), path.data);
#  endif
}

String FixPath(String *str) {
  String path = ConvertPath(&state.arena, *str);
#  if defined(PLATFORM_WIN)
  return F(&state.arena, "%s\\%s", GetCwd(), path.data);
#  elif defined(PLATFORM_LINUX)
  return F(&state.arena, "%s/%s", GetCwd(), path.data);
#  endif
}

String ConvertNinjaPath(String str) {
  String copy = StrNewSize(&state.arena, str.data, str.length + 1);
  memmove(&copy.data[2], &copy.data[1], str.length - 1);
  copy.data[1] = '$';
  copy.data[2] = ':';
  return copy;
}

static void setDefaultState() {
  state.arena = ArenaInit(12000 * sizeof(String));
  state.mateExe = FixPath(&S("./mate"));
  state.compiler = GetCompiler();
  state.mateSource = FixPath(&S("./mate.c"));
  state.mateCachePath = FixPath(&S("./build/mate-cache.json"));
  state.buildDirectory = ConvertPath(&state.arena, S("./build"));
}

static MateConfig parseMateConfig(MateOptions options) {
  MateConfig result;
  result.mateExe = StrNew(&state.arena, options.mateExe);
  result.compiler = StrNew(&state.arena, options.compiler);
  result.mateSource = StrNew(&state.arena, options.mateSource);
  result.mateCachePath = StrNew(&state.arena, options.mateCachePath);
  result.buildDirectory = StrNew(&state.arena, options.buildDirectory);
  return result;
}

void CreateConfig(MateOptions options) {
  setDefaultState();
  MateConfig config = parseMateConfig(options);

  if (!StrIsNull(&config.mateSource)) {
    state.mateSource = FixPath(&config.mateSource);
  }

  if (!StrIsNull(&config.mateCachePath)) {
    state.mateCachePath = FixPath(&config.mateCachePath);
  }

  if (!StrIsNull(&config.mateExe)) {
    state.mateExe = FixPath(&config.mateExe);
  }

  if (!StrIsNull(&config.buildDirectory)) {
    state.buildDirectory = ConvertPath(&state.arena, config.buildDirectory);
  }

  if (!StrIsNull(&config.compiler)) {
    state.compiler = config.compiler;
  }

  state.customConfig = true;
}

errno_t readCache() {
  String cache = {0};
  errno_t err = FileRead(&state.arena, &state.mateCachePath, &cache);

  if (err == FILE_NOT_EXIST) {
    String modifyTime = F(&state.arena, "%llu", TimeNow() / 1000);
    FileWrite(&state.mateCachePath, &modifyTime);
    state.mateCache.firstBuild = true;
    cache = modifyTime;
  } else if (err != SUCCESS) {
    return err;
  }

  char *endptr;
  state.mateCache.lastBuild = strtoll(cache.data, &endptr, 10);
  return SUCCESS;
}

void StartBuild() {
  LogInit();
  if (!state.customConfig) {
    setDefaultState();
  }

  state.startTime = TimeNow();
  Mkdir(state.buildDirectory);
  readCache();
  reBuild();
}

static bool needRebuild() {
  File stats = {0};
  errno_t result = FileStats(&state.mateSource, &stats);
  if (result != SUCCESS) {
    LogError("Could not read fileStats %d", result);
    LogError("Aborting rebuild");
    abort();
  }

  if (stats.modifyTime > state.mateCache.lastBuild) {
    String modifyTime = F(&state.arena, "%llu", stats.modifyTime);
    FileWrite(&state.mateCachePath, &modifyTime);
    return true;
  }

  return false;
}

void reBuild() {
  if (state.mateCache.firstBuild || !needRebuild()) {
    return;
  }

  String mateExeNew = F(&state.arena, "%s/mate-new", state.buildDirectory.data);
  String mateExeOld = F(&state.arena, "%s/mate-old", state.buildDirectory.data);
  String mateExe = ConvertExe(&state.arena, state.mateExe);
  mateExeNew = FixPathExe(&mateExeNew);
  mateExeOld = FixPathExe(&mateExeOld);

  String compileCommand;
  if (StrEqual(&state.compiler, &S("gcc"))) {
    compileCommand = F(&state.arena, "gcc -o \"%s\" -Wall -g \"%s\"", mateExeNew.data, state.mateSource.data);
  }

  if (StrEqual(&state.compiler, &S("clang"))) {
    compileCommand = F(&state.arena, "clang -o \"%s\" -Wall -g \"%s\"", mateExeNew.data, state.mateSource.data);
  }

  if (StrEqual(&state.compiler, &S("MSVC"))) {
    compileCommand = F(&state.arena, "cl.exe /Fe:\"%s\" /W4 /Zi \"%s\"", mateExeNew.data, state.mateSource.data);
  }

  LogWarn("%s changed rebuilding...", state.mateSource.data);
  errno_t rebuildResult = RunCommand(compileCommand);
  if (rebuildResult != 0) {
    LogError("Rebuild failed, with error: %d", rebuildResult);
    abort();
  }

  errno_t renameResult = FileRename(&mateExe, &mateExeOld);
  if (renameResult != SUCCESS) {
    LogError("Error renaming original executable: %d", renameResult);
    abort();
  }

  renameResult = FileRename(&mateExeNew, &mateExe);
  if (renameResult != SUCCESS) {
    LogError("Error moving new executable into place: %d", renameResult);
    FileRename(&mateExeOld, &state.mateExe);
    abort();
  }

  LogInfo("Rebuild finished, running %s", mateExe.data);
  errno_t result = RunCommand(mateExe);
  return exit(result);
}

void defaultExecutable() {
  String executableOutput = ConvertExe(&state.arena, S("main"));
  executable.output = ConvertPath(&state.arena, executableOutput);
  executable.flags = S("-Wall -g");
  executable.linkerFlags = S("");
  executable.includes = S("");
  executable.libs = S("");
}

static Executable parseExecutableOptions(ExecutableOptions options) {
  Executable result;
  result.output = StrNew(&state.arena, options.output);
  result.flags = StrNew(&state.arena, options.flags);
  result.linkerFlags = StrNew(&state.arena, options.linkerFlags);
  result.includes = StrNew(&state.arena, options.includes);
  result.libs = StrNew(&state.arena, options.libs);
  return result;
}

void CreateExecutable(ExecutableOptions executableOptions) {
  defaultExecutable();
  Executable options = parseExecutableOptions(executableOptions);

  if (!StrIsNull(&options.output)) {
    String executableOutput = ConvertExe(&state.arena, options.output);
    executable.output = ConvertPath(&state.arena, executableOutput);
  }

  if (!StrIsNull(&options.flags)) {
    executable.flags = options.flags;
  }

  if (!StrIsNull(&options.linkerFlags)) {
    executable.linkerFlags = options.linkerFlags;
  }

  if (!StrIsNull(&options.includes)) {
    executable.includes = options.includes;
  }

  if (!StrIsNull(&options.libs)) {
    executable.libs = options.libs;
  }
}

// TODO: Implement for linux
// TODO: Add error enum
errno_t CreateCompileCommands() {
  FILE *ninjaPipe;
  FILE *outputFile;
  char buffer[4096];
  size_t bytes_read;

  String buildPath = StrNew(&state.arena, F(&state.arena, "%s\\%s", GetCwd(), ParsePath(&state.arena, state.buildDirectory).data).data);
  String compileCommandsPath = ConvertPath(&state.arena, F(&state.arena, "%s/compile_commands.json", buildPath.data));

  errno_t err = fopen_s(&outputFile, compileCommandsPath.data, "w");
  if (err != 0 || outputFile == NULL) {
    LogError("Failed to open output file '%s': %s", compileCommandsPath.data, strerror(err));
    return 1;
  }

  String compdbCommand = ConvertPath(&state.arena, F(&state.arena, "ninja -f %s/build.ninja -t compdb", buildPath.data));

  ninjaPipe = popen(compdbCommand.data, "r");
  if (ninjaPipe == NULL) {
    LogError("Failed to run command");
    fclose(outputFile);
    return 1;
  }

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), ninjaPipe)) > 0) {
    fwrite(buffer, 1, bytes_read, outputFile);
  }

  fclose(outputFile);
  i32 status = pclose(ninjaPipe);
  if (status != 0) {
    LogError("Command failed with status %d\n", status);
    return status;
  }

  LogSuccess("Successfully created %s\n", compileCommandsPath.data);
  return SUCCESS;
}

static void addFile(String source) {
  VecPush(executable.sources, source);
}

static StringVector outputTransformer(StringVector vector) {
  StringVector result = {0};
  for (size_t i = 0; i < vector.length; i++) {
    String *currentExecutable = VecAt(vector, i);
    String output = S("");
    for (size_t j = currentExecutable->length - 1; j > 0; j--) {
      String currentChar = StrNewSize(&state.arena, &currentExecutable->data[j], 1);
      if (currentChar.data[0] == '/') {
        break;
      }
      output = StrConcat(&state.arena, &currentChar, &output);
    }
    output.data[output.length - 1] = 'o';
    VecPush(result, output);
  }

  return result;
}

// TODO: Make linux version
String InstallExecutable() {
  if (executable.sources.length == 0) {
    LogError("Executable has zero sources, add at least one with AddFile(\"./main.c\")");
    abort();
  }

  String linkCommand;
  String compileCommand;
  if (StrEqual(&state.compiler, &S("gcc"))) {
    linkCommand = F(&state.arena, "rule link\n  command = $cc $flags $linker_flags -o $out $in $libs\n");
    compileCommand = F(&state.arena, "rule compile\n  command = $cc $flags $includes -c $in -o $out\n");
  }

  if (StrEqual(&state.compiler, &S("clang"))) {
    linkCommand = F(&state.arena, "rule link\n  command = $cc $flags $linker_flags -o $out $in $libs\n");
    compileCommand = F(&state.arena, "rule compile\n  command = $cc $flags $includes -c $in -o $out\n");
  }

  if (StrEqual(&state.compiler, &S("MSVC"))) {
    LogError("MSVC not yet implemented");
    abort();
  }

  String ninjaOutput = F(&state.arena,
                         "cc = %s\n"
                         "linker_flag = %s\n"
                         "flags = %s\n"
                         "cwd = %s\n"
                         "builddir = $cwd/%s\n"
                         "target = $builddir/%s\n"
                         "includes = %s\n"
                         "libs = %s\n"
                         "\n"
                         "%s\n"
                         "%s\n",
                         state.compiler.data,
                         executable.linkerFlags.data,
                         executable.flags.data,
                         ConvertNinjaPath(StrNew(&state.arena, GetCwd())).data,
                         state.buildDirectory.data,
                         executable.output.data,
                         executable.includes.data,
                         executable.libs.data,
                         linkCommand.data,
                         compileCommand.data);
  StringVector outputFiles = outputTransformer(executable.sources);

  assert(outputFiles.length == executable.sources.length && "Something went wrong in the parsing");

  String outputString = S("");
  for (size_t i = 0; i < executable.sources.length; i++) {
    String sourceFile = ParsePath(&state.arena, *VecAt(executable.sources, i));
    String outputFile = *VecAt(outputFiles, i);
    String source = F(&state.arena, "build $builddir/%s: compile $cwd/%s\n", outputFile.data, sourceFile.data);
    ninjaOutput = StrConcat(&state.arena, &ninjaOutput, &source);
    outputString = F(&state.arena, "%s $builddir/%s", outputString.data, outputFile.data);
  }

  String target = F(&state.arena,
                    "build $target: link%s\n"
                    "\n"
                    "default $target\n",
                    outputString.data);
  ninjaOutput = StrConcat(&state.arena, &ninjaOutput, &target);

  String buildNinjaPath = F(&state.arena, "%s\\build.ninja", FixPath(&state.buildDirectory).data);
  FileWrite(&buildNinjaPath, &ninjaOutput);

  errno_t result = RunCommand(F(&state.arena, "ninja -f %s", buildNinjaPath.data));
  if (result != 0) {
    LogError("Ninja file compilation failed with code: %d", result);
    abort();
  }

  LogSuccess("Ninja file compilation done");
  state.totalTime = TimeNow() - state.startTime;
  return F(&state.arena, "%s\\%s", FixPath(&state.buildDirectory).data, executable.output.data);
}

errno_t RunCommand(String command) {
  return system(command.data);
}

// TODO: Allocate total string size instead of concat
// TODO: Do it depending on compiler
static void addLibraryPaths(StringVector *vector) {
  for (size_t i = 0; i < vector->length; i++) {
    String *currLib = VecAt((*vector), i);
    if (i == 0 && executable.libs.length == 0) {
      executable.libs = F(&state.arena, "-L\"%s\"", currLib->data);
      continue;
    }

    executable.libs = F(&state.arena, "%s -L\"%s\"", executable.libs.data, currLib->data);
  }
}

// TODO: Same thing here
static void addIncludePaths(StringVector *vector) {
  for (size_t i = 0; i < vector->length; i++) {
    String *currInclude = VecAt((*vector), i);
    if (i == 0 && executable.includes.length == 0) {
      executable.includes = F(&state.arena, "-I\"%s\"", currInclude->data);
      continue;
    }

    executable.includes = F(&state.arena, "%s -I\"%s\"", executable.includes.data, currInclude->data);
  }
}

static void linkSystemLibraries(StringVector *vector) {
  for (size_t i = 0; i < vector->length; i++) {
    String *currLib = VecAt((*vector), i);
    if (i == 0 && executable.libs.length == 0) {
      executable.libs = F(&state.arena, "-l%s", currLib->data);
      continue;
    }

    executable.libs = F(&state.arena, "%s -l%s", executable.libs.data, currLib->data);
  }
}

void EndBuild() {
  LogInfo("Build took: %llums", state.totalTime);
  ArenaFree(&state.arena);
}
#endif
// --- MATE.H END ---
