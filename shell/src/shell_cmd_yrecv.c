/*=========================================================================

  picolua

  shell/shell_cmd_yrecv.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#include <stdio.h> 
#include <getopt.h> 
#include "shell/shell.h" 
#include <klib/defs.h> 
#include <interface/interface.h>
#include <storage/storage.h>
#include <ymodem/ymodem.h>
#include <config.h>
#include "shell/errcodes.h"
#include "shell/shell.h"
#include "shell/shell_commands.h"

/*=========================================================================

  shell_cmd_yrecv_usage

=========================================================================*/
static void shell_cmd_yrecv_usage (void)
  {
  interface_write_stringln ("Usage: yrecv [file]");
  }

/*=========================================================================

  shell_cmd_yrecv_do_recv

=========================================================================*/
static ErrCode shell_cmd_yrecv_do_receive (const char *filename)
  {
  ErrCode ret = 0;
  YmodemErr err = ymodem_receive (filename, XMODEM_MAX); 
  if (err != 0)
    {
    interface_write_stringln (ymodem_strerror (err));
    ret = ERR_YMODEM;
    }
  return ret;
  }

/*=========================================================================

  shell_cmd_yrecv

=========================================================================*/
ErrCode shell_cmd_yrecv (int argc, char **argv)
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
	shell_cmd_yrecv_usage();
        ret = ERR_USAGE;
      }
    }

  if (ret == 0)
    {
    if (optind == argc)
      {
      ret = shell_cmd_yrecv_do_receive (NULL);
      }
    else if (argc - optind == 1)
      {
      // One arg
      ret = shell_cmd_yrecv_do_receive (argv[optind]);
      }
    else 
      {
      shell_cmd_yrecv_usage();
      ret = ERR_USAGE;
      }
    }

  if (usage) ret = 0;
  return ret;
  }




