/*=========================================================================

  picolua

  shell/storage.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <config.h>
#include <interface/interface.h>
#include <errcodes.h>
#include <klib/list.h>
#include <shell/shell.h>
#include "storage/storage.h"
#include "storage/lfs.h"

extern char *itoa (int n, char *buff, int base);

lfs_t lfs;
BOOL mounted = FALSE;

const struct lfs_config cfg = {
    // block device operations
    .read  = interface_block_read,
    .prog  = interface_block_prog,
    .erase = interface_block_erase,
    .sync  = interface_block_sync,

    // block device configuration
    .read_size = 256,
    .prog_size = 256,
    .block_size = INTERFACE_STORAGE_BLOCK_SIZE,
    .block_count = INTERFACE_STORAGE_BLOCK_COUNT,
    .cache_size = 256,
    .lookahead_size = 256,
    .block_cycles = 500,
};


/*=========================================================================

  storage_init 

=========================================================================*/
void storage_init (void)
  {
  interface_block_init ();
  mounted = FALSE;
  int err = lfs_mount (&lfs, &cfg);
  if (err)
    {
    if (lfs_format (&lfs, &cfg))
      {
      printf ("Format failed\n"); // TODO
      }
    else
      {
      lfs_mount (&lfs, &cfg);
      mounted = TRUE; // We hope...
      }
    }
  else
    mounted = TRUE;
  }

/*=========================================================================

  storage_cleanup

=========================================================================*/
void storage_cleanup (void)
  {
  if (mounted)
    lfs_unmount (&lfs);
  interface_block_cleanup ();
  }

/*=========================================================================

  storage_write_file

=========================================================================*/
ErrCode storage_write_file (const char *filename, const void *buf, int len)
  {
  lfs_file_t file;
  int err = lfs_file_open (&lfs, &file, filename, 
       LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC);
  if (err)
    return (ErrCode) -err;

  lfs_ssize_t n = lfs_file_write (&lfs, &file, buf, (lfs_size_t)len);
  lfs_file_close (&lfs, &file);

  if (n != len)
    return (ErrCode) -n;
  else 
    return 0;
  }

/*=========================================================================

  storage_append_file

=========================================================================*/
ErrCode storage_append_file (const char *filename, const void *buf, int len)
  {
  lfs_file_t file;
  int err = lfs_file_open (&lfs, &file, filename, 
     LFS_O_RDWR | LFS_O_APPEND | LFS_O_CREAT);
  if (err)
    return (ErrCode) -err;

  lfs_ssize_t n = lfs_file_write (&lfs, &file, buf, (lfs_size_t)len);
  lfs_file_close (&lfs, &file);

  if (n != len)
    return (ErrCode) -n;
  else 
    return 0;
  }

/*=========================================================================

  storage_list_dir

=========================================================================*/
ErrCode storage_list_dir (const char *path, List *list)
  {
  lfs_dir_t dir;

  int err = lfs_dir_open (&lfs, &dir, path);
  if (err)
    return (ErrCode) -err;

  struct lfs_info info;
  int ret = lfs_dir_read (&lfs, &dir, &info);
  while (ret > 0)
    {
    list_append (list, strdup (info.name));
    ret = lfs_dir_read (&lfs, &dir, &info);
    }

  lfs_dir_close (&lfs, &dir);

  return 0;
  }

/*=========================================================================

  storage_df

=========================================================================*/
ErrCode storage_df (uint32_t *used, uint32_t *total)
  {
  ErrCode ret = 0;
  int res = (int)lfs_fs_size (&lfs);
  if (res >= 0) 
    {
    *used = (uint32_t)res * INTERFACE_STORAGE_BLOCK_SIZE;
    *total = INTERFACE_STORAGE_BLOCK_SIZE * INTERFACE_STORAGE_BLOCK_COUNT;
    }
  else
    ret = (ErrCode) -res;
  return ret;
  }

/*=========================================================================

  storage_format

=========================================================================*/
ErrCode storage_format (void)
  {
  ErrCode ret = 0;
  if (mounted)
    lfs_unmount (&lfs); // Continue whether this succeeds or not
  mounted = FALSE;
  int err = lfs_format (&lfs, &cfg);
  if (err == 0)
    {
    err = lfs_mount (&lfs, &cfg);
    if (err) ret = (ErrCode) -err;
    }
  else
    ret = (ErrCode) -err;
  return ret;
  }

/*=========================================================================

  storage_file_exists

=========================================================================*/
BOOL storage_file_exists (const char *path)
  {
  BOOL ret = FALSE;
  lfs_file_t file;
  int err = lfs_file_open (&lfs, &file, path, LFS_O_RDONLY);
  if (err == 0)
    {
    lfs_file_close (&lfs, &file);
    ret = TRUE;
    }
  else
    ret = FALSE;
  return ret;
  }

