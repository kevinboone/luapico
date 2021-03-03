/*=========================================================================

  picolua

  shell/shell.c

  This file implements the wrapper for the Lua REPL, and provides some
  useful general functions for displaying errors, etc. 

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <libluapico/libluapico.h>
#include "shell/shell.h" 
#include "pico/stdlib.h" 
#include "hardware/gpio.h" 
#include "pico/binary_info.h" 
#include <lua/lualib.h>
#include <lua/lauxlib.h>
#include <klib/defs.h> 
#include <interface/interface.h>
#include <interface/compat.h>
#include <klib/term.h> 
#include <storage/storage.h>
#include <config.h>
#include <errcodes.h>

extern void lua_main (int argc, char **argv);
BOOL interrupted = FALSE;
List *history; // For the line editor
lua_State *global_L = NULL;

/*=========================================================================

  shell_strerror

=========================================================================*/
const char *shell_strerror (ErrCode err)
  {
  switch (err)
    {
    case ERR_NOMEM: return "Out of memory";
    case ERR_IO: return "I/O error";
    case ERR_CORRUPT: return "Corrupt filesystem";
    case ERR_NOENT: return "No such file or directory";
    case ERR_EXIST: return "Entry already exists";
    case ERR_NOTDIR: return "Expected directory";
    case ERR_ISDIR: return "Expected file, got directory";
    case ERR_NOTEMPTY: return "Directory not empty";
    case ERR_BADF: return "Bad file descriptor";
    case ERR_FBIG: return "File too large";
    case ERR_INVAL: return "Invalid parameter";
    case ERR_NAMETOOLONG: return "Filename too long";
    case ERR_NOSPC: return "No space left on device";
    case ERR_NOATTR: return "No data/attr available";
    case ERR_BADARGS: return "Incorrect arguments"; // ...to a command
    case ERR_LINETOOLONG: return "Line too long";
    case ERR_ABANDONED: return "Operation abandoned"; // .. by user
    }
  return "Unknown error";
  }

/*=========================================================================

  shell_write_error

=========================================================================*/
void shell_write_error (ErrCode err)
  {
  interface_write_string (shell_strerror (err));
  interface_write_endl();
  }

/*=========================================================================

  shell_get_line

=========================================================================*/
BOOL shell_get_line (char *buff, int len)
  {
  return term_get_line (buff, len, &interrupted, 
     READLINE_MAX_HISTORY, history);
  }

/*=========================================================================

  shell_clear_interrupt

=========================================================================*/
void shell_clear_interrupt (void)
  {
  interrupted = FALSE;
  }

/*=========================================================================

  shell_set_interrupt

=========================================================================*/
void shell_set_interrupt (void)
  {
  interrupted = TRUE;
  }

/*=========================================================================

  shell_get_interrupt

=========================================================================*/
BOOL shell_get_interrupt (void)
  {
  if (interface_is_interrupt_key())
    interrupted = TRUE;
  return interrupted;
  }

/*=========================================================================

  shell_main

=========================================================================*/
extern void shell_runlua (const char *filename)
  {
  lua_getglobal (global_L, "dofile");
  lua_pushstring (global_L, filename);
  if (lua_pcall (global_L, 1, 0, 0) != 0)
    {
    interface_write_string (lua_tostring (global_L, -1));
    interface_write_endl();
    }
  }

/*=========================================================================

  shell_main

=========================================================================*/
void shell_main()
  { 
  storage_init (); 
  stdio_init_all();
  interface_init ();

 // while (true)
 //   {
 //   int key = term_get_key ();
 //   printf ("key=%d\n", key);
 //   }
  
  history = list_create (free);

  char *argv[] = {"lua", NULL};
  lua_main (1, argv);

  list_destroy (history);

  storage_cleanup();
  interface_cleanup ();
  }

