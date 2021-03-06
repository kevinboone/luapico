/*=========================================================================

  picolua

  libluapico/picoutils.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#if PICO_ON_DEVICE
#include "hardware/i2c.h" 
#endif

#if PICO_ON_DEVICE
i2c_inst_t *picoutils_get_i2c_from_pin (uint8_t pin)
  {
  switch (pin)
    {
    case 1: case 2: case 6: case 7: case 11: case 12:
    case 16: case 17: case 21: case 22: case 26: case 27:
      return i2c0;

    case 4: case 5: case 9: case 10: case 14: case 15:
    case 19: case 20: case 24: case 25: case 31: case 32:
      return i2c1;
    }

  return NULL;
  }
#endif

