/*
 *************************************************************
    *//**
 * \file i2cmaster.c
 * Soft-I2C master implementation for AVR8
 *************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include "i2cmaster.h"
#include "events.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"
#include "iodefine.h"
#include "atomic.h"
#include "timer.h"

#define BUS_NR(i2caddr) (((i2caddr) >> 12) & 0x3)
#define MAX_ACK_POLLS	(2)

#define PORT_SDA(val)	BMOD(3,PORT1.DR.BYTE,(val))
#define PORT_SCL(val)	BMOD(2,PORT1.DR.BYTE,(val))
#define STATUS_SDA	PORT1.PORT.BIT.B3
#define STATUS_SCL	PORT1.PORT.BIT.B2
#define DIR_SDA(val)	BMOD(3,PORT1.DDR.BYTE,(val))
#define DIR_SCL(val)	BMOD(2,PORT1.DDR.BYTE,(val))

#ifdef I2C_SPEED_FAST
#define T_HDSTA 600		// 0.6 uS
#define T_SUSTA 600		// 0.6 uS
#define T_SUSTO 600		// 0.6 uS
#define T_HIGH  600		// 0.6 uS
#define T_LOW   1300		// 1.3 uS
#define T_BUF   1300		// 1.3 uS
#define T_SUDAT 100		// 0.1 uS
#else
#define T_HDSTA  4000		// 4.0 uS
#define T_SUSTA  4700		// 4.7 uS
#define T_SUSTO  4000		// 4.0 uS
#define T_HIGH   4000		// 4.0 uS
#define T_BUF    4700		// 4.7 uS
#define T_LOW    4700		// 4.7 uS
#define T_SUDAT  250		// 0.25 uS
#endif

typedef struct I2C_Master {
	void    (*setSDA)(uint8_t value);
	uint8_t (*getSDA)(void);
	void    (*setSCL)(uint8_t value);
	uint8_t (*getSCL)(void);

	uint16_t stat_fail_location;
	uint16_t stat_scl_blkd;
	uint16_t stat_arb_loss;
	uint16_t stat_no_ack;
	uint16_t stat_bus_reset;
} I2C_Master;

static I2C_Master gI2cm[2];

static uint8_t i2c_delay = 8;
static uint8_t i2c_fast_delay = 8;
static uint8_t i2c_lsdelay = 40;

INLINE void
SDA_High(I2C_Master *i2cm)
{
	i2cm->setSDA(1);
}

INLINE void
SDA_Low(I2C_Master *i2cm)
{
	i2cm->setSDA(0);
}

INLINE void
SCL_High(I2C_Master *i2cm)
{
	i2cm->setSCL(1);
}

INLINE void
SCL_Low(I2C_Master *i2cm)
{
	i2cm->setSCL(0);
}

INLINE uint8_t
SDA_Read(I2C_Master *i2cm)
{
	return i2cm->getSDA();
}

INLINE uint8_t
SCL_Read(I2C_Master *i2cm)
{
	return i2cm->getSCL();
}


INLINE uint8_t
i2c_check_arb(I2C_Master *i2cm)
{
	if (!SDA_Read(i2cm)) {
		i2cm->stat_arb_loss++;
		i2cm->stat_fail_location = __LINE__;
		return I2C_RESULT_ARB_LOSS;
	} else {
		return 0;
	}
}

INLINE void
i2c_ndelay(uint16_t nanos)
{
	volatile int i;
	for (i = 0; i < i2c_delay; i++) {
		/* Do nothing */
	}
}

/**
 ***********************************************************************
 * i2c_sync
 * Wait for maximal 510us for release of the SCL Line
 * If a slave streches SCL for a longer period it is a bad slave 
 ***********************************************************************
 */
static uint8_t
i2c_sync(I2C_Master *i2cm)
{
	uint8_t count;
	for (count = 0; count < 200; count++) {
		if (SCL_Read(i2cm)) {
			return 0;
		}
		i2c_ndelay(1000);
	}
	i2cm->stat_scl_blkd++;
	i2cm->stat_fail_location = __LINE__;
	return I2C_RESULT_SCL_BLKD;
}

