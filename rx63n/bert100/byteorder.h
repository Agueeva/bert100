/**
 *************************************************
 * definitions for little endian 
 *************************************************
 */
#include "types.h"

#define host16_to_le(x) ((uint16_t)(x))
#define host32_to_le(x) ((uint32_t)(x))
#define le16_to_host(x) ((uint16_t)(x))
#define le32_to_host(x) ((uint32_t)(x))
/**
 **********************************************************
 * \fn uint16_t swap32(uint16_t value) {
 * 32 Bit Byte order conversion inline assembler function 
 **********************************************************
 */
static inline uint32_t
swap32(uint32_t value)
{
	uint32_t retval;
	__asm__("revl %1, %0;":"=r"(retval)
	: "r"(value)
	: );
	return retval;
}

/**
 ************************************************************
 * \fn uint16_t swap16(uint16_t value) {
 * 16 Bit Byte order conversion inline assembler function 
 * The Rx instruction works on 32 Bit words and swaps inside
 * both 16bit words
 ************************************************************
 */
static inline uint16_t
swap16(uint32_t value)
{
	uint32_t retval;
	__asm__("revw %1, %0;":"=r"(retval)
	: "r"(value)
	: );
	return retval;
}

static inline uint64_t
swap64(uint64_t value)
{
	uint32_t new_low = swap32(value >> 32);
	uint32_t new_high = swap32(value);
	return new_low | ((uint64_t) new_high << 32);
}

#define host16_to_be(x) swap16(x)
#define host32_to_be(x) swap32(x)
#define host64_to_be(x) swap64(x)
#define be16_to_host(x) swap16(x)
#define be32_to_host(x) swap32(x)
#define be64_to_host(x) swap64(x)
#define htons(x) swap16(x)
#define ntohs(x) swap16(x)
#define htonl(x) swap32(x)
#define ntohl(x) swap32(x)