/*=========================================================================

  storage_create_empty_file

=========================================================================*/
ErrCode storage_create_empty_file (const char *path)
  {
  return storage_write_file (path, "", 0);
  }

/*=========================================================================

  storage_enumerate_bytes

=========================================================================*/
ErrCode storage_enumerate_bytes (const char *path, 
                 StorageEnumBytesFn fn, void *user_data)
  {
  ErrCode ret = 0;
  lfs_file_t file;

  char *buff = malloc (INTERFACE_STORAGE_BLOCK_SIZE);
  if (!buff) 
    return ERR_NOMEM;

  int err = lfs_file_open (&lfs, &file, path, 
       LFS_O_RDONLY);

  if (err == 0)
    {
    lfs_ssize_t n = 0; 
    do
      {
      lfs_ssize_t n = lfs_file_read (&lfs, &file, buff, 
         INTERFACE_STORAGE_BLOCK_SIZE);
      if ((int)n > 0)
        {
        for (int i = 0; i < n; i++)
          {
	  fn ((uint8_t)buff[i], user_data);
          }
        }
      } while (n > 0);
    lfs_file_close (&lfs, &file);
    }
  else
    {
    ret = (ErrCode) -err;
    }
  free (buff);
  return ret;
  }

/*=========================================================================

  storage_rm

=========================================================================*/
extern ErrCode storage_rm (const char *path)
  {
  int err = lfs_remove (&lfs, path);
  
  return (ErrCode)-err;
  }

/*=========================================================================

  storage_read_file

=========================================================================*/
ErrCode storage_read_file (const char *path, 
         uint8_t **buff, int *n)
  {
  ErrCode ret = 0;
  lfs_file_t file;
  int err = lfs_file_open (&lfs, &file, path, LFS_O_RDONLY);
  if (err == 0)
    {
    int size = (int)lfs_file_size (&lfs, &file);
    if (size >= 0)
      {
      *buff = malloc ((unsigned)size);
      if (*buff)
        {
	lfs_file_read (&lfs, &file, *buff, (unsigned)size);
	*n = size;
	}
      else
        {
	ret = ERR_NOMEM;
	}
      }
    else 
      {
      ret = (ErrCode)-size;
      }
    lfs_file_close (&lfs, &file);
    }
  else
    ret = (ErrCode)-err;
  return ret;
  }

/*=========================================================================

  storage_copy_file

=========================================================================*/
ErrCode storage_copy_file (const char *from, const char *to)
  {
  ErrCode ret = 0;
  lfs_file_t file_from;
  lfs_file_t file_to;
  int err = lfs_file_open (&lfs, &file_from, from, LFS_O_RDONLY);
  if (err == 0)
    {
    char *buff = malloc (INTERFACE_STORAGE_BLOCK_SIZE);
    if (buff) 
      {
      int err = lfs_file_open (&lfs, &file_to, to, 
         LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC);
      if (err == 0)
        {
        lfs_ssize_t n = 0;
        do 
          {
          n = lfs_file_read (&lfs, &file_from, buff, 
            INTERFACE_STORAGE_BLOCK_SIZE);
          if ((int)n > 0)
            {
            lfs_file_write (&lfs, &file_to, buff, (unsigned)n);
            }
          } while (n > 0  && shell_get_interrupt() == FALSE);
        lfs_file_close (&lfs, &file_to);
        if (shell_get_interrupt())
          {
          storage_rm (to);
          }
        }
      else
        ret = (ErrCode)-err;
      }
    else
      ret = ERR_NOMEM;

    lfs_file_close (&lfs, &file_from);
    }
  else 
    ret = (ErrCode)-err;
  return ret;
  }

/*=========================================================================

  storage_info

=========================================================================*/
ErrCode storage_info (const char *path, FileInfo *info)
  {
  struct lfs_info linfo;
  int err = lfs_stat (&lfs, path, &linfo);
  if (err == 0)
    {
    strncpy (info->name, linfo.name, STORAGE_NAME_MAX);
    info->type = linfo.type == LFS_TYPE_DIR ? STORAGE_TYPE_DIR 
      : STORAGE_TYPE_REG;
    info->size = linfo.type == LFS_TYPE_REG ? linfo.size : 0; 
    return 0;
    }
  else
    return (ErrCode) -err; 
  }

/*=========================================================================

  storage_mkdir

=========================================================================*/
ErrCode storage_mkdir (const char *path)
  {
  int err = lfs_mkdir (&lfs, path);
  if (err == 0)
    {
    return 0;
    }
  else
    return (ErrCode) -err; 
  }


