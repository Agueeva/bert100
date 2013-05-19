/*	$Id: xy_sha1.c,v 1.11 2002/10/09 10:31:58 cip307 Exp $ */
/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * FIPS pub 180-1: Secure Hash Algorithm (SHA-1)
 * based on: http://csrc.nist.gov/fips/fip180-1.txt
 * implemented by Jun-ichiro itojun Itoh <itojun@itojun.org>
 */

#include <string.h>
#include "types.h" 
#include "byteorder.h"
#include "sha1.h"
#include "interpreter.h"
#include "console.h"


/* constant table */
static uint32_t _K[] = {0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6};

#define K(t)	_K[(t) / 20]

#define F0(b, c, d) (((b) & (c)) | ((~(b)) & (d)))
#define F1(b, c, d) (((b) ^ (c)) ^ (d))
#define F2(b, c, d) (((b) & (c)) | ((b) & (d)) | ((c) & (d)))
#define F3(b, c, d) (((b) ^ (c)) ^ (d))

#define S(n, x)		(((x) << (n)) | ((x) >> (32 - n)))

#define H(n)	(ctxt->h.b32[(n)])
#define COUNT	(ctxt->count)
#define W(n)	(ctxt->m.b32[(n)])

#define PUTBYTE(x) \
do { \
	ctxt->m.b8[(COUNT % 64)] = (x);		\
	COUNT++;				\
	COUNT %= 64;				\
	ctxt->c.b64[0] += 8;			\
	if (COUNT % 64 == 0)			\
		sha1_step(ctxt);		\
} while (0)

#define PUTPAD(x) \
do { \
	ctxt->m.b8[(COUNT % 64)] = (x);		\
	COUNT++;				\
	COUNT %= 64;				\
	if (COUNT % 64 == 0)			\
		sha1_step(ctxt);		\
} while (0)

static void sha1_step(struct sha1_ctxt *);

static void
sha1_step(struct sha1_ctxt * ctxt)
{
	uint32_t	a, b, c, d, e;
	size_t		t, s;
	uint32_t	tmp;

#ifndef WORDS_BIGENDIAN
	int i;
	for(i=0;i<16;i++) {
		ctxt->m.b32[i]= htonl(ctxt->m.b32[i]);
	}
#endif

	a = H(0);
	b = H(1);
	c = H(2);
	d = H(3);
	e = H(4);

	for (t = 0; t < 20; t++)
	{
		s = t & 0x0f;
		if (t >= 16)
			W(s) = S(1, W((s + 13) & 0x0f) ^ W((s + 8) & 0x0f) ^ W((s + 2) & 0x0f) ^ W(s));
		tmp = S(5, a) + F0(b, c, d) + e + W(s) + K(t);
		e = d;
		d = c;
		c = S(30, b);
		b = a;
		a = tmp;
	}
	for (t = 20; t < 40; t++)
	{
		s = t & 0x0f;
		W(s) = S(1, W((s + 13) & 0x0f) ^ W((s + 8) & 0x0f) ^ W((s + 2) & 0x0f) ^ W(s));
		tmp = S(5, a) + F1(b, c, d) + e + W(s) + K(t);
		e = d;
		d = c;
		c = S(30, b);
		b = a;
		a = tmp;
	}
	for (t = 40; t < 60; t++)
	{
		s = t & 0x0f;
		W(s) = S(1, W((s + 13) & 0x0f) ^ W((s + 8) & 0x0f) ^ W((s + 2) & 0x0f) ^ W(s));
		tmp = S(5, a) + F2(b, c, d) + e + W(s) + K(t);
		e = d;
		d = c;
		c = S(30, b);
		b = a;
		a = tmp;
	}
	for (t = 60; t < 80; t++)
	{
		s = t & 0x0f;
		W(s) = S(1, W((s + 13) & 0x0f) ^ W((s + 8) & 0x0f) ^ W((s + 2) & 0x0f) ^ W(s));
		tmp = S(5, a) + F3(b, c, d) + e + W(s) + K(t);
		e = d;
		d = c;
		c = S(30, b);
		b = a;
		a = tmp;
	}

	H(0) = H(0) + a;
	H(1) = H(1) + b;
	H(2) = H(2) + c;
	H(3) = H(3) + d;
	H(4) = H(4) + e;
}

/*------------------------------------------------------------*/

void
sha1_init(struct sha1_ctxt * ctxt)
{
	memset(ctxt,0, sizeof(struct sha1_ctxt));
	H(0) = 0x67452301;
	H(1) = 0xefcdab89;
	H(2) = 0x98badcfe;
	H(3) = 0x10325476;
	H(4) = 0xc3d2e1f0;
}