/**
 ************************************************************
 * i2c_start()
 * Generate a Start condition
 * Start condition assumes that scl and sda are high
 ************************************************************
 */
static uint8_t
i2c_start(I2C_Master *i2cm)
{
	uint8_t ret;
	ret = i2c_sync(i2cm);
	if (ret != I2C_RESULT_OK) {
		return ret;
	}
	if ((ret = i2c_check_arb(i2cm)) != 0) {
		return ret;
	}
	SDA_Low(i2cm);
	i2c_ndelay(T_HDSTA);
	SCL_Low(i2cm);
	return 0;
}

/*
 ***************************************************************
 * i2c_repstart()
 * Generate a repeated start condition
 ***************************************************************
 */

static uint8_t
i2c_repstart(I2C_Master *i2cm)
{
	uint8_t ret;
	SDA_High(i2cm);
	i2c_ndelay(T_SUDAT);
	SCL_High(i2cm);
	i2c_ndelay(T_SUSTA);
	ret = i2c_sync(i2cm);
	if (ret != I2C_RESULT_OK) {
		return ret;
	}
	if ((ret = i2c_check_arb(i2cm)) != 0) {
		return ret;
	}
	SDA_Low(i2cm);
	i2c_ndelay(T_HDSTA);
	SCL_Low(i2cm);
	i2c_ndelay(T_LOW);
	return 0;
}

/**
 ******************************************************************
 * \fn static void i2c_stop(void);
 * Generate a stop condition.
 ******************************************************************
 */
static void
i2c_stop(I2C_Master *i2cm)
{
	SDA_Low(i2cm);
	i2c_ndelay(T_LOW);
	SCL_High(i2cm);
	i2c_sync(i2cm);
	i2c_ndelay(T_SUSTO);
	SDA_High(i2cm);
	i2c_ndelay(T_BUF);
	return;
}

/*
 *****************************************************************
 * \fn static uint8_t i2c_readb(int ack, uint8_t * b);
 * Read a Byte as master
 *****************************************************************
 */
static uint8_t
i2c_readb(I2C_Master *i2cm, int ack, uint8_t * b)
{
	int8_t i;
	uint8_t ret;
	uint8_t data = 0;
	SDA_High(i2cm);
	for (i = 7; i >= 0; i--) {
		i2c_ndelay(T_LOW);
		SCL_High(i2cm);
		ret = i2c_sync(i2cm);
		if (ret != I2C_RESULT_OK) {
			return ret;
		}
		i2c_ndelay(T_HIGH);
		if (SDA_Read(i2cm)) {
			data |= (1 << i);
		}
		SCL_Low(i2cm);
	}
	if (ack) {
		SDA_Low(i2cm);
	}
	i2c_ndelay(T_LOW);
	SCL_High(i2cm);
	i2c_sync(i2cm);
	i2c_ndelay(T_HIGH);
	*b = data;
	if (!ack) {
		ret = i2c_check_arb(i2cm);
		if (ret) {
			return ret;
		}
	}
	SCL_Low(i2cm);
	return 0;
}

/*
 *****************************************************************
 * \fn static uint8_t i2c_writeb(uint8_t value)
 * Write a Byte as master.
 * \param value The byte which will be written to the I2C bus.
 *****************************************************************
 */
static uint8_t
i2c_writeb(I2C_Master *i2cm, uint8_t value)
{
	int8_t i;
	uint8_t ret;
	uint8_t ack;
	for (i = 7; i >= 0; i--) {
		int8_t bit = (value >> i) & 1;
		if (bit) {
			SDA_High(i2cm);
		} else {
			SDA_Low(i2cm);
		}
		i2c_ndelay(T_LOW);
		SCL_High(i2cm);
		ret = i2c_sync(i2cm);
		if (ret != I2C_RESULT_OK) {
			return ret;
		}
		i2c_ndelay(T_HIGH);
		if (bit) {
			ret = i2c_check_arb(i2cm);
			if (ret != I2C_RESULT_OK) {
				return ret;
			}
		}
		SCL_Low(i2cm);
	}
	//i2c_ndelay(T_HDDAT); 
	SDA_High(i2cm);
	i2c_ndelay(T_LOW);
	SCL_High(i2cm);
	i2c_sync(i2cm);
	i2c_ndelay(T_HIGH);
	ack = !SDA_Read(i2cm);
	SCL_Low(i2cm);
	if (ack) {
		return 0;
	} else {
		i2cm->stat_no_ack++;
		i2cm->stat_fail_location = __LINE__;
		return I2C_RESULT_NO_ACK;
	}
}

