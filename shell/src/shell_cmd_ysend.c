/*=========================================================================

  picolua

  shell/shell_cmd_ysend.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#include <stdio.h> 
#include <getopt.h> 
#include "shell/shell.h" 
#include "pico/stdlib.h" 
#include <klib/defs.h> 
#include <interface/interface.h>
#include <storage/storage.h>
#include <ymodem/ymodem.h>
#include <config.h>
#include "shell/errcodes.h"
#include "shell/shell.h"
#include "shell/shell_commands.h"

/*=========================================================================

  shell_cmd_ysend_usage

=========================================================================*/
static void shell_cmd_ysend_usage (void)
  {
  interface_write_stringln ("Usage: ysend [file]");
  }

/*=========================================================================

  shell_cmd_ysend_do_send

=========================================================================*/
static ErrCode shell_cmd_ysend_do_send (const char *filename)
  {
  ErrCode ret = 0;
#if PICO_ON_DEVICE
  stdio_set_translate_crlf (&stdio_usb, false);
#endif
  YmodemErr err = ymodem_send (filename); 
#if PICO_ON_DEVICE
  stdio_set_translate_crlf (&stdio_usb, true);
#endif
  if (err != 0)
    {
    interface_write_stringln (ymodem_strerror (err));
    ret = ERR_YMODEM;
    }
  return ret;
  }

/*=========================================================================

  shell_cmd_ysend

=========================================================================*/
ErrCode shell_cmd_ysend (int argc, char **argv)
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
	shell_cmd_ysend_usage();
        ret = ERR_USAGE;
      }
    }

  if (ret == 0)
    {
    if (argc - optind == 1)
      {
      // One arg
      ret = shell_cmd_ysend_do_send (argv[optind]);
      }
    else 
      {
      shell_cmd_ysend_usage();
      ret = ERR_USAGE;
      }
    }

  if (usage) ret = 0;
  return ret;
  }





