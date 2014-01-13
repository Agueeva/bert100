/*
 *************************************************************
 * (C) 2009 Jochen Karrer
    *//**
 * \file interpreter.c
 * Command interpreter for a serial console. Taken from
 * an Atmel ATMega644 Optical Transceiver Module simulator.
 *************************************************************
 */
#include "types.h"
#include <string.h>
#include <stdlib.h>
#include "interpreter.h"
#include "editor.h"
#include "fat.h"

Interp g_interp;

static Cmd *cmdHead = NULL;

void
Interp_Printf_P(Interp * interp, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	(interp)->PrintVA_P(format, ap);
	va_end(ap);
}

/**
 ****************************************************************************
 * \fn static uint8_t split_args(char *script, char *argv[])
 * Split a commandline into arguments by substituting
 * spaces with 0 and seting up an argument vector. 
 * Be careful because the string and the
 * argument vector are modified in place !  
 * \param script Pointer to a command line. Will be modified in place.
 * \param argv Pointer to an argument vector. Will be modified in place.
 * \return Number of arguments detected.
 ****************************************************************************
 */
#define STATE_INSPACE	(1)
#define STATE_ESCAPE	(2)
#define STATE_TEXT	(0)
static uint8_t
split_args(char *script, char *argv[])
{
	uint8_t i, argc = 0;
	uint8_t state = STATE_INSPACE;
	argv[0] = script;
	for (i = 0; script[i] && (script[i] != 0x0a) && (script[i] != 0xd); i++) {
		if ((state == STATE_INSPACE) && (script[i] == ' ')) {
			continue;
		}
		if (state == STATE_INSPACE) {
			if (script[i] == '"') {
				if (script[i]) {
					argv[argc++] = script + i + 1;
				}
				state = STATE_ESCAPE;
			} else {
				argv[argc++] = script + i;
				state = STATE_TEXT;
			}
			if (argc >= INTERP_MAX_ARGC) {
				return argc;
			}
			continue;
		}
		if ((state == STATE_ESCAPE) && (script[i] == '"')) {
			script[i] = 0;
			state = STATE_INSPACE;
		}
		if ((state == STATE_TEXT) && (script[i] == ' ')) {
			script[i] = 0;
			state = STATE_INSPACE;
		}
	}
	script[i] = 0;
	return argc;
}

/*
 *********************************************************************
 * \fn InterpCmdProc *find_cmd_proc(Interp * interp, char *cmdname)
 * Find a command procedure by a linear search in a linker section
 * reserved for commands. 
 * \param interp The interpreter.
 * \param cmdname The command which will be searched.
 * \return Pointer the the command procedure or NULL if not found.
 *********************************************************************
 */
static InterpCmdProc *
find_cmd_proc(Interp * interp, char *cmdname)
{
	const Cmd *cursor;
	for (cursor = cmdHead; cursor; cursor = cursor->next) {
		if (strcmp(cmdname, cursor->name) == 0) {
			return cursor->cmdProc;
			break;
		}
	}
	return NULL;
}

/**
 ************************************************************
 * \fn static void Interp_PrintError(Interp * interp, int8_t ec)
 * Translate an errorcode in a readable error message 
 * \param interp Interpreter.
 * \param ec The error code.
 ************************************************************
 */
static void
Interp_PrintError(Interp * interp, int8_t ec)
{
	switch (ec) {
	    case -EC_ERROR:
		    Interp_Printf_P(interp, "Error\n");
		    break;

	    case -EC_BADNUMARGS:
		    Interp_Printf_P(interp, "Bad number of arguments\n");
		    break;
	    case -EC_TIMEDOUT:
		    Interp_Printf_P(interp, "Timeout\n");
		    break;
	    case -EC_ARGRANGE:
		    Interp_Printf_P(interp, "Argument out of Range\n");
		    break;
	    case -EC_BADARG:
		    Interp_Printf_P(interp, "Bad argument format\n");
	}
}

