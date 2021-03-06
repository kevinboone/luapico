/*=========================================================================

  picolua

  shell/shell_cmd_cp.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#include <stdio.h> 
#include <getopt.h> 
#include <klib/defs.h> 
#include <interface/interface.h>
#include <storage/storage.h>
#include <config.h>
#include "shell/errcodes.h"
#include "shell/shell.h"
#include "shell/shell_commands.h"
#include "shell/fileutil.h"

/*=========================================================================

  shell_cmd_cp_usage

=========================================================================*/
static void shell_cmd_cp_usage (const char *cmd)
  {
  interface_write_string ("Usage: "); 
  interface_write_string (cmd); 
  interface_write_stringln (" [-v] {files...} {file | directory}");
  }

/*=========================================================================

  shell_cmd_cp

=========================================================================*/
ErrCode shell_cmd_cp_or_mv (int argc, char **argv, const char *cmd)
  {
  int opt;
  optind = 0;
  ErrCode ret = 0;
  BOOL usage = FALSE;
  BOOL verbose = FALSE;
  while ((opt = getopt (argc, argv, "hv")) != -1) 
    {
    switch (opt)
      { 
      case 'v':
        verbose = TRUE;
        break;
      case 'h':
        usage = TRUE;
        // Fall through
      default:
	shell_cmd_cp_usage (cmd);
        ret = ERR_USAGE;
      }
    }

  if (ret == 0)
    {
    if (argc - optind >= 2)
      {
      // cp a b c -- c must be a dir
      // cp a b -- a can be a file or dir. If dir, copy to b/a
      const char *raw_target = argv[argc - 1];

      BOOL multi = (argc - optind >= 3); // Multiple files to copy     

      BOOL target_is_dir = FALSE;
      FileInfo info;
      if (storage_info (raw_target, &info) == 0)
        {
        if (info.type == STORAGE_TYPE_DIR)
          target_is_dir = TRUE;
        }

      if (!target_is_dir && multi)
        {
        interface_write_stringln (
           "Last arg must be a dir, when copying > 1 file");
        ret = ERR_USAGE;
        }
      else
        { 
        for (int i = optind; i < argc - 1 && !shell_get_interrupt(); i++)
          {
          char real_target[MAX_PATH + 1];
          const char *source = argv[i];
          if (target_is_dir)
            {
            // When we copy to a directory, we have to strip the dir part
            //   of the source. E.g., cp /bin/blink.lua /foo should 
            //   result in cp /bin/blink.lua /foo/blink.lua, not
            //   /foo/bin/blink.lua 
            char source_basename[MAX_PATH + 1];
            storage_get_basename (source, source_basename);
            storage_join_path (raw_target, source_basename, real_target);
            }
          else
            strncpy (real_target, raw_target, MAX_PATH);
          if (verbose)
            interface_write_stringln (source); 
          if (strcmp (cmd, "cp") == 0)
            ret = fileutil_copy (source, real_target); 
          else
            ret = fileutil_rename (source, real_target);
          }
        }
      }
    else
      {
      shell_cmd_cp_usage (cmd);
      ret = ERR_USAGE;
      }
    }

  if (usage) ret = 0;
  return ret;
  }

/*=========================================================================

  shell_cmd_cp

=========================================================================*/
ErrCode shell_cmd_cp (int argc, char **argv)
  {
  return shell_cmd_cp_or_mv (argc, argv, "cp");
  }

/*=========================================================================

  shell_cmd_mv

=========================================================================*/
ErrCode shell_cmd_mv (int argc, char **argv)
  {
  return shell_cmd_cp_or_mv (argc, argv, "mv");
  }

