/***********************************************************************/
/*                                                                     */
/*  FILE        :types.h                                               */
/*  DATE        :Mon, Mar 12, 2012                                     */
/*  DESCRIPTION :Aliases of Integer Type                               */
/*  CPU TYPE    :RX621                                                 */
/*                                                                     */
/*  This file is generated by KPIT GNU Project Generator.              */
/*                                                                     */
/***********************************************************************/

#ifndef _TYPES_H
#define _TYPES_H
#define array_size(x) (sizeof(x) / sizeof((x)[0]))

#include <stddef.h>
/**
 **********************************************************************************
 * container_of - cast a member of a structure out to the containing structure
 * stolen from the linux kernel
 * offsetof is in <stddef.h>
 **********************************************************************************
 */
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

typedef signed char _SBYTE;
typedef unsigned char _UBYTE;
typedef signed short _SWORD;
typedef unsigned short _UWORD;
typedef signed int _SINT;
typedef unsigned int _UINT;
typedef signed long _SDWORD;
typedef unsigned long _UDWORD;
typedef signed long long _SQWORD;
typedef unsigned long long _UQWORD;

/* Some types from the C99 standard */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned uint32_t;
typedef _SQWORD int64_t;
typedef _UQWORD uint64_t;
typedef unsigned char bool;
typedef unsigned int uint;
#define false 0
#define true  1

/* ANSI C99 constant declarations */
#if 0
#define UINT8_C(x)     ((uint8_t)(x))
#define UINT16_C(x)    ((uint16_t)(x))
#else
#define UINT8_C(x)     (x)
#define UINT16_C(x)    (x)
#endif
#define UINT32_C(x)    (x##U)
#define UINT64_C(x)    (x##ULL)
#define INT32_C(x)      (x)
#define INT64_C(x)      (x##LL)

typedef uint16_t uint16_le;
typedef uint16_t uint16_be;
typedef uint32_t uint32_le;
typedef uint32_t uint32_be;
#define INLINE static inline
#endif