/**
 *******************************************************
 * \fn static void i2c_reset(void)
 * Reset the I2C bus by sending 10 Clock cycles 
 * followed by a stop condition.
 *******************************************************
 */
void
I2C_ResetBus(uint8_t bus)
{
	uint8_t i;
	I2C_Master *i2cm;
	if((bus >= array_size(gI2cm))) {
		return;
	}
	i2cm = &gI2cm[bus];
	i2cm->stat_bus_reset++;
	SCL_Low(i2cm);
	SDA_High(i2cm);
	i2c_ndelay(5000);
	for (i = 0; i < 10; i++) {
		SCL_High(i2cm);
		i2c_sync(i2cm);
		i2c_ndelay(5000);
		SCL_Low(i2cm);
		i2c_ndelay(5000);
	}
	i2c_stop(i2cm);
}

INLINE uint8_t
i2c_get_delay(uint16_t i2c_addr)
{
	if (i2c_addr & I2C_LOW_SPEED_DEV) {
		return i2c_lsdelay;
	} else {
		return i2c_fast_delay;
	}
}

INLINE void
i2c_select_delay(uint16_t i2c_addr)
{
	i2c_delay = i2c_get_delay(i2c_addr);
}

/**
 **************************************************************
 * \fn static uint8_t I2C_AckPoll(uint8_t i2caddr)
 * Send and I2C address repeatedly until a acknowledge is 
 * received.  
 * \param i2caddr The I2C address of the device which will be polled.
 **************************************************************
 */
static uint8_t
I2C_AckPoll(uint16_t i2caddr, uint8_t retries)
{
	I2C_Master *i2cm;
	uint8_t i, result;
	int8_t reset = 0;
	uint8_t bus = BUS_NR(i2caddr);
	if(bus >= array_size(gI2cm)) {
		return I2C_RESULT_NO_ACK;
	}
	i2cm = &gI2cm[bus];
	for (i = 0; i < retries; i++) {
		result = i2c_start(i2cm);
		if (result != I2C_RESULT_OK) {
			/* Reset only once */
			if (reset == 0) {
				reset++;
				I2C_ResetBus(bus);
				continue;
			} else {
				return result;
			}
		}
		result = i2c_writeb(i2cm, i2caddr);
		if (result == I2C_RESULT_OK) {
			return result;
		} else if (result != I2C_RESULT_NO_ACK) {
			return result;
		}
		i2c_stop(i2cm);
	}
	return I2C_RESULT_NO_ACK;
}

/*
 ******************************************************
 * Find a device on the multiplexer
 ******************************************************
 */
static void
I2C_FindMuxChanSelectSpeed(uint16_t i2caddr)
{
	i2c_select_delay(i2caddr);
}

/** 
 ************************************************************************
 * \fn uint8_t I2C_Read8(uint8_t i2caddr, uint8_t daddr, uint8_t * data, uint8_t count)
 * Read an data block from an I2C-Device. 
 * \param i2caddr The I2C Address of the device.
 * \param daddr The 8 Bit address of the first byte read from the I2C device.
 * \param *data Pointer to the destination for the data block.
 * \param count number of bytes to read. 
 ************************************************************************
 */

