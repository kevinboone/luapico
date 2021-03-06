/*=========================================================================

  picolua

  shell/shell_cmd_format.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#include <stdio.h> 
#include <getopt.h> 
#include "shell/shell.h" 
#include <klib/defs.h> 
#include <interface/interface.h>
#include <storage/storage.h>
#include <config.h>
#include "shell/errcodes.h"
#include "shell/shell.h"
#include "shell/shell_commands.h"

/*=========================================================================

  shell_cmd_format_usage

=========================================================================*/
static void shell_cmd_format_usage (void)
  {
  interface_write_stringln ("Usage: format [-y] {files...}");
  }

/*=========================================================================

  shell_cmd_format

=========================================================================*/
ErrCode shell_cmd_format (int argc, char **argv)
  {
  int opt;
  optind = 0;
  ErrCode ret = 0;
  BOOL usage = FALSE;
  BOOL yes = FALSE;
  while ((opt = getopt (argc, argv, "hy")) != -1) 
    {
    switch (opt)
      { 
      case 'y':
        yes = TRUE;
        break;
      case 'h':
        usage = TRUE;
        // Fall through
      default:
	shell_cmd_format_usage();
        ret = ERR_USAGE;
      }
    }

  if (ret == 0)
    {
    if (!yes)
      {
      const char *msg = "Erase all data (y/n)? ";
      interface_write_buff (msg, strlen (msg));
      char c = interface_get_char ();
      if (c == 'y' || c == 'Y') yes = TRUE;
      interface_write_endl();
      }
    if (yes)
      {
      ret = storage_format();
      if (ret == 0) 
        shell_init_storage();
      else
        shell_write_error (ret);
      }
    }

  if (usage) ret = 0;
  return ret;
  }




