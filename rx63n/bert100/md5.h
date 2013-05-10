/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.
These notices must be retained in any copies of any part of this
documentation and/or software.
 */


#include "types.h" 

typedef struct
{
        uint32_t state[4];         /* state (ABCD) */
        uint32_t count[2];         /* number of bits, modulo 2^64 (lsb first) */
        uint8_t buffer[64];       /* input buffer */
} xy_md5ctx;

void MD5Init(xy_md5ctx *);
void MD5Loop(xy_md5ctx *, unsigned char *buf, unsigned int len);
void MD5Result(unsigned char[16], xy_md5ctx *);
/*
 * MD5 Encode an ascii string
 * digest must point to an array with a length of 16 Bytes
 */
void MD5_EncodeString(uint8_t *digest,const char *string);

void MD5Lib_Init(void);
