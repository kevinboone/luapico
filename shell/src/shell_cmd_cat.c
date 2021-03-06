/*=========================================================================

  picolua

  shell/shell_cmd_cat.c

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

  shell_cmd_cat_usage

=========================================================================*/
static void shell_cmd_cat_usage (void)
  {
  interface_write_stringln ("Usage: cat {files...}");
  }

/*=========================================================================

  shell_cmd_cat

=========================================================================*/
ErrCode shell_cmd_cat (int argc, char **argv)
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
	shell_cmd_cat_usage();
        ret = ERR_USAGE;
      }
    }

  if (ret == 0)
    {
    if (optind == argc)
      {
      shell_cmd_cat_usage ();
      ret = ERR_USAGE;
      }
    else
      {
      for (int i = optind; i < argc && ret == 0; i++)
        {
	uint8_t *buff;
	int len;
	ret = storage_read_file (argv[i], &buff, &len);
	if (ret == 0)
	  {
          interface_write_buff ((char *)buff, len);
	  free (buff);
	  }
	else
	  shell_write_error_filename (ret, argv[i]);
        }
      }
    }

  if (usage) ret = 0;
  return ret;
  }



