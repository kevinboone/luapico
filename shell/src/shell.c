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
#include <getopt.h> 
#include <fnmatch.h> 
#include <libluapico/libluapico.h>
#include "shell/shell.h" 
#include "pico/stdlib.h" 
#include "pico/binary_info.h" 
#include <lua/lualib.h>
#include <lua/lauxlib.h>
#include <klib/defs.h> 
#include <klib/string.h> 
#include <interface/interface.h>
#include <interface/compat.h>
#include <klib/term.h> 
#include <storage/storage.h>
#include <config.h>
#include <bute2/bute2.h>
#include "shell/errcodes.h"
#include "shell/shell_commands.h"

#define SHELL_RC_FILE "/etc/shellrc.sh"
#define LUA_RC_FILE "/etc/luarc.lua"

extern char *file_bin_blink_lua;
extern char *file_etc_luarc_lua;
extern char *file_etc_shellrc_sh;

extern int lua_main (int argc, char **argv);

BOOL interrupted = FALSE;
lua_State *global_L = NULL;

ErrCode shell_do_line (const char *buff); // FWD

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
    case ERR_BADCOMMAND: return "Bad command"; // ..or filename 
    case ERR_USAGE: return "Bad command usage"; 
    case ERR_YMODEM: return "YModem error";  // Receive or transmit
    case ERR_INTERRUPTED: return "Interrupted";  // Ctrl+C 
    case ERR_NOTIMPLEMENTED: return "Feature not implemented";  
    case ERR_BADPIN: return "Bad pin number";  
    case ERR_NOTEXECUTABLE: return "Not executable";  
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

  shell_write_error

=========================================================================*/
void shell_write_error_filename (ErrCode err, const char *filename)
  {
  interface_write_string (filename);
  interface_write_string (": ");
  interface_write_string (shell_strerror (err));
  interface_write_endl();
  }

