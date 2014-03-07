/*
 ********************************************************************************
 * (C) 2009 Jochen Karrer
    *//**
 * \file editor.c
 * ANSI/VT100 Commandline Editor with history.
 * Taken from the freeware embedded system simulator "softgun".
 ********************************************************************************
 */
#include <stdlib.h>
#include <ctype.h>
#include "types.h"
#include "console.h"
#include <string.h>
#include "editor.h"
#include "version.h"

static char g_prompt[20] = "Ernie > ";
#define HISTORY_LINES	(10)
#define PROMPT	g_prompt

/*
 * Use only control sequences which are the same for ANSI and VT100
 * and most other terminals.
 */
#define ANSI_up         "\033[A"
#define ANSI_down       "\033[B"
#define ANSI_left       "\033[D"
#define ANSI_right      "\033[C"
#define ANSI_delchar    "\033[P"
#define ANSI_dellineend "\033[K"
#define ANSI_insertchar "\033[@"
#define ANSI_keydel     "\033[3~"
#define ANSI_pgup       "\033[5~"
#define ANSI_pgdown     "\033[6~"
#define ANSI_newline    "\033E"

#define ANSI_insertmode "\033[4h"
#define ANSI_replacementmode "\033[4l"

#define ED_STATE_IDLE           (0)
#define ED_STATE_ESC            (1)
#define ED_STATE_ESC_5B         (2)
#define ED_STATE_ESC_5B_33      (3)
#define ED_STATE_ESC_5B_35      (4)
#define ED_STATE_ESC_5B_36      (5)
#define ED_STATE_NL        	(6)
#define ED_STATE_CR        	(7)

typedef struct Line {
	char data[EDITOR_MAX_CMDLINE_LEN];
	uint8_t len;
} Line;

struct Editor {
	int8_t state;
	uint8_t insertmode;
	uint8_t cursor;
	uint8_t current_line;	/* The currently edited line */
	uint8_t last_line;	/* The line at the end of the circular buffer */
	Editor_LineSinkProc *LineSinkProc;
	void *LineSinkData;
	Line lines[HISTORY_LINES];
};

Editor g_editor;

/**
 ******************************************************************************
 * \fn static void editor_submit_line(Editor * ed);
 * Called whenever a line is completed with <ENTER>. The line is moved to
 * history and submitted to a data sink (for example a Command Interpreter)
 ******************************************************************************
 */
static void
editor_submit_line(Editor * ed)
{
	Line *curr, *prev;
	curr = &ed->lines[ed->current_line];
	prev = &ed->lines[(HISTORY_LINES + ed->last_line - 1) % HISTORY_LINES];
	if (curr->len > 0) {
		if (ed->LineSinkProc) {
			ed->LineSinkProc(ed->LineSinkData, curr->data);
		}
		/* 
		 **************************************************************
		 * Copy current line into last line because we want to 
		 * have it as the last in history
		 **************************************************************
		 */
		if (ed->current_line != ed->last_line) {
			Line *last;
			last = &ed->lines[ed->last_line];
			memcpy(last->data, curr->data, curr->len + 1);
			last->len = curr->len;
		}
		/* Don't store if line is identical to previous */
		if (strcmp(curr->data, prev->data) != 0) {
			ed->last_line = (ed->last_line + 1) % HISTORY_LINES;
		}
	}
	ed->current_line = ed->last_line;
	ed->cursor = 0;
	curr = &ed->lines[ed->current_line];
	curr->len = 0;
	curr->data[0] = 0;
}

/**
 ***********************************************************************
 * \fn static void editor_insert(Editor * ed, uint8_t c);
 * Insert a character in the current line.
 ***********************************************************************
 */
static void
editor_insert(Editor * ed, uint8_t c)
{
	Line *line = &ed->lines[ed->current_line];
	uint8_t i;
	if (line->len + 1 >= EDITOR_MAX_CMDLINE_LEN) {
		/* ignore it */
		return;
	}
	for (i = line->len; i > ed->cursor; i--) {
		line->data[i] = line->data[i - 1];
	}
	line->data[ed->cursor] = c;
	line->len++;
	ed->cursor++;
	line->data[line->len] = 0;
	/* Do this only if echo is enabled */
	if (!ed->insertmode) {
		Editor_OutPStr(ANSI_insertmode);
		ed->insertmode = 1;
	}
	Editor_OutChar(c);

}

/*
 **********************************************************************
 * \fn static inline void editor_backspace(Editor * ed);
 * Remove a character before the cursor and move the cursor one 
 * character to the left.
 **********************************************************************
 */
