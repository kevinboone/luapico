/*=========================================================================
  picolua

  shell/luapico.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#include <lua/lualib.h> 
#include <lua/lauxlib.h>
#include <klib/defs.h>

BEGIN_DECLS

extern int luapico_ls (lua_State *L); 
extern int luapico_edit (lua_State *L);
extern int luapico_df (lua_State *L);
extern int luapico_rm (lua_State *L);
extern int luapico_format (lua_State *L);
extern int luapico_read (lua_State *L); 
extern int luapico_readline (lua_State *L);
extern int luapico_write (lua_State *L);
extern int luapico_cp (lua_State *L);
extern int luapico_mkdir (lua_State *L);
extern int luapico_stat (lua_State *L);
extern int luapico_gpio_set_dir (lua_State *L);
extern int luapico_gpio_put (lua_State *L);
extern int luapico_gpio_pull_up (lua_State *L);
extern int luapico_gpio_get (lua_State *L);
extern int luapico_sleep_ms (lua_State *L);
extern int luapico_gpio_set_function (lua_State *L);
extern int luapico_pwm_pin_init (lua_State *L);
extern int luapico_pwm_pin_set_level (lua_State *L);
extern int luapico_adc_pin_init (lua_State *L);
extern int luapico_adc_select_input (lua_State *L);
extern int luapico_adc_get (lua_State *L);
extern int luapico_i2c_init (lua_State *L);
extern int luapico_i2c_write_read (lua_State *L);
extern int luapico_yrecv (lua_State *L);
extern int luapico_ysend (lua_State *L);

/* Function exported to lua/loadlib.c, for initializing this library. */
LUAMOD_API int luaopen_pico (lua_State *L);
extern void luapico_init_constants (lua_State *L);

END_DECLS
