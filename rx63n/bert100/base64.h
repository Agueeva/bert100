#include "types.h"
uint16_t Base64_Decode(unsigned char *dst,const unsigned char *src,uint16_t buflen);
uint16_t Base64_Encode(unsigned char *dst,const unsigned char *src,uint16_t srclen); 
INLINE uint16_t Base64_Len(int srclen) {return ((srclen + 2) / 3) * 4;}
