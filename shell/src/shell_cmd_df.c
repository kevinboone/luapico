/*=========================================================================

  picolua

  shell/shell_cmd_df.c

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

  shell_cmd_df

=========================================================================*/
ErrCode shell_cmd_df (int argc, char **argv)
  {
  uint32_t used, total;
  int opt;
  optind = 0;
  ErrCode ret = 0;
  BOOL human = FALSE;
  while ((opt = getopt (argc, argv, "k")) != -1) 
    {
    switch (opt)
      { 
      case 'k':
        human = TRUE;
        break;
      default:
        interface_write_stringln ("Usage: df [-k]");
        ret = ERR_USAGE;
      }
    }

  if (ret == 0)
    {
    ErrCode err = storage_df (NULL, &used, &total);
    if (err == 0)
      {
      if (human)
        printf ("Used: %ldk, total %ldk, free: %ldk", used / 1024,  
        total / 1024, 
        (total - used) / 1024);
      else
        printf ("Used: %ld, total %ld, free: %ld", used, total, 
        total - used);
      interface_write_endl();
      }
    else
      { 
      shell_write_error (err);
      }
    }
  return ret;
  }