uint8_t
I2C_Read8(uint16_t i2caddr, uint8_t daddr, uint8_t * data, uint8_t count)
{
	I2C_Master *i2cm;
	uint8_t result;
	uint8_t i;
	uint8_t bus = BUS_NR(i2caddr);

	if((bus >= array_size(gI2cm))) {
		return I2C_RESULT_NO_ACK;
	}
	i2cm = &gI2cm[bus];

	I2C_FindMuxChanSelectSpeed(i2caddr);
	result = I2C_AckPoll(i2caddr, MAX_ACK_POLLS);
	if (result != I2C_RESULT_OK) {
		/* 
		 * Do not go to out, there was already a stop 
		 *  condition
		 */
		return result;
	}
	result = i2c_writeb(i2cm, daddr);
	if (result != I2C_RESULT_OK) {
		goto out;
	}
#if 1
	result = i2c_repstart(i2cm);
#else
	i2c_stop(i2cm);
	result = i2c_start(i2cm);
#endif
	if (result != I2C_RESULT_OK) {
		return result;
	}
	result = i2c_writeb(i2cm, i2caddr | 1);	/* DIR is read */
	if (result != I2C_RESULT_OK) {
		goto out;
	}
	for (i = 0; i < count; i++) {
		if ((i + 1) == count) {
			result = i2c_readb(i2cm, 0, data + i);
		} else {
			result = i2c_readb(i2cm, 1, data + i);
		}
		if (result != 0) {
			return result;
		}
	}
 out:
	if (result != I2C_RESULT_ARB_LOSS) {
		i2c_stop(i2cm);
	}
	return result;
}

/**
 ***********************************************************************************************
 * \fn uint8_t I2C_Read16(uint16_t i2caddr,uint16_t daddr,uint8_t *data,uint16_t count)
 * Read from a device with internal 16 Bit address.
 ***********************************************************************************************
 */
uint8_t
I2C_Read16(uint16_t i2caddr, uint16_t daddr, uint8_t * data, uint16_t count)
{
	I2C_Master *i2cm;
	uint8_t result;
	uint16_t i;
	uint8_t bus = BUS_NR(i2caddr);
	if(bus >= array_size(gI2cm)) {
		return I2C_RESULT_NO_ACK;
	}
	i2cm = &gI2cm[bus];
	I2C_FindMuxChanSelectSpeed(i2caddr);
	//i2c_select_speed(i2caddr);
	result = I2C_AckPoll(i2caddr, MAX_ACK_POLLS);
	if (result != I2C_RESULT_OK) {
		/* 
		 * Do not go to out, there was already a stop 
		 *  condition
		 */
		return result;
	}
	result = i2c_writeb(i2cm, daddr >> 8);
	if (result != I2C_RESULT_OK) {
		goto out;
	}
	result = i2c_writeb(i2cm, daddr);
	if (result != I2C_RESULT_OK) {
		goto out;
	}
	i2c_repstart(i2cm);

	result = i2c_writeb(i2cm, i2caddr | 1);	/* DIR is read */
	if (result != I2C_RESULT_OK) {
		goto out;
	}
	for (i = 0; i < count; i++) {
		if ((i + 1) == count) {
			result = i2c_readb(i2cm, 0, data + i);
		} else {
			result = i2c_readb(i2cm, 1, data + i);
		}
		if (result != I2C_RESULT_OK) {
			return result;
		}
	}
 out:
	if (result != I2C_RESULT_ARB_LOSS) {
		i2c_stop(i2cm);
	}
	return result;
}

/**
 ********************************************************************
 * Read 8 or 16 Bit, decide format from device address.
 ********************************************************************
 */
uint8_t
I2C_Read(uint16_t i2caddr, uint16_t daddr, uint8_t * data, uint8_t count)
{
	if (i2caddr & I2C_16BIT_ADDR_DEV) {
		return I2C_Read16(i2caddr, daddr, data, count);
	} else {
		return I2C_Read8(i2caddr, daddr, data, count);
	}
}

/** 
 *****************************************************************
 * \fn uint8_t I2C_Write8(uint8_t i2caddr, uint8_t daddr, const uint8_t * data, uint8_t count);
 * Write a data block to an I2C-Device. 
 * \param i2caddr The I2C Address of the device.
 * \param daddr The 8 Bit address of the first written byte in the I2C device.
 * \param *data Pointer to the data block to write.
 * \param count number of bytes to write. 
 *****************************************************************
 */
