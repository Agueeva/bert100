/*
 *************************************************************
 * (C) 2009 Jochen Karrer
 *//**
 * \file i2cmaster.c
 * Soft-I2C master implementation for AVR8
 *************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "i2cmaster.h"
#include "events.h"
#include "console.h"
#include "interpreter.h"
#include "hex.h"
#include "timer.h"

#define TRAFIC_LEDS 1
#define USE_INTERNAL_PULLUP 0
#define MAX_ACK_POLLS	(3)

#define PORT_SDA	PORTE
#define PORT_SCL	PORTE
#define PINCTRL_SDA	PORTE.PIN0CTRL
#define PINCTRL_SCL	PORTE.PIN1CTRL
#define PIN_SDA 0
#define PIN_SCL 1

#define I2C_SPEED_FAST
#ifdef I2C_SPEED_FAST
#define T_HDSTA 600		// 0.6 uS
#define T_SUSTA 600		// 0.6 uS
#define T_SUSTO 600		// 0.6 uS
#define T_HIGH  600		// 0.6 uS
#define T_LOW   1300		// 1.3 uS
#define T_BUF   1300		// 1.3 uS
#define T_SUDAT 100		// 0.1 uS
#else
	/* speed = I2C_SPEED_STD */
#define T_HDSTA  4000		// 4.0 uS
#define T_SUSTA  4700		// 4.7 uS
#define T_SUSTO  4000		// 4.0 uS
#define T_HIGH   4000		// 4.0 uS
#define T_BUF    4700		// 4.7 uS
#define T_LOW    4700		// 4.7 uS
#define T_SUDAT  250		// 0.25 uS
#endif

typedef struct I2C_Master {
	uint16_t stat_fail_location;
	uint8_t stat_ackpoll_fail;
	uint8_t stat_bus_reset;
} I2C_Master;

I2C_Master g_i2cm;

/*
 *****************************************************
 * GPIO Pin write functions 
 *****************************************************
 */
static inline void 
SDA_H(void)
{
#if TRAFIC_LEDS == 1
#endif
	PORT_SDA.OUTSET = (1 << PIN_SDA);
#if USE_INTERNAL_PULLUP == 1
#endif
}

static inline void
SCL_H(void)
{
#if TRAFIC_LEDS == 1
#endif
	PORT_SCL.OUTSET = (1 << PIN_SCL);
#if USE_INTERNAL_PULLUP == 1
#endif
}

static inline void 
SDA_L(void)
{
#if TRAFIC_LEDS == 1
#endif
#if USE_INTERNAL_PULLUP == 1
#endif
	PORT_SDA.OUTCLR = (1 << PIN_SDA);
}

static inline void 
SCL_L(void)
{
#if TRAFIC_LEDS == 1
#endif
#if USE_INTERNAL_PULLUP == 1
#endif
	PORT_SCL.OUTCLR = (1 << PIN_SCL);
}

/*
 ************************************************
 * GPIO Pin read
 ************************************************
 */
static inline uint8_t GET_SDA(void)
{
	return (PORT_SDA.IN >> PIN_SDA) & 1;
}

static inline uint8_t GET_SCL(void)
{
	return (PORT_SCL.IN >> PIN_SCL) & 1;
}

static uint8_t i2c_check_arb(void)
{
	if (!GET_SDA()) {
		return I2C_RESULT_ARB_LOSS;
	} else {
		return 0;
	}
}

static void i2c_ndelay(uint16_t nanos)
{
	volatile uint8_t i;	
	for(i = 0; i < 1; i++) {
		/* do nothing */
	}
#if 0
	int8_t us = nanos / 1000;
	while (us-- >= 0) {
		_delay_us(1);
	}
	//_delay_us(100000); // jk
	//_delay_ms(nanos);
#endif
}

static inline void i2c_cdelay(uint16_t nanos)
{
	volatile uint8_t i;	
	for(i = 0; i < 7; i++) {
		/* do nothing */
	}
}