static void
sha1_pad(struct sha1_ctxt * ctxt)
{
	size_t		padlen;			/* pad length in bytes */
	size_t		padstart;

	PUTPAD(0x80);

	padstart = COUNT % 64;
	padlen = 64 - padstart;
	if (padlen < 8)
	{
		memset(&ctxt->m.b8[padstart],0, padlen);
		COUNT += padlen;
		COUNT %= 64;
		sha1_step(ctxt);
		padstart = COUNT % 64;	/* should be 0 */
		padlen = 64 - padstart; /* should be 64 */
	}
	memset(&ctxt->m.b8[padstart],0, padlen - 8);
	COUNT += (padlen - 8);
	COUNT %= 64;
#ifdef WORDS_BIGENDIAN
	PUTPAD(ctxt->c.b8[0]);
	PUTPAD(ctxt->c.b8[1]);
	PUTPAD(ctxt->c.b8[2]);
	PUTPAD(ctxt->c.b8[3]);
	PUTPAD(ctxt->c.b8[4]);
	PUTPAD(ctxt->c.b8[5]);
	PUTPAD(ctxt->c.b8[6]);
	PUTPAD(ctxt->c.b8[7]);
#else
	PUTPAD(ctxt->c.b8[7]);
	PUTPAD(ctxt->c.b8[6]);
	PUTPAD(ctxt->c.b8[5]);
	PUTPAD(ctxt->c.b8[4]);
	PUTPAD(ctxt->c.b8[3]);
	PUTPAD(ctxt->c.b8[2]);
	PUTPAD(ctxt->c.b8[1]);
	PUTPAD(ctxt->c.b8[0]);
#endif
}

void
sha1_loop(struct sha1_ctxt * ctxt, const uint8_t *input0, size_t len)
{
	const uint8_t *input;
	size_t		gaplen;
	size_t		gapstart;
	size_t		off;
	size_t		copysiz;

	input = (const uint8_t *) input0;
	off = 0;

	while (off < len)
	{
		gapstart = COUNT % 64;
		gaplen = 64 - gapstart;

		copysiz = (gaplen < len - off) ? gaplen : len - off;
		memcpy(&ctxt->m.b8[gapstart],&input[off], copysiz);
		COUNT += copysiz;
		COUNT %= 64;
		ctxt->c.b64[0] += copysiz<<3;
		if (COUNT % 64 == 0)
			sha1_step(ctxt);
		off += copysiz;
	}
}

static void
sha1_fastloop(struct sha1_ctxt * ctxt, const uint8_t *input0, size_t len)
{
	const uint8_t *input;
	size_t		off;

	input = (const uint8_t *) input0;

	for (off=0;off < len;off+=64) {
		memcpy(&ctxt->m.b8[0],&input0[off], 64);
		sha1_step(ctxt);
	}
	ctxt->c.b64[0] += len*8;
}
void
sha1_result(struct sha1_ctxt * ctxt, uint8_t *digest0)
{
	uint8_t	   *digest;

	digest = (uint8_t *) digest0;
	sha1_pad(ctxt);
#ifdef WORDS_BIGENDIAN 
	memcpy(digest,&ctxt->h.b8[0], 20);
#else
	{
		int i;
		for(i=0;i<5;i++) {
			((uint32_t *)digest)[i]=htonl(ctxt->h.b32[i]);
		}
	}
#endif
}

/*
 * calculate sha1sum of buffer
 */
void
sha1_sum(uint8_t *buf,size_t count,uint8_t * result) {
	struct sha1_ctxt ctxt;
	sha1_init(&ctxt);
	sha1_fastloop(&ctxt,buf,count&~63L);
	sha1_loop(&ctxt,buf+(count&~63L),count%64);
	sha1_result(&ctxt,result);
}

static int8_t
cmd_sha1(Interp * interp, uint8_t argc, char *argv[])
{
        uint8_t digest[20];
        uint8_t i;
        if(argc > 1) {
                sha1_sum((uint8_t*)argv[1],strlen(argv[1]),digest);
                for(i = 0; i < 20; i++) {
                        Con_Printf("%02x",digest[i]);
                }
                Con_Printf("\n");
        }
        return 0;
}
INTERP_CMD(sha1Cmd,"sha1", cmd_sha1, "sha1 string # Calculate sha1 sum of string");
/**
 *****************************************************************
 * Initialize the MD5 library. Register test commands.
 *****************************************************************
 */
void
Sha1Lib_Init()
{
        Interp_RegisterCmd(&sha1Cmd);
}
