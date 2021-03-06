/*=========================================================================

  picolua

  shell/startfiles.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/

char *file_bin_blink_lua = 
"-- Flash the on-board LED\n"
"gpio_pin = 25\n"
"pico.gpio_set_function (gpio_pin, GPIO_FUNC_SIO)\n"
"pico.gpio_set_dir (gpio_pin, GPIO_OUT)\n"
"while true do\n"
"  pico.gpio_put (gpio_pin, HIGH)\n"
"  pico.sleep_ms (300)\n"
"  pico.gpio_put (gpio_pin, LOW)\n"
"  pico.sleep_ms (300)\n"
"end\n";

char *file_etc_luarc_lua = 
"--Lua start-up code goes here\n";

char *file_etc_shellrc_sh = 
"# Shell start-up code goes here\n";