/*
 ***********************************************************************
 * i2c_sync
 * Wait for maximal 510us for release of the SCL Line
 * If a slave streches SCL for a longer period it is a bad slave 
 ***********************************************************************
 */
static uint8_t i2c_sync(void)
{
	uint8_t count;
	for (count = 0; count < 255; count++) {
		if (GET_SCL()) {
			return 0;
		}
		_delay_us(2);
	}
	return I2C_RESULT_SCL_BLKD;
}

/*
 ************************************************************
 * i2c_start()
 * Generate a Start condition
 * Start condition assumes that scl and sda are high
 ************************************************************
 */
static uint8_t i2c_start(void)
{
	uint8_t ret;
	i2c_sync();
	if ((ret = i2c_check_arb()) != 0) {
		return ret;
	}
	SDA_L();
	i2c_ndelay(T_HDSTA);
	SCL_L();
	return 0;
}

/*
 ***************************************************************
 * i2c_repstart()
 * Generate a repeated start condition
 ***************************************************************
 */

static uint8_t __attribute__((unused)) i2c_repstart(void)
{
	uint8_t ret;
	SDA_H();
	i2c_ndelay(T_SUDAT);
	SCL_H();
	i2c_ndelay(T_SUSTA);
	if (i2c_sync() != 0) {

	}
	if ((ret = i2c_check_arb()) != 0) {
		return ret;
	}
	SDA_L();
	i2c_ndelay(T_HDSTA);
	SCL_L();
	i2c_ndelay(T_LOW);
	return 0;
}

/**
 ******************************************************************
 * \fn static void i2c_stop(void);
 * Generate a stop condition
 ******************************************************************
 */
static void i2c_stop(void)
{
	SDA_L();
	i2c_ndelay(T_LOW);
	SCL_H();
	i2c_sync();
	i2c_ndelay(T_SUSTO);
	SDA_H();
	i2c_ndelay(T_BUF);
	return;
}

/*
 *****************************************************************
 * \fn static uint8_t i2c_readb(int ack, uint8_t * b);
 * Read a Byte as master
 *****************************************************************
 */
static uint8_t i2c_readb(int ack, uint8_t * b)
{
	int8_t i;
	uint8_t ret;
	uint8_t data = 0;
	SDA_H();
	for (i = 7; i >= 0; i--) {
#if 0
		EV_EnterCritical();
		Con_Printf_P("Before Bit%d: %02x, TWDR %02x sda %d\n",i,data,TWDR,GET_SDA());
		EV_LeaveCritical();
#endif
		i2c_ndelay(T_LOW);
		SCL_H();

#if 0
		EV_EnterCritical();
		Con_Printf_P("Before after SCL_H %d: %02x, TWDR %02x sda %d\n",i,data,TWDR,GET_SDA());
		EV_LeaveCritical();
#endif

		i2c_sync();
		i2c_ndelay(T_HIGH);
		if (GET_SDA()) {
			data |= (1 << i);
		}
		SCL_L();
	}
#if 0
	EV_EnterCritical();
	Con_Printf_P("After Bit%d: %02x, TWDR %02x\n",i,data,TWDR);
	EV_LeaveCritical();
#endif
	if (ack) {
		SDA_L();
	}
	i2c_ndelay(T_LOW);
	SCL_H();
	i2c_sync();
	i2c_ndelay(T_HIGH);

#if 0
	EV_EnterCritical();
	Con_Printf_P("After Ack Bit%d: %02x, TWDR %02x\n",i,data,TWDR);
	EV_LeaveCritical();
#endif

	*b = data;
	if (!ack) {
		if ((ret = i2c_check_arb())) {
			return ret;
		}
	}
	SCL_L();
	return 0;
}

/*
 *****************************************************************
 * \fn static uint8_t i2c_writeb(uint8_t value)
 * Write a Byte as master.
 * \param value The byte which will be written to the I2C bus.
 *****************************************************************
 */
