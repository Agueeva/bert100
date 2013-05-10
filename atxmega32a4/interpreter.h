/**
 ***************************************************************
 * \file interpreter.h
 * Command shell for serial console.
 ***************************************************************
 */
#include <stdint.h>
#include <stdarg.h>
#include <avr/pgmspace.h>

#define INTERP_MAX_ARGC (11)
typedef void Interp_OutProc(const char *str);
typedef void Interp_Printf(const char *format,...);

/*
 *************************************************************
 * Error Codes returned by commands executed by interpreter,
 *************************************************************
 */
#define EC_OK		(0)
#define EC_ERROR	(1)
#define EC_TIMEDOUT 	(2)
#define EC_BADNUMARGS	(3)
#define EC_ARGRANGE	(4)
#define EC_BADARG	(5)

/**
 ***********************************************************
 * \struct Interp;
 * The Interpreter for the command shell. 
 * All fields are private.
 ***********************************************************
 */
typedef struct Interp {
	Interp_OutProc *OutStr;	 /**< Ouput procedure for strings */
	Interp_Printf *Printf_P; /**< Output procedure for formated strings in program memory. */
	uint8_t argc;		 /**< Argument count of last decoded command. */
	char *argv[INTERP_MAX_ARGC]; /**< Arguments of last decoded command. */
} Interp;

/**
 *********************************************************************************
 * \fn Interp *Interp_Init(Interp_OutProc * outStr, Interp_Printf * Printf_P);
 * Initialize the interpreter. 
 *********************************************************************************
 */
Interp *Interp_Init(Interp_OutProc * outStr, Interp_Printf * Printf_P);
/*
 *****************************************************************************
 * Interp_Feed
 * For use by the serial console to feed the interpreter with commands.
 *****************************************************************************
 */
void Interp_Feed(void *clientData, char *cmdline);

/*
 ***********************************************************
 * For use by implementors of the debug Interface only.
 ***********************************************************
 */
static inline void Interp_OutStr(Interp * interp, char *str)
{
	interp->OutStr(str);
}

#define Interp_Printf_P(interp,str...) __Interp_Printf_P(interp,str,0) 
#define __Interp_Printf_P(interp,str,y...) ((interp)->Printf_P(PSTR(str),y))
typedef int8_t InterpCmdProc(Interp *, uint8_t argc, char *argv[]);

/**
 ********************************************************************
 * \struct Cmd;
 * Command information stored in a special linker section. 
 ********************************************************************
 */
typedef struct Cmd {
	InterpCmdProc *cmdProc; /**< Procedure invoked when command is encountered. */
	const prog_char *name;  /**< The command keyword. */
	const prog_char *help;  /**< A free style text explaining the command. */
} Cmd;

/**
 ***************************************************************************
 * \var _progmem_cmds_start;
 * Start of Commands stored in a reserved linker section for commands.
 ***************************************************************************
 */
extern const Cmd PROGMEM _progmem_cmds_start;
/**
 ***************************************************************************
 * \var _progmem_cmds_end;
 * End of Commands stored in a reserved linker section for commands.
 ***************************************************************************
 */
extern const Cmd PROGMEM _progmem_cmds_end;

#define INTERP_CMD_ATTR __attribute__ ((unused,section (".progmem.cmds")))

#define INTERP_CMD(cmd,proc,help) \
	static char PROGMEM cmdstr_##cmd[] = #cmd; \
	static char PROGMEM cmdhlp_##cmd[] = (help); \
	Cmd _cmd_##cmd[] INTERP_CMD_ATTR = {   	  \
		{			  	  \
			(proc),			  \
			cmdstr_##cmd,		  \
			cmdhlp_##cmd		  \
		}			  	  \
	};
