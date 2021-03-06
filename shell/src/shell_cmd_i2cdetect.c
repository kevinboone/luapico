/*=========================================================================

  picolua

  shell/shell_cmd_i2cdetect.c

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

  shell_cmd_i2cdetect_usage

=========================================================================*/
static void shell_cmd_i2cdetect_usage (void)
  {
  interface_write_stringln ("Usage: i2cdetect {pin1} {pin2}");
  }

/*=========================================================================

  shell_cmd_i2cdetect

=========================================================================*/
ErrCode shell_cmd_i2cdetect (int argc, char **argv)
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
	shell_cmd_i2cdetect_usage();
        ret = ERR_USAGE;
      }
    }

  if (ret == 0)
    {
    if (optind == argc - 2)
      {
      int pin1 = atoi (argv[optind]);
      int pin2 = atoi (argv[optind + 1]);
      ret = interface_i2cdetect (pin1, pin2);
      }
    else
      {
      shell_cmd_i2cdetect_usage ();
      ret = ERR_USAGE;
      }
    }

  if (usage) ret = 0;
  return ret;
  }




