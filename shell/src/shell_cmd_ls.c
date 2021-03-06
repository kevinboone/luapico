/*=========================================================================

  picolua

  shell/shell_cmd_ls.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#include <stdio.h> 
#include <getopt.h> 
#include "shell/shell.h" 
#include <klib/defs.h> 
#include <klib/term.h> 
#include <interface/interface.h>
#include <storage/storage.h>
#include <config.h>
#include "shell/errcodes.h"
#include "shell/shell_commands.h"

/*=========================================================================

  shell_cmd_ls

=========================================================================*/
static void shell_cmd_dols (const char *path, const List *list, 
     BOOL lng)
  {
  char result [MAX_PATH + 1];
  char s[20]; // For converting numbers
  int l = list_length (list);
  uint max_name = 0;
  uint32_t max_size = 0;

  for (int i = 0; i < l; i++)
    {
    const char *fname = list_get (list, i);
    uint l = strlen (fname);
    if (l > max_name) max_name = l;
   
    storage_join_path (path, fname, result); 
    FileInfo info;
    if (storage_info (result, &info) == 0)
      {
      if (info.size > max_size) max_size = info.size;
      }
    }

  sprintf (s, "%lu", max_size);
  int sizelen = strlen (s);

  if (lng)
    {
    for (int i = 0; i < l; i++)
      {
      const char *fname = list_get (list, i);
      storage_join_path (path, fname, result); 
      FileInfo info;
      if (storage_info (result, &info) == 0)
	{
	char line [MAX_FNAME + 20];
	char pad[20];
	strcpy (pad, "                   ");
	sprintf (s, "%lu", info.size);
	pad [sizelen - strlen (s)] = 0;
	sprintf (line, "%s %s%s %s", 
	  info.type == STORAGE_TYPE_REG ? "     " : "<dir>", pad, 
	  s, fname);
	interface_write_string (line);
	}
      else
	{
	interface_write_string (fname);
	}
      interface_write_endl();
      }
    }
  else
    {
    uint8_t rows, cols;
    term_get_size (&rows, &cols);
    char *pad = malloc (cols + 1);
    int per_row;
    if (max_name == 0)
      per_row = 50; // Should never happen, but avoid div by zero
    else
      per_row = cols / (max_name + 2);
    per_row--;
    int n = 0;
    for (int i = 0; i < l; i++)
      {
      const char *fname = list_get (list, i);
      for (int j = 0; j < (int)(1 + max_name - strlen (fname)); j++)
        {
        pad[j] = ' ';
        pad[j + 1] = 0;
        }
      interface_write_string (fname);
      interface_write_string (pad);
      if (n++ >= per_row)
        {
        interface_write_endl();
        n = 0;
        }
      }
    free (pad);
    interface_write_endl();
    }
  }

/*=========================================================================

  shell_cmd_lsone

=========================================================================*/
ErrCode shell_cmd_lsone (const char *path, BOOL lng, BOOL show_dir)
  {
  ErrCode ret = 0;
  List *list = list_create (free);
  if (list)
    {
    FileInfo info;
    ret = storage_info (path, &info);
    if (ret == 0)
      {
      if (info.type == STORAGE_TYPE_DIR)
        {
        ret = storage_list_dir (path, list);
        if (ret == 0)
          {
          if (show_dir)
            { 
            interface_write_string (path);
            interface_write_string (":");
            interface_write_endl();
            }
          shell_cmd_dols (path, list, lng);
          }
        else
          {
          shell_write_error_filename (ret, path);
          }
	}
      else
        {
	if (lng)
	  {
	  if (info.type == STORAGE_TYPE_DIR)
	    interface_write_string ("<dir> ");
	  else
	    interface_write_string ("      ");
	  char num[20];
	  sprintf (num, "%5ld ", info.size);
	  interface_write_string (num);
	  }
        interface_write_string (path);
        interface_write_endl();
	}
      }
    else
      shell_write_error_filename (ret, path);
    list_destroy (list);
    }
  else
    {
    shell_write_error (ERR_NOMEM);
    ret = ERR_NOMEM;
    }
  return ret;
  }

/*=========================================================================

  shell_cmd_ls

=========================================================================*/
ErrCode shell_cmd_ls (int argc, char **argv)
  {
  optind = 0;
  ErrCode ret = 0;
  BOOL lng = FALSE;
  int opt;
  while ((opt = getopt (argc, argv, "l")) != -1) 
    {
    switch (opt)
      { 
      case 'l':
        lng = TRUE;
        break;
      default:
        interface_write_string ("Usage: ls [-l]");
        interface_write_endl();
        ret = ERR_USAGE;
      }
    }

  if (ret == 0)
    {
    if (argc - optind == 1)
      {
      // One path arg
      ret = shell_cmd_lsone (argv[optind], lng, FALSE);
      }
    else if (argc - optind > 0)
      {
      // Many path args
      for (int i = optind; i < argc; i++)
        {
        ret = shell_cmd_lsone (argv[i], lng, TRUE);
        }
      }
    else
      {
      // No path args
      ret = shell_cmd_lsone ("", lng, FALSE);
      }
    }
  return ret;
  }


