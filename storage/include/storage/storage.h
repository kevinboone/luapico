/*============================================================================
 * storage.h
 *
 * Copyright (c)2021 Kevin Boone.
 * =========================================================================*/

#pragma once

#include <klib/defs.h>
#include <klib/list.h>
#include <config.h>

#define STORAGE_NAME_MAX MAX_FNAME 

typedef ErrCode (*StorageEnumBytesFn)(uint8_t byte, void *user_data);

typedef enum _FileType
  {
  STORAGE_TYPE_REG = 0,
  STORAGE_TYPE_DIR = 1
  } FileType;

typedef struct _FileInfo
  {
  FileType type;
  uint32_t size;
  char name[STORAGE_NAME_MAX + 1];
  } FileInfo;

BEGIN_DECLS

extern void    storage_init (void);
extern void    storage_cleanup (void);

extern ErrCode storage_read_file (const char *filemame, uint8_t **buff,
                  int *n);

extern ErrCode storage_read_partial (const char *filemame, int offset, 
                  int count, uint8_t *buff, int *n);

extern ErrCode storage_write_file (const char *filename, 
                  const void *buf, int len);

/** Append to a file. If the file does not exist, it
    is created. */
extern ErrCode storage_append_file (const char *filename, 
                  const void *buf, int len);

extern ErrCode storage_format (void);

/** Get total and used storage in bytes. */
extern ErrCode storage_df (const char *path, 
          uint32_t *used, uint32_t *total);

/** This is a convenience function, to test whether a file exists. It
    doesn't distinguish between the file not existing, and the filesystem
    being broker. */
extern BOOL storage_file_exists (const char *path);

extern ErrCode storage_create_empty_file (const char *path);

/** Call the callback function once for each byte in the file. The callback
    should return zero to continue. If it returns non-zero, this is taken
    as the error code to the caller, as well as stopping the enumeration. */
extern ErrCode storage_enumerate_bytes (const char *path, 
                 StorageEnumBytesFn fn, void *user_data);

/** Delete a file or an empty directory. */
extern ErrCode storage_rm (const char *path);

/** Write the directory contents into a List of char* (which must already have
    been initialized. */
extern ErrCode storage_list_dir (const char *path, List *list);

/** Copy a file. Both arguments must be filenames, not directories. */
extern ErrCode storage_copy_file (const char *from, const char *to);

extern ErrCode storage_info (const char *path, FileInfo *info);

extern ErrCode storage_mkdir (const char *path);

extern void storage_join_path (const char *path, const char *fname, 
              char result[MAX_PATH + 1]); 

/** Get the filename part of a path. This test is based entirely
    on the names, not on examination of the filesystem, so cannot be
    definitive. If the path ends with "/", filename will be an empty
    string. */
extern void storage_get_basename (const char *path, 
              char result[MAX_FNAME + 1]);

/** Get the "directory" part of a pathname. This test is based entirely
    on the names, not on examination of the filesystem, so cannot be
    definitive. The trailing slashes on the directory are removed, unless
    that would leave an empty directory name. However, if there is no
    slash in the path, directory name is empty (not ".") by design. 
    If a path ends in /, than the whole thing is taken to be a directory. */
extern void storage_get_dir (const char *path, 
              char result[MAX_PATH + 1]);

extern ErrCode storage_rename (const char *source, const char *target);

END_DECLS

