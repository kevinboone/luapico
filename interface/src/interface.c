#include <stdio.h> 

#if PICO_ON_DEVICE
#include "pico/stdlib.h" 
#include "hardware/gpio.h" 
#include "hardware/flash.h" 
#include "hardware/sync.h" 
#include "hardware/adc.h" 
#include "hardware/pwm.h" 
#include "hardware/i2c.h" 
#endif

#include <klib/defs.h> 
#include "interface/interface.h"
#include "shell/shell.h"

#if PICO_ON_DEVICE
const uint LED_PIN = 25;
extern uint32_t __flash_binary_end;
extern uint32_t __flash_binary_start;
#define FLASH_STORAGE_OFFSET 0x60000
#define FLASH_START_MEM 0x10000000
#define FLASH_STORAGE_START_MEM (FLASH_START_MEM + FLASH_STORAGE_OFFSET)
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
struct termios orig_termios;
#define BLOCKFILE "/tmp/picolua.blockdev"
int blockfd = -1;
#endif 

/*===========================================================================

  interface_get_char

===========================================================================*/
int interface_get_char (void)
  {
#if PICO_ON_DEVICE
  int c;
  while ((c = getchar_timeout_us (0)) < 0)
    {
    // gpio_put (LED_PIN, 1);
    // sleep_ms (50);
    // gpio_put (LED_PIN, 0);
    sleep_ms (1); 
    }
  return c;
#else
  int c;
  while ((c = getchar ()) < 0)
    {
    usleep (10000); 
    }
  return c;
#endif
  }

/*===========================================================================

  interface_get_char_timeout

===========================================================================*/
int interface_get_char_timeout (int msec)
  {
#if PICO_ON_DEVICE
  int c;
  int loops = 0;
  while ((c = getchar_timeout_us (0)) < 0 && loops < msec)
    {
    sleep_us (1000);
    loops++;
    }
  return c;
#else
  (void)msec;
  int c;
  int loops = 0;
  while ((c = getchar ()) < 0 && loops < msec)
    {
    usleep (1000);
    loops++;
    }
  return c;
#endif
  }

/*===========================================================================

  interface_init

===========================================================================*/
void interface_init (void)
  {
#if PICO_ON_DEVICE
  gpio_init (LED_PIN);
  gpio_set_dir (LED_PIN, GPIO_OUT);
#else
  tcgetattr (STDIN_FILENO, &orig_termios);
  struct termios raw = orig_termios;
  raw.c_iflag &= (unsigned int) ~(IXON);
  raw.c_lflag &= (unsigned int) ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VTIME] = 1;
  raw.c_cc[VMIN] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw);
#endif
  }

/*===========================================================================

  interface_write_char

===========================================================================*/
void interface_write_char (char c)
  {
#if PICO_ON_DEVICE
  putchar (c);
#else
  putchar (c);
  fflush (stdout);
#endif
  }

/*===========================================================================

  interface_write_endl

===========================================================================*/
void interface_write_endl (void)
  {
//#if PICO_ON_DEVICE
  puts ("\r"); // \n should be automatic 
//#endif
  }

/*===========================================================================

  interface_write_string

===========================================================================*/
void interface_write_string (const char *s)
  {
#if PICO_ON_DEVICE
  fputs (s, stdout);
  fflush (stdout);
#else
  while (*s)
    {
    interface_write_char (*s);
    s++;
    }
#endif
  }

/*===========================================================================

  interface_write_buff

===========================================================================*/
void interface_write_buff (const char *s, int len)
  {
#if PICO_ON_DEVICE
  fwrite (s, len, 1, stdout);
  fflush (stdout);
#else
  for (int i = 0; i < len; i++)
    interface_write_char (s[i]);
#endif
  }


/*===========================================================================

  interface_block_init

===========================================================================*/
BOOL interface_block_init (void)
  {
#if PICO_ON_DEVICE
  if (FLASH_STORAGE_START_MEM < __flash_binary_end)
    {
    printf ("AARGH! Start of flash storage area is too low.\n");
    return FALSE;
    }

  return TRUE;
#else
  blockfd = open (BLOCKFILE, O_RDWR);
  if (blockfd < 0)
    {
    printf ("Can't open block storage file %s\n", BLOCKFILE);
    return FALSE;
    }
  return TRUE;
#endif
  }

/*===========================================================================

  interface_block_cleanup

===========================================================================*/
void interface_block_cleanup (void)
  {
#if PICO_ON_DEVICE
  // Do we have to do anything here?
#else
  close (blockfd);
#endif
  }

