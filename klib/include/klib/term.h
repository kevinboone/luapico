/*==========================================================================

  term.h

  Functions for interacting with ANSI-like terminals

  (c)2021 Kevin Boone, GPLv3.0

==========================================================================*/

#pragma once

#include <klib/list.h>

// Key codes for cursor movement, etc
#define VK_BACK     8
#define VK_TAB      9
#define VK_ENTER   10
#define VK_ESC     27
// Note that Linux console/terminal sends DEL when backspace is pressed
#define VK_DEL    127 
#define VK_DOWN  1000
#define VK_UP    1001 
#define VK_LEFT  1002 
#define VK_RIGHT 1003 
#define VK_PGUP  1004 
#define VK_PGDN  1005 
#define VK_HOME  1006 
#define VK_END   1007
#define VK_INS   1008
#define VK_CTRLUP 1009
#define VK_CTRLDOWN 1010
#define VK_CTRLLEFT 1011
#define VK_CTRLRIGHT 1012
#define VK_CTRLHOME 1013
#define VK_CTRLEND 1014
#define VK_SHIFTUP 1020
#define VK_SHIFTDOWN 1021
#define VK_SHIFTLEFT 1022
#define VK_SHIFTRIGHT 1023
#define VK_SHIFTHOME 1024
#define VK_SHIFTEND 1025
#define VK_SHIFTTAB 1026
#define VK_CTRLSHIFTUP 1030
#define VK_CTRLSHIFTDOWN 1031
#define VK_CTRLSHIFTLEFT 1032
#define VK_CTRLSHIFTRIGHT 1033
#define VK_CTRLSHIFTHOME 1034
#define VK_CTRLSHIFTEND 1035
#define VK_INTR 2000 
#define VK_EOI 2001 

BEGIN_DECLS

/** Wait for, and return, a key, without echoing it. */
extern int term_get_key (void);

/** Clear screen, without homing cursor. */
extern void term_clear (void);

/** Clear from cursor to end of line. */
void term_clear_eol (void);

/** Clear screen and move cursor to (0,0) */
extern void term_clear_and_home (void);

/** Get the terminal size. For a pseudo-tty we could get this from 
 * hardware using an ioctl() call. For a real terminal, though, I'm not
 * sure there is a standard way. So in practice we always return 25x80. */
extern void term_get_size (uint8_t *rows, uint8_t *cols);

/** Write a line, truncating to fit width if required. Top line is zero. */
extern void term_write_line (uint8_t row, const char *line, BOOL truncate);

/** Set the cursor position, from top-left (which is 0, 0). */
void term_set_cursor (uint8_t row, uint8_t col);

/** Erase the whole line containing the cursor. */
extern void term_erase_current_line (void);

/** Work out how long a string will take on the display, if placed at
  the specified column, and allowing for tabs. */
extern uint8_t term_get_displayed_length (const char *line, uint8_t col);

/** Read a line into the buffer, using simple line editing. 
    Returns TRUE unless the user enters
    the end-of-input character (usually ctrl+d). If the user hits
    interrupt, the function returns TRUE, but sets the interrupt flag. 
    Callers are expected to check this flag. Note that 'len' is the
    length of the buffer, not the length of the line. The line will
    be one character smaller, to allow for the string to be 
    zero-terminated*/
BOOL term_get_line (char *buff, int len, BOOL *interrupt, 
       int max_history, List *history);

void term_enable (BOOL enable);

void term_show_cursor (void);

void term_hide_cursor (void);

END_DECLS

