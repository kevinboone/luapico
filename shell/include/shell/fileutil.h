#pragma once

#include <klib/defs.h>
#include <shell/errcodes.h>

BEGIN_DECLS

extern ErrCode fileutil_copy (const char *source, const char *target);
extern ErrCode fileutil_rename (const char *source, const char *target);

END_DECLS