INLINE void
editor_backspace(Editor * ed)
{

	Line *line = &ed->lines[ed->current_line];
	int8_t i;
	if ((ed->cursor > 0) && (line->len > 0)) {
		line->len--;
		ed->cursor--;
		for (i = ed->cursor; i < line->len; i++) {
			line->data[i] = line->data[i + 1];
		}
		line->data[line->len] = 0;
		/* Do this only if echo is enabled */
		Editor_OutPStr(ANSI_left);
		Editor_OutPStr(ANSI_delchar);
	}
}

/**
 **********************************************************************
 * \fn static INLINE void editor_del(Editor * ed);
 * Remove the character at the cursorposition
 **********************************************************************
 */
INLINE void
editor_del(Editor * ed)
{

	Line *line = &ed->lines[ed->current_line];
	int8_t i;
	if (line->len > ed->cursor) {
		line->len--;
		for (i = ed->cursor; i < line->len; i++) {
			line->data[i] = line->data[i + 1];
		}
		line->data[line->len] = 0;
		/* Do this only if echo is enabled */
		Editor_OutPStr(ANSI_delchar);
	}
}

/**
 **********************************************************************
 * \fn static INLINE void editor_goto_x(Editor * ed, uint8_t pos)
 * Set the cursor to a given position.
 **********************************************************************
 */
INLINE void
editor_goto_x(Editor * ed, uint8_t pos)
{
	while (ed->cursor > pos) {
		ed->cursor--;
		Editor_OutPStr(ANSI_left);
	}
	while (ed->cursor < pos) {
		ed->cursor++;
		Editor_OutPStr(ANSI_right);
	}
}

/**
 **************************************************
 * \fn static void editor_up(Editor * ed);
 * Move one entry up in history. Done only if the
 * entry is not empty.
 **************************************************
 */
static void
editor_up(Editor * ed)
{
	Line *line;
	uint8_t new_current;
	new_current = (HISTORY_LINES + ed->current_line - 1) % HISTORY_LINES;
	line = &ed->lines[new_current];
	if (line->len == 0) {
		return;
	}
	editor_goto_x(ed, 0);
	Editor_OutPStr(ANSI_dellineend);
	ed->current_line = new_current;
	if (line->len) {
		Editor_OutStr(line->data);
	}
	ed->cursor = line->len;
}

/**
 **************************************************************************************
 * \fn static void editor_redraw(Editor *ed)
 * Redraw the current line.
 **************************************************************************************
 */
static void
editor_redraw(Editor * ed)
{
	Line *line;
	unsigned int i;
	uint8_t pos = ed->cursor;
	editor_goto_x(ed, 0);
	for (i = 0; i < strlen(PROMPT); i++) {
		Editor_OutPStr(ANSI_left);
	}
	Editor_OutPStr(ANSI_dellineend);
	Editor_OutPStr(PROMPT);
	line = &ed->lines[ed->current_line];
	if (line->len) {
		Editor_OutStr(line->data);
	}
	ed->cursor = line->len;
	editor_goto_x(ed, pos);
}

/**
 ******************************************************************
 * \fn static void editor_down(Editor * ed);
 * Move one entry down in history. Done only if the entry is not 
 * the last one. 
 ******************************************************************
 */
static void
editor_down(Editor * ed)
{
	Line *line;
	if (ed->current_line == ed->last_line) {
		return;
	}
	editor_goto_x(ed, 0);
	Editor_OutPStr(ANSI_dellineend);
	ed->current_line = (ed->current_line + 1) % HISTORY_LINES;
	line = &ed->lines[ed->current_line];
	if (line->len) {
		Editor_OutStr(line->data);
	}
	ed->cursor = line->len;

}

/**
 ************************************************************************
 * \fn static inline void feed_editor(void *clientData, uint8_t c)
 * Main state machine of the editor. This function is
 * feed by an ANSI/VT100 Terminal char by char.
 ************************************************************************
 */