/*=========================================================================

  shell_get_line

=========================================================================*/
BOOL shell_get_line (char *buff, int len)
  {
  return term_get_line (buff, len, &interrupted, 
     0, NULL);
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

  shell_runlua

  This funtion is called by the screen editor, when invoked from the
  shell, to run a Lua program. It creates a new Lua context for the
  duration of execution. 

=========================================================================*/
extern void shell_runlua (const char *filename)
  {
  BOOL did_init_lua = FALSE;
  if (!global_L)
    {
    global_L = luaL_newstate();  
    luaL_openlibs (global_L);  
    did_init_lua = TRUE;
    }
  lua_getglobal (global_L, "dofile");
  lua_pushstring (global_L, filename);
  if (lua_pcall (global_L, 1, 0, 0) != 0)
    {
    interface_write_string (lua_tostring (global_L, -1));
    interface_write_endl();
    }
  if (did_init_lua)
    {
    lua_close (global_L);
    global_L = NULL;
    }
  }

/*=========================================================================

  shell_cmd_edit

=========================================================================*/
void shell_cmd_edit (int argc, char **argv)
  {
  if (argc >= 2)
    bute_run (argv[1]);
  else
    bute_run (NULL);
  }

/*=========================================================================

  shell_run_lua_main

  The function wraps a call to lua_main, which expects to be called
  with convention argc/argv. It is used when invoking Lua directly as
  a filename. The function essentially shifts argv up one place, and
  inserts "lua" as argv[0].

=========================================================================*/
ErrCode shell_run_lua_main (const char *path, int argc, char **argv)
  {
  ErrCode ret = 0;

  char **newargv = malloc ((argc + 2) * sizeof (char *));
  int newargc = argc+1;
  if (argv)
    {
    newargv[0] = "lua";
    newargv[1] = (char*)path; // Nasty -- should dup
    for (int i = 1; i < argc; i++)
      {
      newargv[i + 1] = argv[i]; 
      }
    newargv[newargc] = NULL;

    ret = lua_main (newargc, newargv);
 
    free (newargv);
    }
  else
    {
    ret = ERR_NOMEM;
    shell_write_error (ret);
    }
  return ret;
  }

/*=========================================================================

  shell_run_script

  This is a very crude, line-by-line, approach to scripts

=========================================================================*/
ErrCode shell_run_script (const char *mypath, int argc, char **argv)
  {
  (void)argc; (void)argv;
  int n; 
  char *buff;
  ErrCode ret = storage_read_file (mypath, (uint8_t **)&buff, &n);
   
  if (ret == 0)
    {
    char *saveptr;
    buff = realloc (buff, n + 1);
    buff[n] = 0;
    char *line = strtok_r (buff, "\n", &saveptr);
    while (line && (ret == 0))
      {
      ret = shell_do_line (line);
      line = strtok_r (NULL, "\n", &saveptr);
      }
    free (buff);
    }
  else
    {
    shell_write_error_filename (ret, mypath);
    }
  return ret;
  }

/*=========================================================================

  shell_find_and_execute_try

=========================================================================*/
static ErrCode shell_find_and_execute_try (const char *dir, const char*cmd, 
          const char *suffix, int argc, char **argv) 
  {
  ErrCode ret = ERR_BADCOMMAND;
  char mypath[MAX_PATH + 1];
  storage_join_path (dir, cmd, mypath);
  strcat (mypath, suffix);
  if (storage_file_exists (mypath))
    {
    const char *e = strrchr (mypath, '.');
    if (e)
      {
      if (strcmp (e, ".lua") == 0)
        {
        shell_run_lua_main (mypath, argc, argv);
        }
      else if (strcmp (e, ".sh") == 0)
        {
        shell_run_script (mypath, argc, argv);
        }
      else 
        {
        ret = ERR_NOTEXECUTABLE;
        shell_write_error_filename (ret, mypath);
        }
      } 
    ret = 0; 
    }
  return ret;
  }

/*=========================================================================

  shell_find_and_execute

=========================================================================*/
static ErrCode shell_find_and_execute (int argc, char **argv)
  {
  ErrCode ret = ERR_BADCOMMAND;
  const char *path = getenv("PATH");
  const char *cmd = argv[0];
  char mypath[MAX_PATH + 1];
  if (path)
    strncpy (mypath, path, MAX_PATH);
  else
    strncpy (mypath, ".", MAX_PATH);

  // Try the complete filename, without path or suffix
  ret = shell_find_and_execute_try ("", cmd, "", argc, argv);
  if (ret == ERR_BADCOMMAND) 
    { 
    char *saveptr;
    char *s = strtok_r (mypath, ":", &saveptr);
    while (s && (ret == ERR_BADCOMMAND))
      {
      ret = shell_find_and_execute_try (s, cmd, "", argc, argv);
      if (ret == ERR_BADCOMMAND) 
        ret = shell_find_and_execute_try (s, cmd, ".lua", argc, argv);
      if (ret == ERR_BADCOMMAND) 
        ret = shell_find_and_execute_try (s, cmd, ".sh", argc, argv);
      s = strtok_r (NULL, ":", &saveptr);
      }
    }

  if (ret == ERR_BADCOMMAND)
    shell_write_error_filename (ERR_BADCOMMAND, argv[0]); 

  return ret;
  }

/*=========================================================================

  shell_do_variable

=========================================================================*/
void shell_do_variable (const char *expr)
  {
  char working [READLINE_MAXINPUT + 1];
  strncpy (working, expr, READLINE_MAXINPUT);
  char *name = working;
  char *p = strchr (name, '=');
  if (!p) return; // Should never happen -- caller has checked
  *p = 0;
  const char *value = p+1;
 
  if (value[0] == 0)
    {
    unsetenv (name);
    }
  else
    {
    setenv (name, value, TRUE);
    }
  }

/*=========================================================================

  shell_do_line_argv

=========================================================================*/
ErrCode shell_do_line_argv (int argc, char **argv)
  {
  ErrCode ret = 0;
  if (argc == 0) return 0;

  if (argc == 1 && strchr (argv[0], '='))
    { 
    shell_do_variable (argv[0]);
    }
  else if (strcmp (argv[0], "lua") == 0)
    lua_main (argc, argv);
  else if (strcmp (argv[0], "edit") == 0)
    shell_cmd_edit (argc, argv);
  else if (strcmp (argv[0], "df") == 0)
    ret = shell_cmd_df (argc, argv);
  else if (strcmp (argv[0], "ls") == 0)
    ret = shell_cmd_ls (argc, argv);
  else if (strcmp (argv[0], "mkdir") == 0)
    ret = shell_cmd_mkdir (argc, argv);
  else if (strcmp (argv[0], "rmdir") == 0)
    ret = shell_cmd_rm (argc, argv);
  else if (strcmp (argv[0], "rm") == 0)
    ret = shell_cmd_rm (argc, argv);
  else if (strcmp (argv[0], "echo") == 0)
    ret = shell_cmd_echo (argc, argv);
  else if (strcmp (argv[0], "cat") == 0)
    ret = shell_cmd_cat (argc, argv);
  else if (strcmp (argv[0], "yrecv") == 0)
    ret = shell_cmd_yrecv (argc, argv);
  else if (strcmp (argv[0], "ysend") == 0)
    ret = shell_cmd_ysend (argc, argv);
  else if (strcmp (argv[0], "cp") == 0)
    ret = shell_cmd_cp (argc, argv);
  else if (strcmp (argv[0], "mv") == 0)
    ret = shell_cmd_cp (argc, argv);
  else if (strcmp (argv[0], "format") == 0)
    ret = shell_cmd_format (argc, argv);
  else if (strcmp (argv[0], "i2cdetect") == 0)
    ret = shell_cmd_i2cdetect (argc, argv);
  else 
    ret = shell_find_and_execute (argc, argv);
    
  return ret;
  }

/*=========================================================================

  shell_do_line

=========================================================================*/
ErrCode shell_do_line (const char *buff)
  { 
  ErrCode ret = 0;
  String *sbuff = string_create (buff);
  List *args = string_tokenize (sbuff);
  int l = list_length (args);  
  char **argv = malloc ((l + 1) * sizeof (char *));

  if (argv)
    {
    for (int i = 0; i < l; i++)
      {
      const String *arg = list_get (args, i);
      argv[i] = strdup (string_cstr (arg));
      }
    argv[l] = NULL;

    int argc = l;
    ret = shell_do_line_argv (argc, argv);

    for (int i = 0; i < l; i++)
      {
      free (argv[i]);
      }

    free (argv);
    } 
  else
    {
    shell_write_error (ERR_NOMEM);
    }

  list_destroy (args);
  string_destroy (sbuff);
  return ret;
  }

/*=========================================================================

  shell_glob_match

=========================================================================*/
BOOL shell_glob_match (const char *filename, const char *match)
  {
  return !my_fnmatch (match, filename, 0);
  }

/*=========================================================================

  shell_globber

=========================================================================*/
static void shell_globber  (String *token, List *list)
  {
  char basename[MAX_FNAME + 1];
  char dir[MAX_PATH + 1];
  const char *c_token = string_cstr (token);
  if (!(strchr (c_token, '*') || strchr (c_token, '?')))
    {
    list_append (list, token);
    return;
    }

  storage_get_basename (c_token, basename);
  storage_get_dir (string_cstr (token), dir);
  //printf ("basename=%s\n", basename);
  //printf ("dir=%s\n", dir);
  BOOL matched = FALSE;

  List *list2 = list_create (free);
  if (list2)
    {
    if (storage_list_dir (dir, list2) == 0)
      {
      int l = list_length (list2);
      for (int i = 0; i < l; i++)
        {
        const char *fname = list_get (list2, i);
	if (fname[0] == '.') continue;
        if (shell_glob_match (fname, basename))
          {
          char newpath[MAX_PATH + 1];
	  storage_join_path (dir, fname, newpath);
          list_append (list, string_create (newpath)); 
	  matched = TRUE;
          }
	}
      }
    else
      {
      // Silently swallow error
      }
    list_destroy (list2);
    }
  else
    {
    // Silently swallow error
    }
  
  if (matched)
    string_destroy (token);
  else
    list_append (list, token);
  }

/*=========================================================================

  shell_init_environment

=========================================================================*/
void shell_init_environment (void)
  {
  setenv ("PATH", "/bin", TRUE); // TODO
  setenv ("LUA_INIT", "@/etc/luarc.lua", TRUE); 
  }

/*=========================================================================

  shell_init_storage

  Create the initial directories after formatting.

=========================================================================*/
void shell_init_storage (void)
  {
  storage_mkdir ("/bin");
  storage_mkdir ("/etc");
  storage_mkdir ("/lib");
  storage_write_file ("/bin/blink.lua", file_bin_blink_lua, 
    strlen (file_bin_blink_lua));
  if (!storage_file_exists (LUA_RC_FILE))
    {
    storage_write_file (LUA_RC_FILE, file_etc_luarc_lua, 
      strlen (file_etc_luarc_lua));
    }


  if (!storage_file_exists (SHELL_RC_FILE))
    {
    storage_write_file (SHELL_RC_FILE, file_etc_shellrc_sh, 
      strlen (file_etc_shellrc_sh));
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
  shell_init_environment ();
  shell_init_storage ();

 // while (true)
 //   {
 //   int key = term_get_key ();
 //   printf ("key=%d\n", key);
 //   }
  
  List *history = list_create (free);
  string_tok_globber = shell_globber;

  if (storage_file_exists (SHELL_RC_FILE))
    {
    char *argv[]  = {"picolua"};
    shell_run_script (SHELL_RC_FILE, 1, argv);
    }

  char buff [READLINE_MAXINPUT + 1];
  while (interface_write_buff ("$ ", 2), 
      term_get_line (buff, sizeof (buff), &interrupted, 
      READLINE_MAX_HISTORY, history))
    {
    if (interrupted)
      {
      }
    else
      {
      shell_do_line (buff);
      }
    interrupted = FALSE;
    }

  list_destroy (history);

  storage_cleanup();
  interface_cleanup ();
  }

