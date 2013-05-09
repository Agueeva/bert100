/**
 *************************************************************
 * This file is part of the XY-0815 Webserver
 * (C) 2002 Jochen Karrer 
 *************************************************************
 */
#include "types.h" 
#include <stdlib.h>
#include "console.h"
#include "interpreter.h"
#include "base64.h"

static const uint8_t encoding_table[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static const uint8_t decoding_table[]={255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,62,255,255,255,63,52,53,54,55,56,57,58,59,60,61,255,255,255,64,255,255,255,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,255,255,255,255,255,255,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,255,255,255,255,255,};

/**
 **********************************************************************************
 * \fn uint16_t Base64_Decode(uint8_t *dst,const uint8_t *src,uint16_t buflen) 
 **********************************************************************************
 */
uint16_t
Base64_Decode(uint8_t *dst,const uint8_t *src,uint16_t buflen) 
{
	uint16_t i=0,count=0;
	uint8_t b;
	for(i=0;*src && (*src<128) && (count<buflen);i++,src++) {
		b=decoding_table[*src];
		if(b>64)  {
			return count;
		}
		if(b==64) {
			break;
		}
		switch(i&3) {
			case 0:
				dst[0]= b << 2;
				count++;
				break;
			case 1:
				dst[0] |= b >> 4;
				dst[1]=b << 4;
				count++;
				break;
				
			case 2:
				dst[1] |= b >> 2;
				dst[2] = b << 6;
				count++;
				break;
			case 3:
				dst[2] |= b;
				dst+=3;
				break;
		}
	}
	return count;
} 
/*
 ******************************************************************************
 * \fn uint16_t Base64_Encode(uint8_t *dst,const uint8_t *src,uint16_t srclen); 
 * dstlen has to be ((int)((srclen+2) / 3))*4;
 ******************************************************************************
 */
uint16_t
Base64_Encode(uint8_t *dst,const uint8_t *src,uint16_t srclen) {
	uint8_t inbytenr = 0;
	uint16_t outbytenr = 0;
	unsigned char previous = 0;
	uint16_t i;
	for(i=0;i<srclen;i++) {
		uint8_t b=src[i];
		switch(inbytenr) {
			case 0:
				dst[outbytenr++]=encoding_table[b>>2];
				previous=b;
				inbytenr++;
				break;
			case 1:
				dst[outbytenr++]=encoding_table[((previous<<4)&0x30) | (b>>4)]; 
				previous=b;
				inbytenr++;
				break;

			case 2:
				dst[outbytenr++]=encoding_table[((previous<<2)&0x3c) | (b>>6)];
				dst[outbytenr++]=encoding_table[b&0x3f];
				inbytenr=0;
				break;

		}
	}
	switch(inbytenr) {
		case 1:
			dst[outbytenr++]=encoding_table[(previous<<4)&0x30]; 
			dst[outbytenr++]=encoding_table[64]; 
			dst[outbytenr++]=encoding_table[64]; 
			return outbytenr;
		case 2:
			dst[outbytenr++]=encoding_table[(previous<<2)&0x3c]; 
			dst[outbytenr++]=encoding_table[64]; 
			return outbytenr;
		default:
			return outbytenr;

	}
}


#ifdef BASE64TEST
void 
mkdecodingtable() {
	int i,j;
	printf("static unsigned char decoding_table[]={");
	for(i=0;i<128;i++) {
		for(j=0;j<65;j++) {
			if(encoding_table[j]==i) {
				printf("%d,",j);
				break;
			}
		}
		if(j==65) {
			printf("255,");
		}
	}
	printf("};\n");
}
int
main() {
	char dst[50];
	char *enc;
	char *str;
	//mkdecodingtable();
	//printf("%d\n",strlen(encoding_table));
	str=dst;
	str+=XY_Base64Decode(str,"YWZmZTphZmZl",50);
	*str++=0;
	printf("decode %s\n",dst);

	str=dst;
	str+=XY_Base64Decode(dst,"ZXNlbDpodW5kZWJyZWk=",50);
	*str++=0;
	printf("decode %s\n",dst);

	str=dst;
	str+=XY_Base64Decode(dst,"eDp5eg==",50);
	*str++=0;
	printf("decode %s\n",dst);

	str=dst;
	enc="x:yz";
	str+=XY_Base64Encode(dst,enc,strlen(enc));
	*str++=0;
	printf("encode \"%s\"\n",dst);

	str=dst;
	enc="esel:hundebrei";
	str+=XY_Base64Encode(dst,enc,strlen(enc));
	*str++=0;
	printf("encode \"%s\"\n",dst);

	str=dst;
	enc="affe:affe";
	str+=XY_Base64Encode(dst,enc,strlen(enc));
	*str++=0;
	printf("encode \"%s\"\n",dst);

	str=dst;
	str+=XY_Base64Decode(dst,"OmFmZmU6a3Vo",50);
	*str++=0;
	printf("decode %s\n",dst);
	exit(0);	
}
#endif // BASE64TEST