uint8_t
I2C_Write8(uint16_t i2caddr, uint8_t daddr, const uint8_t * data, uint8_t count)
{
	I2C_Master *i2cm;
	uint8_t result;
	uint8_t i;
	uint8_t bus = BUS_NR(i2caddr);
	if(bus >= array_size(gI2cm)) {
		return I2C_RESULT_NO_ACK;
	}
	i2cm = &gI2cm[bus];
	I2C_FindMuxChanSelectSpeed(i2caddr);
	result = I2C_AckPoll(i2caddr, MAX_ACK_POLLS);
	if (result != I2C_RESULT_OK) {
		/* 
		 * Do not go to out, there was already a stop 
		 * condition
		 */
		return result;
	}
	result = i2c_writeb(i2cm, daddr);
	if (result != I2C_RESULT_OK) {
		goto out;
	}
	for (i = 0; i < count; i++) {
		result = i2c_writeb(i2cm, data[i]);
		if (result != I2C_RESULT_OK) {
			goto out;
		}
	}
 out:
	if (result != I2C_RESULT_ARB_LOSS) {
		i2c_stop(i2cm);
	}
	return result;
}

/**
 **********************************************
 * I2C-Write version with 16 bit address
 **********************************************
 */

uint8_t
I2C_Write16(uint16_t i2caddr, uint16_t daddr, const uint8_t * data, uint16_t count)
{
	I2C_Master *i2cm;
	uint8_t result;
	uint16_t i;
	uint8_t bus = BUS_NR(i2caddr);
	if(bus >= array_size(gI2cm)) {
		return I2C_RESULT_NO_ACK;
	}
	i2cm = &gI2cm[bus];
	I2C_FindMuxChanSelectSpeed(i2caddr);
	//i2c_select_speed(i2caddr);
	result = I2C_AckPoll(i2caddr, MAX_ACK_POLLS);
	if (result != I2C_RESULT_OK) {
		/* 
		 * Do not go to out, there was already a stop 
		 *  condition
		 */
		return result;
	}
	result = i2c_writeb(i2cm, daddr >> 8);
	if (result != I2C_RESULT_OK) {
		goto out;
	}
	result = i2c_writeb(i2cm, daddr);
	if (result != I2C_RESULT_OK) {
		goto out;
	}
	for (i = 0; i < count; i++) {
		result = i2c_writeb(i2cm, data[i]);
		if (result != I2C_RESULT_OK) {
			goto out;
		}
	}
 out:
	if (result != I2C_RESULT_ARB_LOSS) {
		i2c_stop(i2cm);
	}
	return result;
}

/**
 * Do a write with 8 or 16 bit address depending on address
 */
uint8_t
I2C_Write(uint16_t i2caddr, uint16_t daddr, const uint8_t * data, uint8_t count)
{
	if (i2caddr & I2C_16BIT_ADDR_DEV) {
		return I2C_Write16(i2caddr, daddr, data, count);
	} else {
		return I2C_Write8(i2caddr, daddr, data, count);
	}
}

/**
 **********************************************************
 * \fn static void I2C_RiseTimes(void);
 * Meassure the I2C rise times and print them
 * on the serial console.
 **********************************************************
 */
