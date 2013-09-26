/*------------------------------------------------------------------------
                                                                       |
   FILE        : reset_program.asm                                     |
   DATE        :  Tue, Apr 02, 2013                                    |
   DESCRIPTION :   Reset Program                                       |
   CPU TYPE    :    RX63N                                              |
                                                                       |
   This file is generated by KPIT GNU Project Generator (Ver.4.5).     |
                                                                       |
------------------------------------------------------------------------*/
                       
#include "stack.h"                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                          
                                                                           
                                                                           
                                                                         
	/*reset_program.asm*/

	.list
	.section        .bootloader_flash,"ax"
	.global _Bootloader_Reset    /*global Start routine */
	
	.extern _data
	.extern _mdata
	.extern _ebss
	.extern _bss
	.extern _edata
	.extern _main 
	.extern _thread0_stack
	.extern _istack0
	.extern _rvectors
#if GDB_SIMULATOR_DEBUG
	.extern _exit
#endif

	
_Bootloader_Reset :
/* initialise user stack pointer */
	//mvtc	#_ustack,USP
	mvtc    #0x17c00,USP

/* initialise interrupt stack pointer */
	//mvtc	#_istack,ISP
	mvtc #_istack0 + ISTACKSIZE,ISP

/* setup intb */
	mvtc	#_rvectors_start, intb	/* INTERRUPT VECTOR ADDRESS  definition	*/

/* setup FPSW */
	mvtc    #100h, fpsw	

/* load data section from ROM to RAM */

#if 0
	mov     #_mdata,r2      /* src ROM address of data section in R2 */
	mov     #_data,r1       /* dest start RAM address of data section in R1 */
	mov     #_edata,r3      /* end RAM address of data section in R3 */
	sub    r1,r3            /* size of data section in R3 (R3=R3-R1) */
	smovf                   /* block copy R3 bytes from R2 to R1 */
#endif

/* Copy the Bootloader from flash to RAM */
        mov             #0xfffff004,r2                  /* src ROM address of bootloader */
        mov             #_bootloader_start,r1   /* destination RAM address of bootloader */
        mov             #_bootloader_end,r3             /* end RAM address of bootloader */
        sub             r1,r3                                   /* size of data section in R3 */
        smovf


#if 0
/* bss initialisation : zero out bss */

	mov	#00h,r2  	/* load R2 reg with zero */
	mov	#_ebss, r3  /* store the end address of bss in R3 */
	mov	#_bss, r1 	/* store the start address of bss in R1 */
	sub   r1,r3	   	/* size of bss section in R3 (R3=R3-R1) */
	sstr.b

/* call the hardware initialiser */
	bsr.a	_HardwareSetup	
#endif
	nop

/* setup PSW */
	mvtc	#00000h, psw			/* Clear Ubit & Ibit for PSW */

/* change PSW PM to user-mode */
	MVFC   PSW,R1
	OR     #00020000h,R1
	PUSH.L R1
	MVFC   PC,R1
	ADD    #10,R1
	PUSH.L R1
	RTE
	NOP
	NOP

/* start user program */
 .global _BootloaderStart    /*global Start routine */
	bra.a _BootloaderStart

.section .fvectors,"ax"
//;0xffffff80  MDES - Endian Select Register
    .long  0xffffffff
//;0xffffff84  Reserved
    .long  0xffffffff
//;0xffffff88  Reserved
    .long  0xffffffff
//;0xffffff8C  Reserved
    .long  0xffffffff
//;0xffffff90  Reserved
    .long  0xffffffff
//;0xffffff94  Reserved
    .long  0xffffffff
//;0xffffff98  Reserved
    .long  0xffffffff
//;0xffffff9C  Reserved
    .long  0xffffffff
//;0xffffffA0  Reserved
    .long  0xffffffff
//;0xffffffA4  Reserved
    .long  0xffffffff
//;0xffffffA8  Reserved
    .long  0xffffffff
//;0xffffffAC  Reserved
    .long  0xffffffff
//;0xffffffB0  Reserved
    .long  0xffffffff
//;0xffffffB4  Reserved
    .long  0xffffffff
//;0xffffffB8  Reserved
    .long  0xffffffff
//;0xffffffBC  Reserved
    .long  0xffffffff
//;0xffffffC0  Reserved
    .long  0xffffffff
//;0xffffffC4  Reserved
    .long  0xffffffff
//;0xffffffC8  Reserved
    .long  0xffffffff
//;0xffffffCC  Reserved
    .long  0xffffffff
//;0xffffffd0  Exception(Supervisor Instruction)
    .long  0xffffEfd0
//;0xffffffd4  Reserved
    .long  0
//;0xffffffd8  Reserved
    .long  0
//;0xffffffdc  Exception(Undefined Instruction)
    .long 0xffffEfdc
//;0xffffffe0  Reserved
    .long  0
//;0xffffffe4  Exception(Floating Point)
    .long 0xffffEfe4
//;0xffffffe8  Reserved
    .long  0
//;0xffffffec  Reserved
    .long  0
//;0xfffffff0  Reserved
    .long  0
//;0xfffffff4  Reserved
    .long  0
//;0xfffffff8  NMI
    .long 0xffffeff8
//;0xfffffffc  RESET
    .long _Bootloader_Reset                                                                                                                  
.text	
.end
