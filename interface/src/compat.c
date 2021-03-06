/*==========================================================================

  compat.c

  Functions for supplying missing functions in one build platform or
  another 

  (c)2021 Kevin Boone, GPLv3.0

==========================================================================*/
#include <string.h> 
#include <ctype.h> 
#include <klib/defs.h> 
#include "interface/compat.h"

#if PICO_ON_DEVICE
extern char *itoa (int num, char* str, int base);
#else
void swap (char *x, char *y) { char t = *x; *x = *y; *y = t; }

void reverse(char *buffer, int i, int j)
  {
  while (i < j)
    swap (&buffer[i++], &buffer[j--]);
  }

char *itoa (int num, char* str, int base)
  {
  int i = 0;
  BOOL isNegative = FALSE;

  if (num == 0)
    {
    str[i++] = '0';
    str[i] = '\0';
    return str;
    }

  if (num < 0 && base == 10)
    {
    isNegative = TRUE;
    num = -num;
    }

  while (num != 0)
    {
    int rem = num % base;
    str[i++] = (char) ((rem > 9)? (rem-10) + 'a' : rem + '0' );
    num = num/base;
    }

  if (isNegative)
    str[i++] = '-';

  str[i] = '\0'; 

  reverse (str, 0, i - 1);
  return str;
  }
#endif

/*==========================================================================

  my_fnmatch

==========================================================================*/
int my_fnmatch (const char *pattern, const char *string, int flags)
  {
  register const char *p = pattern, *n = string;
  register unsigned char c;

#define FOLD(c)	((flags & MYFNM_CASEFOLD) ? tolower (c) : (c))

  while ((c = *p++) != '\0')
    {
      c = FOLD (c);

      switch (c)
	{
	case '?':
	  if (*n == '\0')
	    return MYFNM_NOMATCH;
	  else if ((flags & MYFNM_FILE_NAME) && *n == '/')
	    return MYFNM_NOMATCH;
	  else if ((flags & MYFNM_PERIOD) && *n == '.' &&
		   (n == string || ((flags & MYFNM_FILE_NAME) && n[-1] == '/')))
	    return MYFNM_NOMATCH;
	  break;

	case '\\':
	  if (!(flags & MYFNM_NOESCAPE))
	    {
	      c = *p++;
	      c = FOLD (c);
	    }
	  if (FOLD ((unsigned char)*n) != c)
	    return MYFNM_NOMATCH;
	  break;

	case '*':
	  if ((flags & MYFNM_PERIOD) && *n == '.' &&
	      (n == string || ((flags & MYFNM_FILE_NAME) && n[-1] == '/')))
	    return MYFNM_NOMATCH;

	  for (c = *p++; c == '?' || c == '*'; c = *p++, ++n)
	    if (((flags & MYFNM_FILE_NAME) && *n == '/') ||
		(c == '?' && *n == '\0'))
	      return MYFNM_NOMATCH;

	  if (c == '\0')
	    return 0;

	  {
	    unsigned char c1 = (!(flags & MYFNM_NOESCAPE) && c == '\\') ? *p : c;
	    c1 = FOLD (c1);
	    for (--p; *n != '\0'; ++n)
	      if ((c == '[' || FOLD ((unsigned char)*n) == c1) &&
		  my_fnmatch (p, n, flags & ~MYFNM_PERIOD) == 0)
		return 0;
	    return MYFNM_NOMATCH;
	  }

	case '[':
	  {
	    /* Nonzero if the sense of the character class is inverted.  */
	    register int negate;

	    if (*n == '\0')
	      return MYFNM_NOMATCH;

	    if ((flags & MYFNM_PERIOD) && *n == '.' &&
		(n == string || ((flags & MYFNM_FILE_NAME) && n[-1] == '/')))
	      return MYFNM_NOMATCH;

	    negate = (*p == '!' || *p == '^');
	    if (negate)
	      ++p;

	    c = *p++;
	    for (;;)
	      {
		register unsigned char cstart = c, cend = c;

		if (!(flags & MYFNM_NOESCAPE) && c == '\\')
		  cstart = cend = *p++;

		cstart = cend = FOLD (cstart);

		if (c == '\0')
		  /* [ (unterminated) loses.  */
		  return MYFNM_NOMATCH;

		c = *p++;
		c = FOLD (c);

		if ((flags & MYFNM_FILE_NAME) && c == '/')
		  /* [/] can never match.  */
		  return MYFNM_NOMATCH;

		if (c == '-' && *p != ']')
		  {
		    cend = *p++;
		    if (!(flags & MYFNM_NOESCAPE) && cend == '\\')
		      cend = *p++;
		    if (cend == '\0')
		      return MYFNM_NOMATCH;
		    cend = FOLD (cend);

		    c = *p++;
		  }

		if (FOLD ((unsigned char)*n) >= cstart
		    && FOLD ((unsigned char)*n) <= cend)
		  goto matched;

		if (c == ']')
		  break;
	      }
	    if (!negate)
	      return MYFNM_NOMATCH;
	    break;

	  matched:;
	    /* Skip the rest of the [...] that already matched.  */
	    while (c != ']')
	      {
		if (c == '\0')
		  /* [... (unterminated) loses.  */
		  return MYFNM_NOMATCH;

		c = *p++;
		if (!(flags & MYFNM_NOESCAPE) && c == '\\')
		  /* XXX 1003.2d11 is unclear if this is right.  */
		  ++p;
	      }
	    if (negate)
	      return MYFNM_NOMATCH;
	  }
	  break;

	default:
	  if (c != FOLD ((unsigned char)*n))
	    return MYFNM_NOMATCH;
	}

      ++n;
    }

  if (*n == '\0')
    return 0;

  if ((flags & MYFNM_LEADING_DIR) && *n == '/')
    /* The MYFNM_LEADING_DIR flag says that "foo*" matches "foobar/frobozz".  */
    return 0;

  return MYFNM_NOMATCH;
  }


