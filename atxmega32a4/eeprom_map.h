#include <stddef.h>
#include <stdint.h>

typedef struct EEProm_Layout {
	uint16_t pwm_resolution;
	uint16_t imtx_max[4];
	uint16_t ibtx_max[4];
	uint16_t imtx_init[4];
	uint16_t ibtx_init[4];
//	uint16_t lpbias_init;
	uint8_t lddis_init;
} __attribute__((packed)) EEProm_Layout;

#define EEADDR(x) (offsetof(EEProm_Layout,x))
#define EEPROM_HXR_ADDR		(0x280)
#define EEPROM_HXR_MSK_ADDR	(0x320)
