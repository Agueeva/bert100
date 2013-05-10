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
#ifndef _EDITOR_H
#define _EDITOR_H
#define EDITOR_MAX_CMDLINE_LEN (80)
#define Editor_OutPStr(x) Con_Printf((x))
#define Editor_OutStr(x)  Con_Printf((x))
#define Editor_OutChar(x) Con_OutChar(NULL,(x))
typedef struct Editor Editor;
typedef void Editor_LineSinkProc(void *clientData, char *line);
Editor *Editor_Init(Editor_LineSinkProc *, void *clientData);
void Editor_Feed(void *clientData, uint8_t c);
void Editor_SetPrompt(const char *str);
#endif
