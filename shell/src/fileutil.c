/*=========================================================================

  picolua

  shell/fileutil.c

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

/*=========================================================================

  fileutil_copy 

=========================================================================*/
ErrCode fileutil_copy (const char *source, const char *target)
  {
  ErrCode ret = 0;

  uint8_t buff[256];

  ret = storage_write_file (target, "", 0); 
  if (ret == 0)
     {
     int offset = 0;
     int n = 0;
     do
       {
       ret = storage_read_partial (source, offset, sizeof (buff), 
               buff, &n); 
       if (ret == 0)
         {
         ret = storage_append_file (target, buff, n);
         if (ret)
           shell_write_error_filename (ret, target);
         }
       else
         shell_write_error_filename (ret, source);

       offset += sizeof (buff);
       } while (n == sizeof (buff) && ret == 0 && !shell_get_interrupt());
     }
  else
     {
     shell_write_error_filename (ret, target);
     }
  if (shell_get_interrupt())
    {
    shell_write_error (ERR_INTERRUPTED);
    }
  return ret;
  }

/*=========================================================================

  fileutil_rename

=========================================================================*/
ErrCode fileutil_rename (const char *source, const char *target)
  {
  ErrCode ret = 0;
  ret = storage_rename (source, target);
  if (ret)
    shell_write_error (ret);
  return ret;
  }

