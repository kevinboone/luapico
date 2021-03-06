/*=========================================================================

  picolua

  shell/shell.c

  This file implements the wrapper for the Lua REPL, and provides some
  useful general functions for displaying errors, etc. 

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#pragma once

#include <klib/defs.h>
#include "shell/errcodes.h"

BEGIN_DECLS

/** Convenience function for emiting an error message, followed by
    an EOL */
extern void    shell_write_error (ErrCode err);

/** Convenience function for emiting an error message and a filename, 
    followed by an EOL */
extern void    shell_write_error_filename (ErrCode err, const char *filename);

/** Read a line into the buffer. Returns TRUE unless the user enters
    the end-of-input character (usually ctrl+d). If the user hits
    interrupt, the function returns TRUE, but sets the interrupt flag. 
    Callers are expected to check this flag. Note that 'len' is the
    length of the buffer, not the length of the line. The line will
    be one character smaller, to allow for the string to be 
    zero-terminated*/
extern BOOL    shell_get_line (char *buff, int len);

/** Return a string representation of the error code. In English. */
extern const char *shell_strerror (ErrCode err);

/** Clear the shell interrupt flag. The shell will do this at the start of
   each line that is executed. */
extern void    shell_clear_interrupt (void);

/** Set the shell interrupt flag. This might be called form a terminal keyboard
   handler. */
extern void    shell_set_interrupt (void);

/** Shell starts here. */
extern void    shell_main (void);

/** Returns TRUE if the shell has detected a keyboard interupt (ctrl+c). 
   Many components use this to decide whether to stop running. */
extern BOOL    shell_get_interrupt (void);

/** Run a Lua script in the global Lua context. This is used when 
    running a Lua script from inside the editor. */
extern void    shell_runlua (const char *filename);

/** Run a single line using the shell. This function returns an error code
    but, on the whole, it will have signalled any problems to the console. */
extern ErrCode shell_do_line (const char *buff);

/** After formatting storage, this method creates the basic directories. */
extern void shell_init_storage (void);

END_DECLS

