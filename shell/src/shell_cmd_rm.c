/*=========================================================================

  picolua

  shell/shell_cmd_rm.c

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
#include "shell/shell_commands.h"

/*=========================================================================

  shell_cmd_rm

=========================================================================*/
ErrCode shell_cmd_rm (int argc, char **argv)
  {
  int opt;
  optind = 0;
  ErrCode ret = 0;
  BOOL usage = FALSE;
  while ((opt = getopt (argc, argv, "h")) != -1) 
    {
    switch (opt)
      { 
      case 'h':
        usage = TRUE;
        // Fall through
      default:
        interface_write_stringln ("Usage: rm {files... dirs...}");
        ret = ERR_USAGE;
      }
    }

  if (ret == 0)
    {
    for (int i = optind; i < argc && ret == 0; i++)
      {
      ret = storage_rm (argv[i]); 
      if (ret) 
        shell_write_error_filename (ret, argv[i]);
      }
    }

  if (usage) ret = 0;
  return ret;
  }


