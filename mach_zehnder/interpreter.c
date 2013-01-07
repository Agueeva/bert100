/*
 *************************************************************
 * (C) 2010 Jochen Karrer
 *//**
 * \file interpreter.c
 * Command interpreter for a serial console.
 *************************************************************
 */
#include <stdint.h>
#include <string.h>
#include <alloca.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include "interpreter.h"

Interp g_interp;

/**
 ****************************************************
 * \fn static uint8_t split_args(char *script, char *argv[])
 * Split a commandline into arguments by substituting
 * spaces with 0 and seting up an argument vector. 
 * Be careful because the string and the
 * argument vector are modified in place !  
 * \param script Pointer to a command line. Will be modified in place.
 * \param argv Pointer to an argument vector. Will be modified in place.
 * \return Number of arguments detected.
 ****************************************************
 */
static uint8_t split_args(char *script, char *argv[])
{
	uint8_t i, argc = 0;
	uint8_t state = 1;
	argv[0] = script;
	for (i = 0; script[i]; i++) {
		if ((state == 1) && (script[i] == ' ')) {
			continue;
		}
		if (state == 1) {
			state = 0;
			argv[argc++] = script + i;
			if (argc >= INTERP_MAX_ARGC) {
				return argc;
			}
			continue;
		}
		if ((state == 0) && (script[i] == ' ')) {
			script[i] = 0;
			state = 1;
		}
	}
	return argc;
}

/*
 ***************************************************************
 * \fn InterpCmdProc *find_cmd_proc(Interp * interp, char *cmdname)
 * Find a command procedure by a linear search in a linker section
 * reserved for commands. 
 * \param interp The interpreter.
 * \param cmdname The command which will be searched.
 * \return Pointer the the command procedure or NULL if not found.
 ***************************************************************
 */
InterpCmdProc *find_cmd_proc(Interp * interp, char *cmdname)
{
	const Cmd *cursor;
	const char *cursorname;
	InterpCmdProc *proc = NULL;
	for (cursor = &_progmem_cmds_start; cursor < &_progmem_cmds_end;
	     cursor++) {
		cursorname = (const char *)pgm_read_word(&cursor->name);
		if (strcmp_P(cmdname, cursorname) == 0) {
			proc =
			    (InterpCmdProc *) pgm_read_word(&cursor->cmdProc);
			break;
		}
	}
	return proc;
}

/**
 ************************************************************
 * \fn static void Interp_PrintError(Interp * interp, int8_t ec)
 * Translate an errorcode in a readable error message 
 * \param interp Interpreter.
 * \param ec The error code.
 ************************************************************
 */
static void Interp_PrintError(Interp * interp, int8_t ec)
{
	switch (ec) {
	case -EC_ERROR:
		//Interp_OutPStr(interp,"Unspecified error\n");
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
		Interp_Printf_P(interp, "Bad argument\n");
		break;
	}
}

/**
 *************************************************************************
 * \fn void Interp_Feed(void *clientData, char *cmdline)
 * Feed the Interpreter with lines for interpretation. Called for
 * example by the commandline editor whenever a line of input is complete.
 *************************************************************************
 */
void Interp_Feed(void *clientData, char *cmdline)
{
	int8_t result;
	char *argbuf = alloca(strlen(cmdline) + 1);
	InterpCmdProc *cmdProc;
	Interp *interp = (Interp *) clientData;
	strcpy(argbuf,cmdline);
	interp->argc = split_args(argbuf, interp->argv);
	if (interp->argc == 0) {
		return;
	}
	cmdProc = find_cmd_proc(interp, interp->argv[0]);
	if (cmdProc == NULL) {
		Interp_OutStr(interp, interp->argv[0]);
		Interp_Printf_P(interp, ": Command not found\n");
	} else {
		result = cmdProc(interp, interp->argc, interp->argv);
		if (result < 0) {
			Interp_PrintError(interp, result);
		}
	}
}

/**
 *************************************************************************
 * \fn Interp *Interp_Init(Interp_OutProc * OutStr, Interp_Printf* Printf_P)
 * Initialize the Interpreter and set the destination of its output.
 *************************************************************************
 */
Interp *Interp_Init(Interp_OutProc * OutStr, Interp_Printf* Printf_P)
{
	Interp *interp = &g_interp;
	interp->OutStr = OutStr;
	interp->Printf_P = Printf_P;
	return interp;
}

/**
 *************************************************************************
 * \fn static int8_t cmd_help(Interp * interp, uint8_t argc, char *argv[])
 * Print a help text for a single command. If no command name is given
 * print the help of all commands. This command walks through the
 * command section of a reserved linker section. 
 *************************************************************************
 */

static int8_t cmd_help(Interp * interp, uint8_t argc, char *argv[])
{
	const Cmd *cursor;
	const char *cursorname;
	const char *help;
	for (cursor = &_progmem_cmds_start; cursor < &_progmem_cmds_end;
	     cursor++) {
		cursorname = (const char *)pgm_read_word(&cursor->name);
		if ((argc < 2) || (strcmp_P(argv[1], cursorname) == 0)) {
			help = (const char *)pgm_read_word(&cursor->help);
			Interp_Printf_P(interp, "    ");
			interp->Printf_P((char *)help);
			Interp_Printf_P(interp, "\n");
		}
	}
	return 0;
}

INTERP_CMD(help, cmd_help, "help        ?<cmdname>?")