/*===========================================================================

  interface_block_sync

===========================================================================*/
int interface_block_sync (const struct lfs_config *cfg)
  {
  (void)cfg;
#if PICO_ON_DEVICE
  // Do we have to do anything here?
#else
  (void)cfg;
  sync();
#endif
  return 0;
  }

/*===========================================================================

  interface_block_erase

===========================================================================*/
extern int  interface_block_erase (const struct lfs_config *cfg, 
    lfs_block_t block)
  {
#if PICO_ON_DEVICE
  (void)cfg;
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase (FLASH_STORAGE_OFFSET + (block * INTERFACE_STORAGE_BLOCK_SIZE), INTERFACE_STORAGE_BLOCK_SIZE);
  restore_interrupts (ints);
#else
  (void)cfg; (void)block;
#endif
  return 0;
  }

/*===========================================================================

  interface_block_prog

===========================================================================*/
extern int interface_block_prog (const struct lfs_config *cfg, 
        lfs_block_t block, lfs_off_t off, const void *buffer, 
	lfs_size_t size)
  {
#if PICO_ON_DEVICE
  (void)block; (void)cfg;
  //interface_block_erase (cfg, block);
  int mem = FLASH_STORAGE_OFFSET + 
      ((int)block * (int)cfg->block_size) + (int)off;
  //printf ("PROG %08X %ld %ld %02X %02X\n", mem, size,
  //  block, ((char *)buffer)[0], ((char *)buffer)[1]);
  uint32_t ints = save_and_disable_interrupts();
  flash_range_program ((uint32_t)mem, buffer, size);
  restore_interrupts (ints);
  //printf ("PROG done\n");

  return 0;
#else
  off_t res1 = lseek (blockfd,
            (off_t)block*cfg->block_size + (off_t)off, SEEK_SET);
  if (res1 < 0) 
    {
    int err = -errno;
    return err;
    }

  ssize_t res2 = write (blockfd, buffer, size);
  if (res2 < 0) 
    {
    int err = -errno;
    return err;
    }

  return 0;
#endif
  }