static uint8_t i2c_writeb(uint8_t value)
{
	int8_t i;
	uint8_t ret;
	uint8_t ack;
	for (i = 7; i >= 0; i--) {
		int8_t bit = (value >> i) & 1;
		if (bit) {
			SDA_H();
		} else {
			SDA_L();
		}
		i2c_ndelay(T_LOW);
		SCL_H();
		ret = i2c_sync();
		if (ret != I2C_RESULT_OK) {
			return ret;
		}
		i2c_ndelay(T_HIGH);
		if (bit) {
			ret = i2c_check_arb();
			if (ret != I2C_RESULT_OK) {
				return ret;
			}
		}
		SCL_L();
	}
	// i2c_ndelay(T_HDDAT); 
	SDA_H();
	i2c_ndelay(T_LOW);
	SCL_H();
	i2c_sync();
	i2c_ndelay(T_HIGH);
	ack = !GET_SDA();
	SCL_L();
	if (ack) {
		return 0;
	} else {
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
static void i2c_reset(void)
{
	uint8_t i;
	g_i2cm.stat_bus_reset++;
	SCL_L();
	SDA_H();
	i2c_ndelay(5000);
	for (i = 0; i < 10; i++) {
		SCL_H();
		i2c_sync();
		i2c_ndelay(5000);
		SCL_L();
		i2c_ndelay(5000);
	}
	i2c_stop();
}

/**
 **************************************************************
 * \fn static uint8_t I2C_AckPoll(uint8_t i2caddr)
 * Send and I2C address repeatedly until a acknowledge is 
 * received.  
 * \param i2caddr The I2C address of the device which will be polled.
 **************************************************************
 */
static uint8_t I2C_AckPoll(uint8_t i2caddr)
{
	uint8_t i, result;
	int8_t reset = 0;
	for (i = 0; i < MAX_ACK_POLLS; i++) {
		result = i2c_start();
		if (result != I2C_RESULT_OK) {
			/* Reset only once */
			if (reset == 0) {
				reset++;
				i2c_reset();
				continue;
			} else {
				return result;
			}
		}
		result = i2c_writeb(i2caddr);
		if (result == I2C_RESULT_OK) {
			return result;
		} else if (result != I2C_RESULT_NO_ACK) {
			return result;
		}
		i2c_stop();
	}
	return I2C_RESULT_NO_ACK;
}

/** 
 ************************************************************************
 * \fn uint8_t I2C_Read8(uint8_t i2caddr, uint8_t daddr, uint8_t * data, uint8_t count)
 * Read an data block from an I2C-Device. 
 * \param i2caddr The I2C Address of the device.
 * \param daddr The 8 Bit address of the first byte read from the I2C device.
 * \param data Pointer to the destination for the data block.
 * \param count number of bytes to read. 
 ************************************************************************
 */

uint8_t I2C_Read8(uint8_t i2caddr, uint8_t daddr, uint8_t * data, uint8_t count)
{
	uint8_t result;
	uint8_t i;
	//EV_DoOneEvent();
	result = I2C_AckPoll(i2caddr);
	if (result != I2C_RESULT_OK) {
		/* 
	         * Do not go to out, there was already a stop 
		 *  condition
		 */
		return result;
	}
	result = i2c_writeb(daddr);
	if (result != I2C_RESULT_OK) {
		goto out;
	}
	/* The Helix Chips can not handle Repeated start */
	i2c_stop();
	result = i2c_start();
	if (result != 0) {
		return result;
	}
	result = i2c_writeb(i2caddr | 1);	/* DIR is read */
	if (result != I2C_RESULT_OK) {
		goto out;
	}
	for (i = 0; i < count; i++) {
		if ((i + 1) == count) {
			result = i2c_readb(0, data + i);
		} else {
			result = i2c_readb(1, data + i);
		}
		if (result != 0) {
			return result;
		}
	}
 out:
	if (result != I2C_RESULT_ARB_LOSS) {
		i2c_stop();
	}
	return result;
}

/** 
 *****************************************************************
 * \fn uint8_t I2C_Write8(uint8_t i2caddr, uint8_t daddr, const uint8_t * data, uint8_t count);
 * Write a data block to an I2C-Device. 
 * \param i2caddr The I2C Address of the device.
 * \param daddr The 8 Bit address of the first written byte in the I2C device.
 * \param data Pointer to the data block to write.
 * \param count number of bytes to write. 
 *****************************************************************
 */
uint8_t
I2C_Write8(uint8_t i2caddr, uint8_t daddr, const uint8_t * data, uint8_t count)
{
	uint8_t result;
	uint8_t i;
	//EV_DoOneEvent();
	result = I2C_AckPoll(i2caddr);
	if (result != I2C_RESULT_OK) {
		/* 
	         * Do not go to out, there was already a stop 
		 *  condition
		 */
		return result;
	}
	result = i2c_writeb(daddr);
	if (result != I2C_RESULT_OK) {
		goto out;
	}
	for (i = 0; i < count; i++) {
		result = i2c_writeb(data[i]);
		if (result != I2C_RESULT_OK) {
			goto out;
		}
	}
 out:
	if (result != I2C_RESULT_ARB_LOSS) {
		i2c_stop();
	}
	return result;
}

/**
 **********************************************************
 * \fn static void I2C_RiseTimes(void);
 * Meassure the I2C rise times and print them
 * on the serial console.
 **********************************************************
 */
static void I2C_RiseTimes(void)
{
	uint8_t sda_count = 0;
	uint8_t scl_count = 0;
	uint8_t i;
	SDA_L();
	i2c_ndelay(5000);
	SDA_H();
	for (i = 100; i > 0; i--) {
		if (GET_SDA() == 0) {
			sda_count++;
		}
	}
	SCL_L();
	i2c_ndelay(5000);
	SCL_H();
	for (i = 100; i > 0; i--) {
		if (GET_SCL() == 0) {
			scl_count++;
		}
	}
	Con_Printf_P("I2C-Master Rise Time Check: SDA %d, SCL %d: ",sda_count,scl_count); 
	if ((sda_count <= 2) && (scl_count <= 2)) {
		Con_Printf_P("Good\n");
	} else if ((sda_count <= 3) && (scl_count <= 3)) {
		Con_Printf_P("Acceptable\n");
	} else if ((sda_count <= 4) && (scl_count <= 4)) {
		Con_Printf_P("Bad\n");
	} else {
		Con_Printf_P("Very Bad\n");
	}
}

/**
 ***********************************************************************
 * \fn void I2CM_Init(void)
 * Initialize the I2C master module. Configure port pins, reset the
 * I2C bus and meassure rise times. 
 ***********************************************************************
 */
void I2CM_Init(void)
{
	memset(&g_i2cm, 0, sizeof(g_i2cm));
	PINCTRL_SDA = PORT_OPC_WIREDANDPULL_gc;
	PINCTRL_SCL = PORT_OPC_WIREDANDPULL_gc;
	PORT_SDA.OUT |= (1 << PIN_SDA);
	PORT_SCL.OUT |= (1 << PIN_SCL);
	PORT_SDA.DIRSET = (1 << PIN_SDA);
	PORT_SCL.DIRSET = (1 << PIN_SCL);
#if 0
	while(1) {
	PINCTRL_SDA = PORT_OPC_TOTEM_gc;
	PINCTRL_SCL = PORT_OPC_TOTEM_gc;
	PORT_SDA.OUTCLR = (1 << PIN_SDA);
	PORT_SCL.OUTCLR = (1 << PIN_SCL);
	}
#endif
	i2c_reset();
	I2C_RiseTimes();
}

/**
 ******************************************************************
 * \fn static int8_t cmd_i2cr(Interp * interp, uint8_t argc, char *argv[])
 * Shell command for reading from the I2C-Bus.
 ******************************************************************
 */
static int8_t cmd_i2cr(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t result;
	uint8_t i2c_addr;
	uint8_t mem_addr;
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
		if ((result =
		     I2C_Read8(i2c_addr, mem_addr, buf,
			       cnt)) != I2C_RESULT_OK) {
			Interp_Printf_P(interp, "%02x I2C-Read failed\n",result);
			return -EC_ERROR;
		}
		for (i = 0; i < cnt; i++) {
			Interp_Printf_P(interp,"%02x ",buf[i]);
			print_cnt++;
			if(print_cnt == 16) {
				print_cnt = 0;
				Interp_Printf_P(interp, "\n");
			}
		}
		mem_addr += cnt;
	}
	if(print_cnt) {
		Interp_Printf_P(interp, "\n");
	}
	return 0;
}

INTERP_CMD(i2cr, cmd_i2cr, "i2cr        <i2caddr> <byteaddr> ?<count>?");

/**
 ***************************************************************************
 * \fn static int8_t cmd_i2cw(Interp * interp, uint8_t argc, char *argv[]);
 * Shell command for writing to the I2C-Bus. 
 ***************************************************************************
 */
static int8_t cmd_i2cw(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t result;
	uint8_t i2c_addr;
	uint8_t mem_addr;
	uint8_t bufcnt = 0;
	uint8_t buf[4];
	uint8_t i;
	if (argc < 4) {
		return -EC_BADNUMARGS;
	}
	i2c_addr = astrtoi16(argv[1]);
	mem_addr = astrtoi16(argv[2]);
	for (i = 3; i < argc; i++) {
		buf[bufcnt++] = astrtoi16(argv[i]);
		if (bufcnt == 4) {
			result = I2C_Write8(i2c_addr, mem_addr, buf, bufcnt);
			if (result != I2C_RESULT_OK) {
				goto error;
			}
			bufcnt = 0;
		}
	}
	if (bufcnt) {
		result = I2C_Write8(i2c_addr, mem_addr, buf, bufcnt);
		if (result != I2C_RESULT_OK) {
			goto error;
		}
	}
	return 0;
error:
	Interp_Printf_P(interp, "%02x I2C-Write failed\n",result);
	return -EC_ERROR;
}

INTERP_CMD(i2cw, cmd_i2cw,
	   "i2cw        <i2caddr> <byteaddr> <data1> ?<data2>? .....");

/**
 ***************************************************************************
 * \fn static int8_t cmd_i2cstat(Interp * interp, uint8_t argc, char *argv[])
 * Shell command for getting the status of this I2C-module. 
 ****************************************************************************
 */
static int8_t cmd_i2cstat(Interp * interp, uint8_t argc, char *argv[])
{
	Interp_Printf_P(interp, "Reset Count  : %02x\n",g_i2cm.stat_bus_reset);
	Interp_Printf_P(interp, "Ackpoll Fail : %02x\n",g_i2cm.stat_ackpoll_fail);
	Interp_Printf_P(interp, "Fail Location: %04x\n",g_i2cm.stat_fail_location);
	return 0;
}

INTERP_CMD(i2cstat, cmd_i2cstat,
	   "i2cstat     # Dump the status of the I2C-Master");

/**
 ********************************************************************************
 * \fn static int8_t cmd_i2cscan(Interp * interp, uint8_t argc, char *argv[])
 * Shell command scanning for I2C devices.
 ********************************************************************************
 */
static int8_t cmd_i2cscan(Interp * interp, uint8_t argc, char *argv[])
{
	uint8_t i;
	for(i=0;i<127;i++) {
		if(I2C_AckPoll(i << 1) == I2C_RESULT_OK) {
			i2c_stop();
			Interp_Printf_P(interp,"0x%02x ",i<<1);
		}
	}
	Interp_Printf_P(interp,"\n");
	return 0;
}

INTERP_CMD(i2cscan, cmd_i2cscan,
           "i2cscan     # Scan for I2C devices ");

static int8_t cmd_cald(Interp * interp, uint8_t argc, char *argv[])
{
	uint16_t i;
	uint32_t diff;
	TimeMs_t after;
	TimeMs_t before = TimeMs_Get();
	for(i = 0; i < 33333;i++) {
		i2c_cdelay(0);
	}
	after = TimeMs_Get();
	diff = after - before;	
	diff = diff * 30;	
	Interp_Printf_P(interp,"%d nanoseconds\n",diff);
	return 0;
}

INTERP_CMD(cald, cmd_cald,
           "cald        # Delay loop time check");
