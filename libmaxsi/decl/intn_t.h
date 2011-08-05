#ifndef _INTN_T_DECL
#define _INTN_T_DECL

/* Define basic signed types. */
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;

/* Define basic unsigned types. */
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;

/* The maximum width integers available on this platform. */
typedef __intmax_t intmax_t;
typedef __uintmax_t uintmax_t;

/* Define an integer able to hold the size of the largest continious memory */
/* region and define pointer safe integer types. */
typedef __size_t size_t;
typedef __ssize_t ssize_t;
typedef __intptr_t intptr_t;
typedef __uintptr_t uintptr_t;
typedef __ptrdiff_t ptrdiff_t;

#define INT8_MIN __INT8_MIN
#define INT16_MIN __INT16_MIN
#define INT32_MIN __INT32_MIN
#define INT64_MIN __INT64_MIN
#define INT8_MAX __INT8_MAX
#define INT16_MAX __INT16_MAX
#define INT32_MAX __INT32_MAX
#define INT64_MAX __INT64_MAX
#define UINT8_MAX __UINT8_MAX
#define UINT16_MAX __UINT16_MAX
#define UINT32_MAX __UINT32_MAX
#define UINT64_MAX __UINT64_MAX

#define INTMAX_MIN __INTMAX_MIN
#define INTMAX_MAX __INTMAX_MAX
#define UINTMAX_MAX __UINTMAX_MAX

#define CHAR_BIT __CHAR_BIT
#define SCHAR_MIN __SCHAR_MIN
#define SCHAR_MAX __SCHAR_MAX
#define UCHAR_MAX __UCHAR_MAX
#define CHAR_MIN __CHAR_MIN
#define CHAR_MAX __CHAR_MAX

#define WCHAR_MIN __WCHAR_MIN
#define WCHAR_MAX __WCHAR_MAX

#define SHRT_MIN __SHRT_MIN
#define SHRT_MAX __SHRT_MAX
#define USHRT_MAX __USHRT_MAX

#define WORD_BIT __WORD_BIT
#define INT_MIN __INT_MIN
#define INT_MAX __INT_MAX
#define UINT_MAX __UINT_MAX

#define LONG_BIT __LONG_BIT
#define LONG_MIN __LONG_MIN
#define LONG_MAX __LONG_MAX
#define ULONG_MAX __ULONG_MAX

#define LONG_MIN __LONG_MIN
#define LLONG_MAX __LLONG_MAX
#define ULLONG_MAX __ULLONG_MAX

#define SSIZE_MIN __SSIZE_MIN
#define SSIZE_MAX __SSIZE_MAX
#define SIZE_MAX __SIZE_MAX
#define INTPTR_MIN __INTPTR_MIN
#define INTPTR_MAX __INTPTR_MAX
#define UINTPTR_MAX __UINTPTR_MAX

#endif
