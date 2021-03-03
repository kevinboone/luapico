/*==========================================================================

  compat.c

  Functions for supplying missing functions in one build platform or
  another 

  (c)2021 Kevin Boone, GPLv3.0

==========================================================================*/
#pragma once

#include <klib/defs.h>

BEGIN_DECLS

extern char *itoa (int num, char* str, int base);

END_DECLS
