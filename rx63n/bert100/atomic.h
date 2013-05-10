#ifndef _ATOMIC_H
#define _ATOMIC_H
#include "types.h"

typedef uint32_t Flags_t;

#define RESTORE_FLAGS(flags)	{ \
	asm volatile ("MVTC %0,PSW\n":: "r" (flags):"memory"); \
}

#define SAVE_FLAGS(flags) { \
	 asm volatile("MVFC   PSW, %0\n" : "=r" (flags)::"memory"); \
}

#define SAVE_FLAGS_CLI(flags) { \
	 asm volatile("MVFC   PSW, %0\n" : "=r" (flags)::"memory"); \
	 asm volatile("CLRPSW   I\n" :::"memory"); \
}

#define DISABLE_IRQ() {\
	 asm volatile("CLRPSW   I\n" :::"memory"); \
}

#define ENABLE_IRQ() {\
	 asm volatile("SETPSW   I\n" :::"memory"); \
}

#define BSET(bit,obj) { \
	asm volatile("BSET %0, [%1].B" ::"i"(bit),"r"(&(obj)):"memory"); \
}
#define BCLR(bit,obj) { \
	asm volatile("BCLR %0, [%1].B" ::"i"(bit),"r"(&(obj)):"memory"); \
}

#define BMOD(bit,obj,val) { \
	if(val) { \
		BSET(bit,obj); \
	} else { \
		BCLR(bit,obj); \
	} \
}

/**
 ***********************************************************
 * \def barrier()
 * Barrier against instruction reordering at optimization. 
 ***********************************************************
 */
#define barrier() asm("":::"memory");

#define SAVE_FLAGS_SET_IPL(flags,ipl) { \
	SAVE_FLAGS(flags); \
	__builtin_rx_mvtipl(ipl); \
}
#endif
