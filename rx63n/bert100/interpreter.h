/**
 ***************************************************************
 * \file interpreter.h
 * Command shell for serial console.
 ***************************************************************
 */
#include <stdarg.h>
#include "types.h"
#include "events.h"
#include "fat.h"

#define INTERP_MAX_ARGC (16)
typedef void Interp_OutProc(const char *str);
typedef void Interp_PrintVA(const char *format, va_list ap);

/*
 *************************************************************
 * Error Codes returned by commands executed by interpreter,
 *************************************************************
 */
#define EC_ERROR		(1)
#define EC_TIMEDOUT 	(2)
#define EC_BADNUMARGS	(3)
#define EC_ARGRANGE		(4)
#define EC_BADARG		(5)

typedef struct Interp Interp;
typedef int8_t InterpCmdProc(Interp *, uint8_t argc, char *argv[]);

/**
 ********************************************************************
 * \struct Cmd;
 * Command information stored in a special linker section. 
 ********************************************************************
 */
typedef struct Cmd {
	InterpCmdProc *cmdProc;	/**< Procedure invoked when command is encountered. */
	const char *name;  /**< The command keyword. */
	const char *help;  /**< A free style text explaining the command. */
	struct Cmd *next;
} Cmd;

/**
 ***********************************************************
 * \struct Interp;
 * The Interpreter for the command shell. 
 * All fields are private.
 ***********************************************************
 */
struct Interp {
	Event nextLineEvent;
	FIL file;
	Interp_OutProc *OutStr;	 /**< Ouput procedure for strings */
	Interp_PrintVA *PrintVA_P; /**< Output procedure for formated strings in program memory. */
	uint8_t argc;		 /**< Argument count of last decoded command. */
	char *argv[INTERP_MAX_ARGC]; /**< Arguments of last decoded command. */
};

/**
 *********************************************************************************
 * \fn Interp *Interp_Init(Interp_OutProc * outStr, Interp_Printf * Printf_P);
 * Initialize the interpreter. 
 *********************************************************************************
 */
Interp *Interp_Init(Interp_OutProc * outStr, Interp_PrintVA * PrintVA_P);
/**
 *******************************************************************
 * \fn void Interp_StartScript(Interp *interp,char *path);
 * Run a shell script from the filesystem
 *******************************************************************
 */
bool Interp_StartScript(const char *path);
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
static inline void
Interp_OutStr(Interp * interp, char *str)
{
	interp->OutStr(str);
}

void Interp_Printf_P(Interp * interp, const char *format, ...);

/**
 ***************************************************************************
 * \var _progmem_cmds_start;
 * Start of Commands stored in a reserved linker section for commands.
 ***************************************************************************
 */
extern const Cmd _progmem_cmds_start;
/**
 ***************************************************************************
 * \var _progmem_cmds_end;
 * End of Commands stored in a reserved linker section for commands.
 ***************************************************************************
 */
extern const Cmd _progmem_cmds_end;

/**
 ***************************************************************************
 * \fn void Interp_RegisterCmd(Cmd *cmd)
 * Register a command. This function can be called BEFORE the interpreter
 * is initialized !
 ***************************************************************************
 */
void Interp_RegisterCmd(Cmd * cmd);

#define INTERP_CMD_ATTR __attribute__ ((unused,section (".progmem.cmds")))

#define INTERP_CMD(cmdvar,cmdstr,proc,help) \
	static Cmd cmdvar = {proc, cmdstr, help};