/**
 *************************************************************************
 * \fn void Interp_Feed(void *clientData, char *cmdline)
 * Feed the Interpreter with lines for interpretation. Called for
 * example by the commandline editor whenever a line of input is complete.
 *************************************************************************
 */
void
Interp_Feed(void *clientData, char *cmdline)
{
	int8_t result;
	char argbuf[EDITOR_MAX_CMDLINE_LEN + 1];
	InterpCmdProc *cmdProc;
	Interp *interp = (Interp *) clientData;
	strcpy(argbuf, cmdline);
	interp->argc = split_args(argbuf, interp->argv);
	if (interp->argc == 0) {
		return;
	}
	cmdProc = find_cmd_proc(interp, interp->argv[0]);
	if (cmdProc == NULL) {
		Interp_OutStr(interp, interp->argv[0]);
		Interp_Printf_P(interp, ": CMD not found\n");
	} else {
		result = cmdProc(interp, interp->argc, interp->argv);
		if (result < 0) {
			Interp_PrintError(interp, result);
		}
	}
}

/**
 *************************************************************************
 * \fn static int8_t cmd_help(Interp * interp, uint8_t argc, char *argv[])
 * Print a help text for a single command. If no command name is given
 * print the help of all commands. This command walks through the
 * command section of a reserved linker section. 
 *************************************************************************
 */

static int8_t
cmd_help(Interp * interp, uint8_t argc, char *argv[])
{
	const Cmd *cursor;
	const char *help;
	for (cursor = cmdHead; cursor; cursor = cursor->next) {
		if ((argc < 2) || (strcmp(argv[1], cursor->name) == 0)) {
			help = cursor->help;
			Interp_Printf_P(interp, "    ");
			Interp_Printf_P(interp, help);
			Interp_Printf_P(interp, "\n");
		}
	}
	return 0;
}

INTERP_CMD(helpcmd, "help", cmd_help, "help        ?<cmdname>?");

void
Interp_RegisterCmd(Cmd * cmd)
{
	cmd->next = cmdHead;
	cmdHead = cmd;
}

static void
Interp_ExecuteScript(void *eventData)
{
	char linebuf[40];
	char *line = NULL;
	Interp *interp = eventData;
	line = f_gets(linebuf, sizeof(linebuf), &interp->file);
	if (line) {
		if (line[0] != '#') {
			Interp_Feed(eventData, line);
		}
		EV_Trigger(&interp->nextLineEvent);
	} else {
		f_close(&interp->file);
	}
}

/**
 ********************************************************************************
 * \fn void Interp_StartScript(Interp * interp, char *path)
 ********************************************************************************
 */
bool
Interp_StartScript(Interp * interp, char *path)
{
	FRESULT res;
	res = f_open(&interp->file, path, FA_READ);
	if (res == FR_OK) {
		EV_Trigger(&interp->nextLineEvent);
		return true;
	}
	return false;
}

static int8_t
cmd_script(Interp * interp, uint8_t argc, char *argv[])
{
	if(argc > 1) {
		if(Interp_StartScript(interp,argv[1]) != true) {
			Con_Printf("Failed to start script %s\n",argv[1]);
		}
		return 0;
	} else {
		return -EC_BADNUMARGS;
	}
}

INTERP_CMD(cmdScript, "script", cmd_script, "script        <filename>");
/**
 ***************************************************************************
 * \fn Interp *Interp_Init(Interp_OutProc * OutStr, Interp_Printf* Printf_P)
 * Initialize the Interpreter and set the destination of its output.
 ***************************************************************************
 */
Interp *
Interp_Init(Interp_OutProc * OutStr, Interp_PrintVA * PrintVA_P)
{
	Interp *interp = &g_interp;
	interp->OutStr = OutStr;
	interp->PrintVA_P = PrintVA_P;
	Interp_RegisterCmd(&helpcmd);
	Interp_RegisterCmd(&cmdScript);
	EV_Init(&interp->nextLineEvent, Interp_ExecuteScript, interp);
	return interp;
}