void
Editor_Feed(void *clientData, uint8_t c)
{
	Editor *ed = (Editor *) clientData;
	Line *line = &ed->lines[ed->current_line];

	switch (ed->state) {
	    case ED_STATE_CR:
	    case ED_STATE_NL:
		    if ((ed->state == ED_STATE_CR) && (c == '\n')) {
			    ed->state = ED_STATE_IDLE;
			    break;
		    }
		    if ((ed->state == ED_STATE_NL) && (c == '\r')) {
			    ed->state = ED_STATE_IDLE;
			    break;
		    }
		    ed->state = ED_STATE_IDLE;
	    case ED_STATE_IDLE:
		    /* Swallow UTF-8 multibyte start characters for now */
		    if (c > 0x7f) {
			    break;
		    }
		    switch (c) {
			case '\t':
				break;
			case '\033':
				ed->state = ED_STATE_ESC;
				break;
			case '\n':
			case '\r':
				if (c == '\n') {
					ed->state = ED_STATE_NL;
				} else {
					ed->state = ED_STATE_CR;
				}
				Editor_OutPStr(ANSI_replacementmode);
				ed->insertmode = 0;
				Editor_OutPStr("\n");
				editor_submit_line(ed);
				Editor_OutPStr(PROMPT);
				/* Feed Line to Interpreter here */
				break;

			case 0x02:	/* ASCII_STX */
			case 0x0c:	/* Form Feed ^L taken from vi */
				editor_redraw(ed);
				break;
			case 0x08:
			case 0x7f:
				editor_backspace(ed);
				break;

			default:
				if(isprint(c)) {
					editor_insert(ed, c);
				}
		    }
		    break;

	    case ED_STATE_ESC:
		    if (c == '[') {
			    ed->state = ED_STATE_ESC_5B;
		    } else if (c == 0x7f) {
			    editor_del(ed);
			    ed->state = ED_STATE_IDLE;
		    } else {
			    ed->state = ED_STATE_IDLE;
		    }
		    break;

	    case ED_STATE_ESC_5B:
		    if (c == 'A') {
			    editor_up(ed);
		    } else if (c == 'B') {
			    editor_down(ed);
		    } else if (c == 'C') {
			    /* RIGHT */
			    if (ed->cursor < line->len) {
				    Editor_OutPStr(ANSI_right);
				    ed->cursor++;
			    }
		    } else if (c == 'D') {
			    /* LEFT */
			    if (ed->cursor > 0) {
				    //Editor_OutPStr(ANSI_insertmode);
				    Editor_OutPStr(ANSI_left);
				    ed->cursor--;
			    }
		    } else if (c == 'H') {
			    editor_goto_x(ed, 0);
		    } else if (c == 'F') {
			    editor_goto_x(ed, line->len);
		    } else if (c == '3') {
			    ed->state = ED_STATE_ESC_5B_33;
			    break;
		    } else if (c == '5') {
			    ed->state = ED_STATE_ESC_5B_35;
			    break;
		    } else if (c == '6') {
			    ed->state = ED_STATE_ESC_5B_36;
			    break;
		    } else {
			    /* Unknown Escape sequence */
		    }
		    ed->state = ED_STATE_IDLE;
		    break;

	    case ED_STATE_ESC_5B_33:
		    if (c == '~') {
			    /* DEL */
			    editor_del(ed);
		    } else {
			    /* Unknown */
		    }
		    ed->state = ED_STATE_IDLE;
		    break;

	    case ED_STATE_ESC_5B_35:
		    if (c == '~') {
			    /* PGUP */
		    } else {
			    /* Unknown */
		    }
		    ed->state = ED_STATE_IDLE;
		    break;

	    case ED_STATE_ESC_5B_36:
		    if (c == '~') {
			    /* PGDOWN */
		    } else {
			    /* Unknown */
		    }
		    ed->state = ED_STATE_IDLE;
		    break;
	    default:
		    /* Bug: Editor in unknown state */
		    break;
	}
	return;
}

/**
 ***************************************************************************
 * May be invoked before the Editor is initialized !
 ****************************************************************************
 */
void
Editor_SetPrompt(const char *str)
{
	if (strlen(str) >= sizeof(g_prompt)) {
		return;
	} else {
		strcpy(g_prompt, str);
	}
}

/**
 ****************************************************************************************************
 * \fn void Editor_Init(Editor_LineSinkProc * LineSinkProc, void *LineSinkData);
 * Initialize the command line editor. Set a data sink and a data source.
 * \param LineSinkProc Pointer to a function handling the complete line when enter is pressed.
 * \param LineSinkData Pointer to the argument of the line sink proc.
 ****************************************************************************************************
 */

Editor *
Editor_Init(Editor_LineSinkProc * LineSinkProc, void *LineSinkData)
{
	Editor *ed;
	ed = &g_editor;
	memset(ed, 0, sizeof(Editor));
	ed->last_line = ed->current_line = 0;
	ed->state = ED_STATE_IDLE;
	Editor_OutPStr("\r\nWelcome to Bert100 Version ");
	Con_Printf(" %s\n", System_GetVersion());
	////Con_Printf(" %s \n", g_CompileTime);
	//Con_Printf("Version %s\n", g_Version);
	Editor_OutPStr(PROMPT);
	ed->LineSinkProc = LineSinkProc;
	ed->LineSinkData = LineSinkData;
	return ed;
}
