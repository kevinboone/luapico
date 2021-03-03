/*=========================================================================

  picolua

  shell/luapico.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/
#define lpicolib_c
#define LUA_LIB

#include <stdio.h>
#include "pico/stdlib.h" 
#include <config.h>
#include <lua/lprefix.h>
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>
#include <shell/shell.h>
#include <storage/storage.h>
#include <interface/interface.h>
#include <klib/term.h> 
#include <bute2/bute2.h>
#include <ymodem/ymodem.h>
#include "libluapico/libluapico.h"

BOOL adc_initialized = FALSE;

/*=========================================================================

  luapico_edit

=========================================================================*/
int luapico_edit (lua_State *L) 
  {
  int t = lua_gettop (L);

  if (t == 1)
    {
    const char *path;
    path = luaL_checkstring (L, 1);

    //ErrCode err = shell_edit_run (path); 
    bute_run (path);
    }
  else
    luaL_error (L, "Usage: pico.edit (\"filename\")");

  return 0;
  }

/*=========================================================================

  luapico_df

=========================================================================*/
int luapico_df (lua_State *L) 
  {
  uint32_t used, total;
  ErrCode err = storage_df (&used, &total);
  if (err == 0) 
    {
    lua_newtable (L);
    lua_pushstring (L, "total");
    lua_pushnumber (L, total); 
    lua_settable (L, -3);
    lua_pushstring (L, "used");
    lua_pushnumber (L, used); 
    lua_settable (L, -3);
    lua_pushstring (L, "free");
    lua_pushnumber (L, total - used); 
    lua_settable (L, -3);
    return 1;
    }
  else
    {
    luaL_error (L, shell_strerror (err));
    return 0; // Never reached
    }
  }

/*=========================================================================

  luapico_rm

=========================================================================*/
int luapico_rm (lua_State *L) 
  {
  int t = lua_gettop (L);

   if (t > 0)
    {
    for (int i = 1; i <= t; i++)
      {
      const char *path = luaL_checkstring (L, i);
      ErrCode err = storage_rm (path);
      if (err)
        luaL_error (L, shell_strerror (err));
      }
    }
  else
    luaL_error (L, "Usage: pico.rm (\"file1\", \"file2\"...)");
    
  return 0; 
  }

/*=========================================================================

  luapico_readline

=========================================================================*/
int luapico_readline (lua_State *L) 
  {
  char buff[READLINE_MAXINPUT];
  int t = lua_gettop (L);
  if (t == 0)
    {
    BOOL interrupted = FALSE;
    BOOL ret = term_get_line (buff, sizeof (buff), &interrupted, 0, NULL);
    if (interrupted)
      luaL_error (L, "Interrupted");

    if (ret)
      lua_pushstring (L, buff);
    else
      lua_pushnil (L);
    }
  else
    {
    luaL_error (L, "Usage: string = pico.readline()");
    }
    
  return 1; 
  }

/*=========================================================================

  luapico_read

=========================================================================*/
int luapico_read (lua_State *L) 
  {
  int t = lua_gettop (L);

  if (t == 1)
    {
    const char *path = luaL_checkstring (L, 1);
    int n;
    char *buff = NULL;
    ErrCode err = storage_read_file (path, (uint8_t**) &buff, &n);
    if (err == 0)
      {
      buff = realloc (buff, (size_t)n + 1);
      buff[n] = 0;
      lua_pushstring (L, buff); 
      free (buff);
      }
    else
      luaL_error (L, shell_strerror (err));
    }
  else
    luaL_error (L, "Usage: string = pico.read (\"file\")");
    
  return 1; 
  }

/*=========================================================================

  luapico_write

=========================================================================*/
int luapico_write (lua_State *L) 
  {
  int t = lua_gettop (L);

  if (t == 2)
    {
    const char *path = luaL_checkstring (L, 1);
    const char *string = luaL_checkstring (L, 2);
    int n = (int) strlen (string);
    ErrCode err = storage_write_file (path, string, n);
    if (err == 0)
      {
      }
    else
      luaL_error (L, shell_strerror (err));
    }
  else
    luaL_error (L, "Usage: pico.write (\"file\", string)");
    
  return 0; 
  }

/*=========================================================================

  luapico_format

=========================================================================*/
int luapico_format (lua_State *L) 
  {
  char in[10];
  interface_write_string ("All data will be deleted. Proceed (y/n)? ");
  shell_get_line (in, sizeof (in) - 1);
  if (in[0] == 'y' || in[0] == 'Y')
    {
    ErrCode err = storage_format ();
    if (err)
      luaL_error (L, shell_strerror (err));
    }
  return 0; 
  }

