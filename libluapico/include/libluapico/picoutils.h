/*=========================================================================
  picolua

  libluapico/picoutils.h

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#include <klib/defs.h>

#if PICO_ON_DEVICE
#include "hardware/i2c.h" 
#endif

BEGIN_DECLS

#if PICO_ON_DEVICE
i2c_inst_t *picoutils_get_i2c_from_pin (uint8_t pin);
#endif

END_DECLS