/*===========================================================================

  interface_block_read

===========================================================================*/
extern int interface_block_read (const struct lfs_config *cfg, 
        lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
  {
#if PICO_ON_DEVICE
  (void)cfg; (void)block;
  //char *mem = (char *)FLASH_STORAGE_START_MEM + off;
  char *mem = (char *)FLASH_STORAGE_START_MEM + 
      ((int)block * (int)cfg->block_size) + (int)off;
  //printf ("READ %p %ld %ld %02X %02X\n", mem, size, block, *mem, *(mem+1)) ;
  memcpy (buffer, mem, size);
  //printf ("READ done\n");
  return 0;
#else
  off_t res1 = lseek(blockfd,
            (off_t)block*cfg->block_size + (off_t)off, SEEK_SET);
  if (res1 < 0) 
    {
    int err = -errno;
    return err;
    }

  ssize_t res2 = read (blockfd, buffer, size);
  if (res2 < 0) 
    {
    int err = -errno;
    return err;
    }
  return 0;
  #endif
  }

/*===========================================================================

  interface_gpio_put

===========================================================================*/
void interface_gpio_put (uint8_t pin, uint8_t level)
  {
#if PICO_ON_DEVICE
  gpio_put (pin, level);
#else
  printf ("gpio_set_dir: pin=%d dir=%d\n", pin, level); 
#endif
  }

/*===========================================================================

  interface_gpio_get

===========================================================================*/
uint8_t interface_gpio_get (uint8_t pin)
  {
#if PICO_ON_DEVICE
  return (uint8_t)gpio_get (pin);
#else
  (void)pin;
  return 0; // No implementation
#endif
  }

/*===========================================================================

  interface_gpio_pull_up

===========================================================================*/
void interface_gpio_pull_up (uint8_t pin)
  {
#if PICO_ON_DEVICE
  gpio_pull_up (pin);
#else
  printf ("gpio_pull_up: pin=%d\n", pin); 
#endif
  }

/*===========================================================================

  interface_gpio_set_dir

===========================================================================*/
void interface_gpio_set_dir (uint8_t pin, uint8_t dir)
  {
#if PICO_ON_DEVICE
  gpio_set_dir (pin, dir);
#else
  printf ("gpio_set_dir: pin=%d dir=%d\n", pin, dir); 
#endif
  }

/*===========================================================================

  interface_sleep_ms

===========================================================================*/
void interface_sleep_ms (uint32_t val)
  {
#if PICO_ON_DEVICE
  sleep_ms (val); 
#else
  usleep (val * 1000);
#endif
  }

/*===========================================================================

  interface_gpio_set_function

===========================================================================*/
extern void interface_gpio_set_function (uint8_t pin, uint8_t function)
  {
#if PICO_ON_DEVICE
  gpio_set_function (pin, function);
#else
  printf ("gpio_set_function: pin=%d func=%d\n", pin, function); 
#endif
  }

/*===========================================================================

  interface_pwm_pin_init

===========================================================================*/
void interface_pwm_pin_init (uint8_t pin)
  {
#if PICO_ON_DEVICE
   gpio_set_function (pin, GPIO_FUNC_PWM);
   uint slice = pwm_gpio_to_slice_num (pin);
   pwm_config config = pwm_get_default_config ();
   pwm_config_set_clkdiv (&config, 8.f);
   pwm_init (slice, &config, true);

#else
  printf ("pwm_pin_init: pin=%d\n", pin); 
#endif
  }

/*===========================================================================

  interface_pwm_pin_set_level

===========================================================================*/
void interface_pwm_pin_set_level (uint8_t pin, uint16_t level)
  {
#if PICO_ON_DEVICE
  pwm_set_gpio_level (pin, level);
#else
  printf ("pwm_pin_set_level: pin=%d level=%d\n", pin, level); 
#endif
  }

/*===========================================================================

  interface_is_interrupt_key

===========================================================================*/
BOOL interface_is_interrupt_key (void)
  {
#if PICO_ON_DEVICE
  if (getchar_timeout_us (0) == I_INTR) return TRUE;
  return FALSE;
#else
  if (getchar() == I_INTR) return TRUE;
  return FALSE;
#endif
  }

/*===========================================================================

  interface_adc_init

===========================================================================*/
void interface_adc_init (void)
  {
#if PICO_ON_DEVICE
  adc_init();
#else
  printf ("ADC init\n");
#endif
  }

/*===========================================================================

  interface_adc_pin_init

===========================================================================*/
void interface_adc_pin_init (uint8_t pin)
  {
#if PICO_ON_DEVICE
  adc_gpio_init (pin);
#else
  printf ("ADC GPIO init, pin=%d\n", pin);
#endif
  }

/*===========================================================================

  interface_adc_select_input

===========================================================================*/
void interface_adc_select_input (uint8_t input)
  {
#if PICO_ON_DEVICE
  adc_select_input (input);
#else
  printf ("ADC select input, input=%d\n", input);
#endif
  }

/*===========================================================================

  interface_i2c_init

===========================================================================*/
extern void interface_i2c_init (uint8_t port, uint32_t baud)
  {
#if PICO_ON_DEVICE
  if (port == 0)
    i2c_init (i2c0, baud);
  else
    i2c_init (i2c1, baud);
#else
  printf ("I2C init, port=%d baud=%d\n", port, baud);
#endif
  }

/*===========================================================================

  interface_adc_get

===========================================================================*/
uint16_t interface_adc_get (void)
  {
#if PICO_ON_DEVICE
  return adc_read(); 
#else
  return 0;
#endif
  }


/*===========================================================================

  interface_i2c_write_read

===========================================================================*/
ErrCode interface_i2c_write_read (uint8_t port, uint8_t addr, 
     uint8_t num_write, const uint8_t *write, 
     uint8_t num_read, uint8_t *read)
  {
#if PICO_ON_DEVICE
  i2c_inst_t *p;
  if (port == 0) 
    p = i2c0;
  else
    p = i2c1;
  if (num_write > 0)
    {
    if (num_read > 0)
      i2c_write_blocking (p, addr, write, num_write, TRUE);
    else
      i2c_write_blocking (p, addr, write, num_write, FALSE);
    }
  if (num_read > 0)
    {
    i2c_read_blocking (p, addr, read, num_read, FALSE);
    }
  return 0;
#else
  (void)num_write; (void)write; 
  (void)num_read; (void)read;
  (void)port; (void)addr;
  for (int i = 0; i < num_read; i++)
    read[i] = 0;
  return 0;
#endif
  }

/*===========================================================================

  interface_cleanup

===========================================================================*/
void interface_cleanup (void)
  {
#if PICO_ON_DEVICE
#else
 tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
#endif
  }