/*=========================================================================

  luapico_mkdir

=========================================================================*/
int luapico_mkdir (lua_State *L) 
  {
  int t = lua_gettop (L);

  if (t == 1)
    {
    const char *path = luaL_checkstring (L, 1);
    ErrCode err = storage_mkdir (path);
    if (err)
      luaL_error (L, shell_strerror (err));
    }
  else
    {
    luaL_error (L, "Usage: mkdir (\"path\"");
    }
  return 0; 
  }

/*=========================================================================

  luapico_cp

=========================================================================*/
int luapico_cp (lua_State *L) 
  {
  int t = lua_gettop (L);

  if (t == 2)
    {
    const char *from = luaL_checkstring (L, 1);
    const char *to = luaL_checkstring (L, 2);
    ErrCode err = storage_copy_file (from, to);
    if (err == 0)
      {
      }
    else
      luaL_error (L, shell_strerror (err));
    }
  else
    luaL_error (L, "Usage: pico.cp (\"file1\", \"file2\")");
    
  return 0; 
  }

/*=========================================================================

  luapico_ls 

=========================================================================*/
int luapico_ls (lua_State *L) 
  {
  List *list = list_create (free);
  if (list)
    {
    int t = lua_gettop (L);

    const char *path = "/";
    if (t >= 1)
      {
      if (!lua_isnil (L, 1))
        path = luaL_checkstring (L, 1);
      }

    ErrCode err = storage_list_dir (path, list);
    if (err == 0) 
      {
      lua_newtable (L);
      int l = list_length (list);
      for (int i = 0; i < l; i++)
	{
	const char *s = list_get (list, i);
	lua_pushnumber (L, i + 1);
	lua_pushstring (L, s); 
	lua_settable (L, -3);
	}
      }
    else
      {
      luaL_error (L, shell_strerror (err));
      }

    list_destroy (list);
    }
  else
    {
    luaL_error (L, shell_strerror (ERR_NOMEM));
    }

  return 1;
  }

/*=========================================================================

  luapico_stat

=========================================================================*/
int luapico_stat (lua_State *L) 
  {
  int t = lua_gettop (L);
  if (t == 1)
    {
    const char *path = luaL_checkstring (L, 1);
    FileInfo info;
    ErrCode err = storage_info (path, &info);
    if (err == 0)
      {
      lua_newtable (L);
      lua_pushstring (L, "type");
      lua_pushstring (L, info.type == STORAGE_TYPE_DIR ? "directory" : "file");
      lua_settable (L, -3);
      lua_pushstring (L, "size");
      lua_pushnumber (L, info.size);
      lua_settable (L, -3);
      lua_pushstring (L, "name");
      lua_pushstring (L, info.name);
      lua_settable (L, -3);
      }
    else
      luaL_error (L, shell_strerror (err));
    }
  else
    {
    luaL_error (L, "Usage: stat(\"file\")");
    }

  return 1;
  }

/*=========================================================================

  luapico_ysend

=========================================================================*/
int luapico_ysend (lua_State *L) 
  {
  int t = lua_gettop (L);
  if (t == 1)
    {
    const char *path = luaL_checkstring (L, 1);
#ifdef PICO_ON_BOARD
    stdio_set_translate_crlf (&stdio_usb, false);
#endif
    YmodemErr err = ymodem_send (path); 
#ifdef PICO_ON_BOARD
    stdio_set_translate_crlf (&stdio_usb, true);
#endif
    if (err != 0)
      {
      luaL_error (L, ymodem_strerror (err));
      }
    }
  else
    {
    luaL_error (L, "Usage: ysend([\"file\"])");
    }

  return 0; // No return value
  }

/*=========================================================================

  luapico_yrecv

=========================================================================*/
int luapico_yrecv (lua_State *L) 
  {
  int t = lua_gettop (L);
  if (t == 0)
    {
    YmodemErr err = ymodem_receive (NULL, XMODEM_MAX); 
    if (err != 0)
      {
      luaL_error (L, ymodem_strerror (err));
      }
    else
      interface_write_endl();
    }
  else if (t == 1)
    {
    const char *path = luaL_checkstring (L, 1);
    YmodemErr err = ymodem_receive (path, XMODEM_MAX); 
    if (err != 0)
      {
      luaL_error (L, ymodem_strerror (err));
      }
    else
      interface_write_endl();
    }
  else
    {
    luaL_error (L, "Usage: yrecv([\"file\"])");
    }

  return 0; // No return value
  }

