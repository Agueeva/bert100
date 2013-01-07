/*
 ********************************************************************************
 * (C) 2009 Jochen Karrer
 *//**
 * \file editor.h
 * ANSI/VT100 Commandline Editor with history.
 * Taken from the freeware embedded system simulator "softgun".
 ********************************************************************************
 */

#include "console.h"
#include "twislave.h"
#define Editor_OutPStr(x) Con_Printf_P((x))
#define Editor_OutStr(x)  Con_OutStr((x))
#define Editor_OutChar(x) Con_OutChar((x))
typedef void Editor_LineSinkProc(void *clientData, char *line);
void Editor_Init(Editor_LineSinkProc *, void *clientData);
