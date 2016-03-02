/*
 * Copyright (c) 2012, 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * inttypes.h
 * Fixed size integer types.
 */

#ifndef INCLUDE_INTTYPES_H
#define INCLUDE_INTTYPES_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <stdint.h>

#if __USE_SORTIX || defined(__STDC_FORMAT_MACROS) || !defined(__cplusplus)

#define PRId8 __PRId8
#define PRIi8 __PRIi8
#define PRIo8 __PRIo8
#define PRIu8 __PRIu8
#define PRIx8 __PRIx8
#define PRIX8 __PRIX8
#define SCNd8 __SCNd8
#define SCNi8 __SCNi8
#define SCNo8 __SCNo8
#define SCNu8 __SCNu8
#define SCNx8 __SCNx8

#define PRId16 __PRId16
#define PRIi16 __PRIi16
#define PRIo16 __PRIo16
#define PRIu16 __PRIu16
#define PRIx16 __PRIx16
#define PRIX16 __PRIX16
#define SCNd16 __SCNd16
#define SCNi16 __SCNi16
#define SCNo16 __SCNo16
#define SCNu16 __SCNu16
#define SCNx16 __SCNx16

#define PRId32 __PRId32
#define PRIi32 __PRIi32
#define PRIo32 __PRIo32
#define PRIu32 __PRIu32
#define PRIx32 __PRIx32
#define PRIX32 __PRIX32
#define SCNd32 __SCNd32
#define SCNi32 __SCNi32
#define SCNo32 __SCNo32
#define SCNu32 __SCNu32
#define SCNx32 __SCNx32

#define PRId64 __PRId64
#define PRIi64 __PRIi64
#define PRIo64 __PRIo64
#define PRIu64 __PRIu64
#define PRIx64 __PRIx64
#define PRIX64 __PRIX64
#define SCNd64 __SCNd64
#define SCNi64 __SCNi64
#define SCNo64 __SCNo64
#define SCNu64 __SCNu64
#define SCNx64 __SCNx64

#define PRIdFAST8 __PRIdFAST8
#define PRIiFAST8 __PRIiFAST8
#define PRIoFAST8 __PRIoFAST8
#define PRIuFAST8 __PRIuFAST8
#define PRIxFAST8 __PRIxFAST8
#define PRIXFAST8 __PRIXFAST8
#define SCNdFAST8 __SCNdFAST8
#define SCNiFAST8 __SCNiFAST8
#define SCNoFAST8 __SCNoFAST8
#define SCNuFAST8 __SCNuFAST8
#define SCNxFAST8 __SCNxFAST8

#define PRIdFAST16 __PRIdFAST16
#define PRIiFAST16 __PRIiFAST16
#define PRIoFAST16 __PRIoFAST16
#define PRIuFAST16 __PRIuFAST16
#define PRIxFAST16 __PRIxFAST16
#define PRIXFAST16 __PRIXFAST16
#define SCNdFAST16 __SCNdFAST16
#define SCNiFAST16 __SCNiFAST16
#define SCNoFAST16 __SCNoFAST16
#define SCNuFAST16 __SCNuFAST16
#define SCNxFAST16 __SCNxFAST16

#define PRIdFAST32 __PRIdFAST32
#define PRIiFAST32 __PRIiFAST32
#define PRIoFAST32 __PRIoFAST32
#define PRIuFAST32 __PRIuFAST32
#define PRIxFAST32 __PRIxFAST32
#define PRIXFAST32 __PRIXFAST32
#define SCNdFAST32 __SCNdFAST32
#define SCNiFAST32 __SCNiFAST32
#define SCNoFAST32 __SCNoFAST32
#define SCNuFAST32 __SCNuFAST32
#define SCNxFAST32 __SCNxFAST32

#define PRIdFAST64 __PRIdFAST64
#define PRIiFAST64 __PRIiFAST64
#define PRIoFAST64 __PRIoFAST64
#define PRIuFAST64 __PRIuFAST64
#define PRIxFAST64 __PRIxFAST64
#define PRIXFAST64 __PRIXFAST64
#define SCNdFAST64 __SCNdFAST64
#define SCNiFAST64 __SCNiFAST64
#define SCNoFAST64 __SCNoFAST64
#define SCNuFAST64 __SCNuFAST64
#define SCNxFAST64 __SCNxFAST64