/*=========================================================================

  luapico_gpio_set_dir

=========================================================================*/
int luapico_gpio_set_dir (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 2)
    {
    uint8_t pin = (uint8_t)luaL_checknumber (L, 1);
    uint8_t dir = (uint8_t)luaL_checknumber (L, 2);
    interface_gpio_set_dir (pin, dir);
    }
  else
    luaL_error (L, "Usage: pico.gpio_set_dir (pin, {gpio_in | gpio_out{)");
    
  return 0; 
  }

/*=========================================================================

  luapico_gpio_set_function

=========================================================================*/
int luapico_gpio_set_function (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 2)
    {
    uint8_t pin = (uint8_t)luaL_checknumber (L, 1);
    uint8_t function = (uint8_t)luaL_checknumber (L, 2);
    interface_gpio_set_function (pin, function);
    }
  else
    luaL_error (L, 
      "Usage: pico.gpio_set_function (pin, {GPIO_FUNC_XIP...}");
    
  return 0; 
  }

/*=========================================================================

  luapico_pwm_pin_set_level

=========================================================================*/
int luapico_pwm_pin_set_level (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 2)
    {
    uint8_t pin = (uint8_t)luaL_checknumber (L, 1);
    uint16_t level = (uint16_t)luaL_checknumber (L, 2);
    interface_pwm_pin_set_level (pin, level);
    }
  else
    luaL_error (L, "Usage: pico.pwm_pin_set_level (pin, {0..65535}");
    
  return 0;
  }

/*=========================================================================

  luapico_gpio_get

=========================================================================*/
int luapico_gpio_get (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 1)
    {
    uint8_t pin = (uint8_t)luaL_checknumber (L, 1);
    uint8_t v = interface_gpio_get (pin);
    lua_pushnumber (L, v); 
    }
  else
    luaL_error (L, "Usage: level = pico.gpio_get (pin)");
    
  return 1;
  }

/*=========================================================================

  luapico_gpio_put

=========================================================================*/
int luapico_gpio_put (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 2)
    {
    uint8_t pin = (uint8_t)luaL_checknumber (L, 1);
    uint8_t level = (uint8_t)luaL_checknumber (L, 2);
    interface_gpio_put (pin, level);
    }
  else
    luaL_error (L, "Usage: pico.gpio_put (pin, {high | low | 0 | 1");
    
  return 0;
  }

/*=========================================================================

  luapico_gpio_pull_up

=========================================================================*/
int luapico_gpio_pull_up (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 1)
    {
    uint8_t pin = (uint8_t)luaL_checknumber (L, 1);
    interface_gpio_pull_up (pin); 
    }
  else
    luaL_error (L, "Usage: pico.gpio_pull_up (pin)");
    
  return 0;
  }

/*=========================================================================

  luapico_sleep_ms

=========================================================================*/
int luapico_sleep_ms (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 1)
    {
    uint32_t ms = (uint32_t)luaL_checknumber (L, 1);
    interface_sleep_ms (ms); 
    }
  else
    luaL_error (L, "Usage: pico.sleep_ms (milliseconds)");
    
  return 0;
  }

/*=========================================================================

  luapico_pwm_pin_init

=========================================================================*/
int luapico_pwm_pin_init (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 1)
    {
    uint8_t pin = (uint8_t)luaL_checknumber (L, 1);
    interface_pwm_pin_init (pin); 
    }
  else
    luaL_error (L, "Usage: pico.pwm_pin_init (pin)");
    
  return 0;
  }

/*=========================================================================

  luapico_init_adc

=========================================================================*/
static void luapico_init_adc (void)
  {
  interface_adc_init();
  adc_initialized = TRUE;
  }

/*=========================================================================

  luapico_adc_pin_init

=========================================================================*/
int luapico_adc_pin_init (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 1)
    {
    if (!adc_initialized)
      luapico_init_adc(); 
    uint8_t pin = (uint8_t)luaL_checknumber (L, 1);
    interface_adc_pin_init (pin); 
    }
  else
    luaL_error (L, "Usage: pico.adc_pin_init (pin)");
    
  return 0;
  }

/*=========================================================================

  luapico_adc_select_input

=========================================================================*/
int luapico_adc_select_input (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 1)
    {
    if (!adc_initialized)
      luapico_init_adc(); 
    uint8_t input = (uint8_t)luaL_checknumber (L, 1);
    interface_adc_select_input (input); 
    }
  else
    luaL_error (L, "Usage: pico.adc_select_input (input)");
    
  return 0;
  }

