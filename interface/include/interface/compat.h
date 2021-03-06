/*==========================================================================

  compat.c

  Functions for supplying missing functions in one build platform or
  another 

  (c)2021 Kevin Boone, GPLv3.0

==========================================================================*/
#pragma once

#include <klib/defs.h>

// Constants for my_fnmatch()
#define MYFNM_NOMATCH     -1
#define MYFNM_LEADING_DIR 0x0001 
#define MYFNM_NOESCAPE    0x0002 
#define MYFNM_PERIOD      0x0004 
#define MYFNM_FILE_NAME   0x0008 
#define MYFNM_CASEFOLD    0x0010 

BEGIN_DECLS

extern char *itoa (int num, char* str, int base);

// Simplified replacement for the glibc fnmatch (not present in
//   Pico SDK)
extern int my_fnmatch (const char *patten, const char *fname, int flags);

END_DECLS
