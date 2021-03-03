/*===========================================================================

  BUTE version 2

  The "Basically Usable Text Editor"

  This editor is based on work by Michael Ringgaard, and released under
  a BSD licemce. The original source is here:

  http://www.jbox.dk/downloads/edit.c

===========================================================================*/

#pragma once

#include "../errcodes.h"
#include <klib/defs.h> 

struct _ButeEnv;
typedef struct _ButeEnv ButeEnv;

BEGIN_DECLS

/** Create the BUTE environment. If this fails, it will be because
    of a lack of memory. */
extern ButeEnv *buteenv_create (void);

/** Destroy the environment and tidy up. */
extern void buteenv_destroy (ButeEnv *self);

/** Adds an editor to the environment. The filename need not relate to
    a file that exists. It is not an error if it does not. The only
    likely error return is ERR_NOMEM. */
extern ErrCode buteenv_add_editor (ButeEnv *self, const char *filename);

/** Run the editor(s). */
extern void buteenv_run (ButeEnv *self);

/** Umbrella function that creates a ButeEnv, adds an editor for the 
    specified filename, and runs it. The filename may be NULL, which
    gives an untitled editor. */
extern void bute_run (const char *filename);

END_DECLS