/*=========================================================================

  luapico_adc_get

=========================================================================*/
int luapico_adc_get (lua_State *L)
  {
  int16_t val = interface_adc_get();
  lua_pushnumber (L, val);
  return 1;
  }

/*=========================================================================

  luapico_i2c_init

=========================================================================*/
int luapico_i2c_init (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 2)
    {
    uint8_t port = (uint8_t)luaL_checknumber (L, 1);
    uint32_t baud = (uint32_t)luaL_checknumber (L, 2);
    interface_i2c_init (port, baud);
    }
  else
    luaL_error (L, "Usage: pico.i2c_init (port, baud)");
    
  return 0;
  }

/*=========================================================================

  luapico_i2c_write_read

=========================================================================*/
int luapico_i2c_write_read (lua_State *L)
  {
  int t = lua_gettop (L);

  if (t == 4)
    {
    unsigned int out_len;
    uint8_t port = (uint8_t)luaL_checknumber (L, 1);
    uint8_t addr = (uint8_t)luaL_checknumber (L, 2);
    luaL_checkstring (L, 3);
    const char *out = lua_tolstring (L, 3, (size_t *)&out_len);
    unsigned int in_len = (unsigned int)luaL_checknumber (L, 4);

    char *in = malloc (in_len);
    if (in)
      {
      ErrCode err = interface_i2c_write_read 
        (port, addr, out_len, (uint8_t*)out, in_len, (uint8_t*)in); 
      if (err == 0)
        lua_pushlstring (L, in, in_len);
      else
        luaL_error (L, shell_strerror (err));
      free (in);
      }
    else
      luaL_error (L, shell_strerror (ERR_NOMEM));
    }
  else
    luaL_error (L, 
      "Usage: pico.i2c_write_read (port, addr, \"data\", num_read)");
    
  return 1;
  }


/*=========================================================================

  function table 

=========================================================================*/
static const luaL_Reg picolib[] = 
  {
  {"ls", luapico_ls},
  {"edit", luapico_edit},
  {"df", luapico_df},
  {"rm", luapico_rm},
  {"format", luapico_format},
  {"read", luapico_read},
  {"write", luapico_write},
  {"cp", luapico_cp},
  {"mkdir", luapico_mkdir},
  {"stat", luapico_stat},
  {"gpio_set_dir", luapico_gpio_set_dir},
  {"gpio_put", luapico_gpio_put},
  {"gpio_pull_up", luapico_gpio_pull_up},
  {"gpio_get", luapico_gpio_get},
  {"sleep_ms", luapico_sleep_ms},
  {"pwm_pin_init", luapico_pwm_pin_init},
  {"pwm_pin_set_level", luapico_pwm_pin_set_level},
  {"gpio_set_function", luapico_gpio_set_function},
  {"adc_pin_init", luapico_adc_pin_init},
  {"adc_select_input", luapico_adc_select_input},
  {"adc_get", luapico_adc_get},
  {"i2c_init", luapico_i2c_init},
  {"i2c_write_read", luapico_i2c_write_read},
  {"yrecv", luapico_yrecv},
  {"ysend", luapico_ysend},
  {"readline", luapico_readline},
  {NULL, NULL}
  };

/*=========================================================================

  constant table 

=========================================================================*/
struct PicoConstant
  {
  const char *name;
  int val;
  };

struct PicoConstant pico_constants[] = 
  {
  {"LOW", 0},
  {"HIGH", 1},
  {"GPIO_IN", 0},
  {"GPIO_OUT", 1},
  {"GPIO_FUNC_XIP", 0},
  {"GPIO_FUNC_SPI", 1},
  {"GPIO_FUNC_UART", 2},
  {"GPIO_FUNC_I2C", 3},
  {"GPIO_FUNC_PWM", 4},
  {"GPIO_FUNC_SIO", 5},
  {"GPIO_FUNC_PIO0", 6},
  {"GPIO_FUNC_PIO1", 7},
  {"GPIO_FUNC_GPCK", 8},
  {"GPIO_FUNC_USB", 9},
  {"GPIO_FUNC_NULL", 0xF},
  {NULL, 0}
  };


/*=========================================================================

  luaopen_pico 

=========================================================================*/
LUAMOD_API int luaopen_pico (lua_State *L)
  {
  luaL_newlib (L, picolib);
  return 1;
  }

/*=========================================================================

  luapico_init_constants

=========================================================================*/
void luapico_init_constants (lua_State *L)
  {
  int n = 0;
  while (pico_constants[n].name)
    {
    struct PicoConstant *c = &pico_constants[n];  
    lua_pushnumber (L, c->val);
    lua_setglobal(L, c->name);
    n++;
    }
  }













