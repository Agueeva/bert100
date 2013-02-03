/*
 *************************************************************
 * (C) 2009 Jochen Karrer
 *//**
 * \file irqflags.h
 * CPU-Flags: Interrupt disabling and restoring for AVR8.
 * Required only for non-balanced access to interrupt flags.
 * Balanced access can be done with the routines comming with
 * WinAVR.
 *************************************************************
 */
#ifndef _IRQFLAGS_H
#define _IRQFLAGS_H
#include <avr/io.h>
#include <stdint.h>

typedef uint8_t Flags_t;

#define IPL_ALLON 	(PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm)
#define IPL_LOW 	(PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm)
#define IPL_MEDIUM 	(PMIC_HILVLEN_bm)
#define IPL_HIGH	(0)

/**
 ***********************************************************
 * \fn static inline void barrier(void)
 * Instruktion reordering barrier for gcc.
 ***********************************************************
 */
static inline void barrier(void)
{
	asm volatile ("":::"memory");
}
/**
 **********************************************************
 * \fn static inline uint8_t save_flags_cli(void)
 * Save the flags and disable the interrupts 
 **********************************************************
 */
static inline Flags_t save_flags_cli(void)
{
	Flags_t flags;
	asm volatile ("in %0, 0x3f \n" "cli	     \n":"=g" (flags)
		      :		/* no input */
		      :"memory");
	return flags;
}

/**
 **************************************************************
 * Disable only some interrupts 
 **************************************************************
 */
static inline uint8_t 
save_ipl_set(uint8_t newctrl) {
	uint8_t ctrl = PMIC_CTRL;	
	PMIC_CTRL = newctrl;
	return ctrl;
}

/**
 **************************************************************
 * Restore an old pmic.ctrl value
 **************************************************************
 */
static inline void restore_ipl(uint8_t pmic) 
{
	PMIC_CTRL = pmic;
}

/**
 ***********************************************************
 * \fn static inline void restore_flags(Flags_t flags)
 * Restore old flags;
 ***********************************************************
 */
static inline void restore_flags(Flags_t flags)
{
	asm volatile ("out 0x3f, %0 \n": /* No output */
		      :"g" (flags)
		      :"memory", "cc");
}

#endif
