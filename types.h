// types.h
// Provides: u8,u16,u32,u64,i8,i16,i32,i64,f32,f64,usize,isize,uptr,iptr,
//           optional u128/i128 on compilers that support __int128,
//           min/max constants and printf format helpers.

#ifndef TYPES_H
#define TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <float.h>

// fixed-width integers
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

// pointer-sized integers
typedef size_t    usize;
typedef ptrdiff_t isize;
typedef uintptr_t uptr;
typedef intptr_t  iptr;

// floats
typedef float  f32;
typedef double f64;

// 128-bit ints
typedef unsigned long long u128;
typedef long long          i128;

// compile-time size checks (C11)
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(u8)  == 1, "u8 must be 1 byte");
_Static_assert(sizeof(i8)  == 1, "i8 must be 1 byte");
_Static_assert(sizeof(u16) == 2, "u16 must be 2 bytes");
_Static_assert(sizeof(i16) == 2, "i16 must be 2 bytes");
_Static_assert(sizeof(u32) == 4, "u32 must be 4 bytes");
_Static_assert(sizeof(i32) == 4, "i32 must be 4 bytes");
_Static_assert(sizeof(u64) == 8, "u64 must be 8 bytes");
_Static_assert(sizeof(i64) == 8, "i64 must be 8 bytes");
_Static_assert(sizeof(f32) == 4, "f32 must be 4 bytes");
_Static_assert(sizeof(f64) == 8, "f64 must be 8 bytes");
#endif

#define U8_MAX   UINT8_MAX
#define U16_MAX  UINT16_MAX
#define U32_MAX  UINT32_MAX
#define U64_MAX  UINT64_MAX

#define I8_MIN   INT8_MIN
#define I8_MAX   INT8_MAX
#define I16_MIN  INT16_MIN
#define I16_MAX  INT16_MAX
#define I32_MIN  INT32_MIN
#define I32_MAX  INT32_MAX
#define I64_MIN  INT64_MIN
#define I64_MAX  INT64_MAX

#define USIZE_MAX  SIZE_MAX
#define ISIZE_MIN  PTRDIFF_MIN
#define ISIZE_MAX  PTRDIFF_MAX
#define UPTR_MAX   UINTPTR_MAX
#define IPTR_MIN   INTPTR_MIN
#define IPTR_MAX   INTPTR_MAX

// note: FLT_MIN/DBL_MIN are smallest positive normals, not most-negative
#define F32_MIN_POS  FLT_MIN
#define F32_MAX      FLT_MAX
#define F64_MIN_POS  DBL_MIN
#define F64_MAX      DBL_MAX

#define USIZE_MAX  SIZE_MAX
#define ISIZE_MIN  PTRDIFF_MIN
#define ISIZE_MAX  PTRDIFF_MAX
#define UPTR_MAX   UINTPTR_MAX
#define IPTR_MIN   INTPTR_MIN
#define IPTR_MAX   INTPTR_MAX

#define F32_MIN    FLT_MIN
#define F32_MAX    FLT_MAX
#define F64_MIN    DBL_MIN
#define F64_MAX    DBL_MAX

// printf format helpers
// usage: printf("val=%" U64_FMT "\n", (u64)val);
#define U8_FMT   PRIu8
#define I8_FMT   PRId8
#define U16_FMT  PRIu16
#define I16_FMT  PRId16
#define U32_FMT  PRIu32
#define I32_FMT  PRId32
#define U64_FMT  PRIu64
#define I64_FMT  PRId64

// size and pointer formats
#define USIZE_FMT "zu"
#define ISIZE_FMT "zd"
#define UPTR_FMT  PRIuPTR
#define IPTR_FMT  PRIdPTR
#define HEXPTR_FMT PRIxPTR

// literal helpers
// usage: (u64)U64_C(123) or I64_C(-5)
#define U64_C(x)  UINT64_C(x)
#define I64_C(x)  INT64_C(x)

// byte alias (explicit, to avoid confusion with char)
typedef u8 byte;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // TYPES_H