static void
I2C_RiseTimes(uint8_t bus)
{
	I2C_Master *i2cm;
	uint8_t sda_count = 0;
	uint8_t scl_count = 0;
	uint8_t i;
	if(bus >= array_size(gI2cm)) {
		return;
	}
	i2cm = &gI2cm[bus];
	SDA_Low(i2cm);
	i2c_ndelay(5000);
	SDA_High(i2cm);
	if (SDA_Read(i2cm) == 0) {
		sda_count++;
	}
	if (SDA_Read(i2cm) == 0) {
		sda_count++;
	}
	if (SDA_Read(i2cm) == 0) {
		sda_count++;
	}
	if (SDA_Read(i2cm) == 0) {
		sda_count++;
	}
	if (SDA_Read(i2cm) == 0) {
		sda_count++;
	}
	if (SDA_Read(i2cm) == 0) {
		sda_count++;
	}
	if (SDA_Read(i2cm) == 0) {
		sda_count++;
	}
	for (i = 0; i < 100; i++) {
		if (SDA_Read(i2cm) == 0) {
			sda_count++;
		}
	}
	SCL_Low(i2cm);
	i2c_ndelay(5000);
	SCL_High(i2cm);
	if (SCL_Read(i2cm) == 0) {
		scl_count++;
	}
	if (SCL_Read(i2cm) == 0) {
		scl_count++;
	}
	if (SCL_Read(i2cm) == 0) {
		scl_count++;
	}
	if (SCL_Read(i2cm) == 0) {
		scl_count++;
	}
	if (SCL_Read(i2cm) == 0) {
		scl_count++;
	}
	if (SCL_Read(i2cm) == 0) {
		scl_count++;
	}
	if (SCL_Read(i2cm) == 0) {
		scl_count++;
	}
	for (i = 0; i < 100; i++) {
		if (SCL_Read(i2cm) == 0) {
			scl_count++;
		}
	}
	if ((sda_count > 1) || (scl_count > 1)) {
		Con_Printf("I2C-Bus %d Rise Time: SDA %d, SCL %d: ", bus, sda_count, scl_count);
	}
	if ((sda_count < 2) && (scl_count < 2)) {
		/* Con_Printf("Good\n"); */
	} else if ((sda_count < 3) && (scl_count < 3)) {
		Con_Printf("Critical\n");
	} else if ((sda_count < 4) && (scl_count < 4)) {
		Con_Printf("Bad\n");
	} else {
		Con_Printf("Very Bad\n");
	}
}

/**
 ***************************************************************************
 * \fn static int8_t cmd_i2cr(Interp * interp, uint8_t argc, char *argv[])
 * Shell command for reading from the I2C-Bus.
 ***************************************************************************
 */
static int8_t
cmd_i2cr(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t result;
	uint16_t i2c_addr;
	uint16_t mem_addr;
	uint16_t count = 1;
	uint8_t buf[8];
	uint8_t i;
	uint8_t print_cnt = 0;
	if (argc < 3) {
		return -EC_BADNUMARGS;
	}
	i2c_addr = astrtoi16(argv[1]);
	mem_addr = astrtoi16(argv[2]);
	if (argc > 3) {
		count = astrtoi16(argv[3]);
		if (count == 0) {
			count = 0x100;
		}
	}
	while (count) {
		uint8_t cnt = count > sizeof(buf) ? sizeof(buf) : count;
		count -= cnt;
		result = I2C_Read(i2c_addr, mem_addr, buf, cnt);
		if (result != I2C_RESULT_OK) {
			Interp_Printf_P(interp, "%02x I2C-Read failed\n", result);
			return -EC_ERROR;
		}
		for (i = 0; i < cnt; i++) {
			Interp_Printf_P(interp, "%02x ", buf[i]);
			print_cnt++;
			if (print_cnt == 16) {
				print_cnt = 0;
				Interp_Printf_P(interp, "\n");
			}
		}
		mem_addr += cnt;
	}
	if (print_cnt) {
		Interp_Printf_P(interp, "\n");
	}
	return 0;
}

INTERP_CMD(i2crCmd, "i2cr", cmd_i2cr, "i2cr        <i2caddr> <byteaddr> ?<count>?");

/**
 ***************************************************************************
 * \fn static int8_t cmd_i2cw(Interp * interp, uint8_t argc, char *argv[]);
 * Shell command for writing to the I2C-Bus. 
 ***************************************************************************
 */
static int8_t
cmd_i2cw(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t result;
	uint16_t i2c_addr;
	uint16_t mem_addr;
	uint8_t bufcnt = 0;
	uint8_t buf[8];
	uint8_t i;
	if (argc < 3) {
		return -EC_BADNUMARGS;
	}
	i2c_addr = astrtoi16(argv[1]);
	mem_addr = astrtoi16(argv[2]);
	for (i = 3; i < argc; i++) {
		buf[bufcnt++] = astrtoi16(argv[i]);
		if (bufcnt == array_size(buf)) {
			result = I2C_Write(i2c_addr, mem_addr, buf, bufcnt);
			if (result != I2C_RESULT_OK) {
				goto error;
			}
			bufcnt = 0;
			mem_addr += array_size(buf);
		}
	}
	if (bufcnt || (argc == 3)) {
		result = I2C_Write(i2c_addr, mem_addr, buf, bufcnt);
		if (result != I2C_RESULT_OK) {
			goto error;
		}
	}
	return 0;
 error:
	Interp_Printf_P(interp, "%02x I2C-Write failed\n", result);
	return -EC_ERROR;
}

