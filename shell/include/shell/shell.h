/*=========================================================================

  picolua

  shell/shell.c

  This file implements the wrapper for the Lua REPL, and provides some
  useful general functions for displaying errors, etc. 

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#pragma once

#include <klib/defs.h>
#include <errcodes.h>

BEGIN_DECLS

/** Convenience function for emiting an error message, followed by
    an EOL */
extern void    shell_write_error (ErrCode err);

/** Read a line into the buffer. Returns TRUE unless the user enters
    the end-of-input character (usually ctrl+d). If the user hits
    interrupt, the function returns TRUE, but sets the interrupt flag. 
    Callers are expected to check this flag. Note that 'len' is the
    length of the buffer, not the length of the line. The line will
    be one character smaller, to allow for the string to be 
    zero-terminated*/
extern BOOL    shell_get_line (char *buff, int len);

extern const char *shell_strerror (ErrCode err);

extern void    shell_clear_interrupt (void);
extern void    shell_set_interrupt (void);
extern BOOL    shell_get_interrupt (void);

extern void    shell_main (void);

extern BOOL    shell_get_interrupt (void);

extern void    shell_runlua (const char *filename);

END_DECLS

