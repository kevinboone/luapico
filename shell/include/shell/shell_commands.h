/*=========================================================================

  picolua

  shell/shell_commands.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/

#pragma once

#include <klib/defs.h> 
#include "shell/errcodes.h"

BEGIN_DECLS

extern ErrCode shell_cmd_df (int argc, char **argv);
extern ErrCode shell_cmd_ls (int argc, char **argv);
extern ErrCode shell_cmd_mkdir (int argc, char **argv);
extern ErrCode shell_cmd_rm (int argc, char **argv);
extern ErrCode shell_cmd_echo (int argc, char **argv);
extern ErrCode shell_cmd_cat (int argc, char **argv);
extern ErrCode shell_cmd_yrecv (int argc, char **argv);
extern ErrCode shell_cmd_ysend (int argc, char **argv);
extern ErrCode shell_cmd_cp (int argc, char **argv);
extern ErrCode shell_cmd_mv (int argc, char **argv);
extern ErrCode shell_cmd_format (int argc, char **argv);
extern ErrCode shell_cmd_i2cdetect (int argc, char **argv);

END_DECLS