INTERP_CMD(i2cwCmd, "i2cw", cmd_i2cw, "i2cw        <i2caddr> <byteaddr> <data1> ?<data2>? .....");

/**
 ****************************************************************************
 * \fn int8_t cmd_i2cscan(Interp *interp,uint8_t argc argc,char *argv[]);
 * Shell command for scanning all I2C busses.
 ****************************************************************************
 */
static int8_t
cmd_i2cscan(Interp * interp, uint8_t argc, char *argv[])
{
	int i;
	Interp_Printf_P(interp, "Bus 0: ");
	for (i = 0; i < 127; i++) {
		EV_Yield();
		if (I2C_AckPoll(i << 1, 1) == I2C_RESULT_OK) {
			i2c_stop(&gI2cm[0]);
			Interp_Printf_P(interp, "0x%02x ", i << 1);
		}
	}
	Interp_Printf_P(interp, "\nBus 1: ");
	for (i = 0; i < 127; i++) {
		EV_Yield();
		if (I2C_AckPoll(0x1000| (i << 1), 1) == I2C_RESULT_OK) {
			i2c_stop(&gI2cm[0]);
			Interp_Printf_P(interp, "0x%02x ", i << 1);
		}
	}
	Interp_Printf_P(interp, "\n");
	return 0;
}

INTERP_CMD(i2cscanCmd, "i2cscan", cmd_i2cscan, "i2cscan        # Scan for I2C devices ");

/**
 ***************************************************************************
 * \fn static int8_t cmd_i2cstat(Interp * interp, uint8_t argc, char *argv[])
 * Shell command for getting the status of this I2C-module. 
 ****************************************************************************
 */
static int8_t
cmd_i2cstat(Interp * interp, uint8_t argc, char *argv[])
{
	I2C_Master *i2cm = &gI2cm[0];
	Interp_Printf_P(interp, "Reset Count  : %u\n", i2cm->stat_bus_reset);
	Interp_Printf_P(interp, "No Ack       : %u\n", i2cm->stat_no_ack);
	Interp_Printf_P(interp, "SCL-Blocked  : %u\n", i2cm->stat_scl_blkd);
	Interp_Printf_P(interp, "Arb Loss     : %u\n", i2cm->stat_arb_loss);
	Interp_Printf_P(interp, "Fail Location: %u\n", i2cm->stat_fail_location);
	return 0;
}

INTERP_CMD(i2cstatCmd, "i2cstat", cmd_i2cstat, "i2cstat     # Dump the status of the I2C-Master");

/**
 ***********************************************************************************
 * \fn static int8_t cmd_cald(Interp *interp,uint8_t argc, char *argv[]);
 * Shell command for calculating the delay of one I2C_Delay() loop.
 ***********************************************************************************
 */
static int8_t
cmd_cald(Interp * interp, uint8_t argc, char *argv[])
{
	TimeMs_t actionStartTime;
	uint32_t reqTime;
	int i;
	i2c_delay = i2c_fast_delay;
	actionStartTime = TimeMs_Get();
	for (i = 0; i < 30000; i++) {
		i2c_ndelay(0);
	}
	reqTime = (TimeMs_Get() - actionStartTime) * UINT32_C(1000000);
	Con_Printf("Start %lu now %lu\n", actionStartTime, TimeMs_Get());
	Con_Printf("Total %ld us\n", reqTime / 1000);
	reqTime = reqTime / UINT32_C(30000);
	Con_Printf("Nanosecs per Delay loop %ld ns\n", reqTime);
	return 0;
}

INTERP_CMD(caldCmd, "cald", cmd_cald, "cald      # Calibrate I2C delay loop");


/**
 ****************************************************************************
 * \fn static int8_t cmd_i2cdelay(Interp *interp,uint8_t argc,char *argv[])
 * Configure the i2cdelay with a shell command.
 ****************************************************************************
 */