#define PRIdLEAST8 __PRIdLEAST8
#define PRIiLEAST8 __PRIiLEAST8
#define PRIoLEAST8 __PRIoLEAST8
#define PRIuLEAST8 __PRIuLEAST8
#define PRIxLEAST8 __PRIxLEAST8
#define PRIXLEAST8 __PRIXLEAST8
#define SCNdLEAST8 __SCNdLEAST8
#define SCNiLEAST8 __SCNiLEAST8
#define SCNoLEAST8 __SCNoLEAST8
#define SCNuLEAST8 __SCNuLEAST8
#define SCNxLEAST8 __SCNxLEAST8

#define PRIdLEAST16 __PRIdLEAST16
#define PRIiLEAST16 __PRIiLEAST16
#define PRIoLEAST16 __PRIoLEAST16
#define PRIuLEAST16 __PRIuLEAST16
#define PRIxLEAST16 __PRIxLEAST16
#define PRIXLEAST16 __PRIXLEAST16
#define SCNdLEAST16 __SCNdLEAST16
#define SCNiLEAST16 __SCNiLEAST16
#define SCNoLEAST16 __SCNoLEAST16
#define SCNuLEAST16 __SCNuLEAST16
#define SCNxLEAST16 __SCNxLEAST16

#define PRIdLEAST32 __PRIdLEAST32
#define PRIiLEAST32 __PRIiLEAST32
#define PRIoLEAST32 __PRIoLEAST32
#define PRIuLEAST32 __PRIuLEAST32
#define PRIxLEAST32 __PRIxLEAST32
#define PRIXLEAST32 __PRIXLEAST32
#define SCNdLEAST32 __SCNdLEAST32
#define SCNiLEAST32 __SCNiLEAST32
#define SCNoLEAST32 __SCNoLEAST32
#define SCNuLEAST32 __SCNuLEAST32
#define SCNxLEAST32 __SCNxLEAST32

#define PRIdLEAST64 __PRIdLEAST64
#define PRIiLEAST64 __PRIiLEAST64
#define PRIoLEAST64 __PRIoLEAST64
#define PRIuLEAST64 __PRIuLEAST64
#define PRIxLEAST64 __PRIxLEAST64
#define PRIXLEAST64 __PRIXLEAST64
#define SCNdLEAST64 __SCNdLEAST64
#define SCNiLEAST64 __SCNiLEAST64
#define SCNoLEAST64 __SCNoLEAST64
#define SCNuLEAST64 __SCNuLEAST64
#define SCNxLEAST64 __SCNxLEAST64

#define PRIdPTR __PRIdPTR
#define PRIiPTR __PRIiPTR
#define PRIoPTR __PRIoPTR
#define PRIuPTR __PRIuPTR
#define PRIxPTR __PRIxPTR
#define PRIXPTR __PRIXPTR
#define SCNdPTR __SCNdPTR
#define SCNiPTR __SCNiPTR
#define SCNoPTR __SCNoPTR
#define SCNuPTR __SCNuPTR
#define SCNxPTR __SCNxPTR

#define PRIdMAX __PRIdMAX
#define PRIiMAX __PRIiMAX
#define PRIoMAX __PRIoMAX
#define PRIuMAX __PRIuMAX
#define PRIxMAX __PRIxMAX
#define PRIXMAX __PRIXMAX
#define SCNdMAX __SCNdMAX
#define SCNiMAX __SCNiMAX
#define SCNoMAX __SCNoMAX
#define SCNuMAX __SCNuMAX
#define SCNxMAX __SCNxMAX

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __wchar_t_defined
#define __wchar_t_defined
#define __need_wchar_t
#include <stddef.h>
#endif

typedef struct
{
	intmax_t quot;
	intmax_t rem;
} imaxdiv_t;

intmax_t imaxabs(intmax_t);
imaxdiv_t imaxdiv(intmax_t, intmax_t);
intmax_t strtoimax(const char* __restrict, char** __restrict, int);
uintmax_t strtoumax(const char* __restrict, char** __restrict, int);
intmax_t wcstoimax(const wchar_t* __restrict, wchar_t** __restrict, int);
uintmax_t wcstoumax(const wchar_t* __restrict, wchar_t** __restrict, int);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
