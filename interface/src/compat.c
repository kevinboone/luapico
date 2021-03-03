/*==========================================================================

  compat.c

  Functions for supplying missing functions in one build platform or
  another 

  (c)2021 Kevin Boone, GPLv3.0

==========================================================================*/
#include <string.h> 
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