static int8_t
cmd_i2cdelay(Interp * interp, uint8_t argc, char *argv[])
{

	if (argc > 1) {
		if (strcmp(argv[1], "save") == 0) {
			//Param_Write(i2cdelay_fast,&i2c_fast_delay);
			//Param_Write(i2cdelay_slow,&i2c_lsdelay);
		} else {
			i2c_fast_delay = astrtoi16(argv[1]);
			if (argc > 2) {
				i2c_lsdelay = astrtoi16(argv[2]);
			}
		}
		return 0;
	} else {
		Interp_Printf_P(interp, "Delay Fast %d, Std %d, current %d\n", i2c_fast_delay,
				i2c_lsdelay, i2c_delay);
	}
	return 0;
}

INTERP_CMD(i2cdelayCmd, "i2cdelay", cmd_i2cdelay,
	   "i2cdelay <high-speed delay> <low-speed delay> # Get/Set i2cdelay");

static void
SetSDA_Bus0(uint8_t value)
{
	BMOD(1,PORTA.PDR.BYTE,!value); /* SDA Direction register */
}

static void
SetSCL_Bus0(uint8_t value)
{
	BMOD(2,PORTA.PDR.BYTE,!value); /* SCL Direction register */
}

static uint8_t 
GetSDA_Bus0(void)
{
	return PORTA.PIDR.BIT.B1;
}

static uint8_t 
GetSCL_Bus0(void)
{
	return PORTA.PIDR.BIT.B2;
}

static void
Init_Bus0(void)
{
	SetSCL_Bus0(1);
	SetSDA_Bus0(1);
	BCLR(1,PORTA.PODR.BYTE); /* SDA Output Register to Low */
	BCLR(2,PORTA.PODR.BYTE); /* SCL Output Register to Low */
}

/*
 * Bus 1 PLL
 */
static void
SetSDA_Bus1(uint8_t value)
{
	BMOD(6,PORTD.PDR.BYTE,!value); /* SDA Direction register */
}

static void
SetSCL_Bus1(uint8_t value)
{
	BMOD(7,PORTD.PDR.BYTE,!value); /* SCL Direction register */
}

static uint8_t 
GetSDA_Bus1(void)
{
	return PORTD.PIDR.BIT.B6;
}

static uint8_t 
GetSCL_Bus1(void)
{
	return PORTD.PIDR.BIT.B7;
}

static void
Init_Bus1(void)
{
	SetSCL_Bus1(1);
	SetSDA_Bus1(1);
	BCLR(6,PORTD.PODR.BYTE); /* SDA Output Register to Low */
	BCLR(7,PORTD.PODR.BYTE); /* SCL Output Register to Low */
}


/**
 ***********************************************************************
 * \fn void I2CM_Init(void)
 * Initialize the I2C master module. Configure port pins, reset the
 * I2C bus and meassure rise times. 
 ***********************************************************************
 */
void
I2CM_Init(void)
{
	I2C_Master *i2cm0 = &gI2cm[0];
	I2C_Master *i2cm1 = &gI2cm[1];
	Init_Bus0();
	i2cm0->setSDA = SetSDA_Bus0;
	i2cm0->getSDA = GetSDA_Bus0;
	i2cm0->setSCL = SetSCL_Bus0;
	i2cm0->getSCL = GetSCL_Bus0;

	I2C_ResetBus(0);
	I2C_RiseTimes(0);

	Init_Bus1();
	i2cm1->setSDA = SetSDA_Bus1;
	i2cm1->getSDA = GetSDA_Bus1;
	i2cm1->setSCL = SetSCL_Bus1;
	i2cm1->getSCL = GetSCL_Bus1;

	I2C_ResetBus(1);
	I2C_RiseTimes(1);

	Interp_RegisterCmd(&i2crCmd);
	Interp_RegisterCmd(&i2cwCmd);
	Interp_RegisterCmd(&i2cstatCmd);
	Interp_RegisterCmd(&i2cscanCmd);
	Interp_RegisterCmd(&caldCmd);
	Interp_RegisterCmd(&i2cdelayCmd);
}
