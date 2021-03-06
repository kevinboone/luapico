/*============================================================================
 * interface.h
 *
 * This file contains definitions that control how the program interacts
 * with a terminal.
 *
 * Copyright (c)2021 Kevin Boone.
 * =========================================================================*/

#pragma once

#include "pico.h"
#include <shell/errcodes.h>
#include <klib/defs.h>
#include <storage/lfs.h>

// Define the ASCII code that the terminal will transmit to erase
//  the last character.
#if PICO_ON_DEVICE
#define I_BACKSPACE 8
#else
#define I_BACKSPACE 127
#endif

// Define the ASCII code that we transmit to the terminal to erase
//  the last character. This isn't necessarily the same as I_BACKSPACE
#if PICO_ON_DEVICE
#define O_BACKSPACE 8
#else
#define O_BACKSPACE 8 
#endif

// Define the ASCII code that the terminal will transmit to delete 
//  the last character (if it sends a code at all -- some transmit a
//  CSI sequence)
#define I_DEL 127

// Serial code for "interrupt" on the terminal. There usually isn't one,
// and we usually hit ctrl+c (ASCII 3)
#define I_INTR 3

// Serial code for "end of input" on the terminal. There usually isn't one,
// and we usually hit ctrl+d (ASCII 4)
#define I_EOI 4

// Define this if your terminal does not rub out the previous character
//   when it receives a backspace from the Pico. This setting has the
//   effect that PMBASIC will send backspace-space-backspace.
#define I_EMIT_DESTRUCTIVE_BACKSPACE

// Define how to send a end-of-line to the terminal. \n=10,
//  \r=13. Some terminals automatically expand a \n into \r\n, but most
//  do not by default.
#define I_ENDL "\r\n"

// The character that indicates an end-of-line -- usually 13 or 10 (or both)
#if PICO_ON_DEVICE
#define I_EOL 13
#else
#define I_EOL 10
#endif

// Time in milliseconds we wait after the escape key is pressed, to see
//   if it's just an escape, or the start of a CSI sequence. There is no
//   right answer here -- it depends on baud rate and other factors. Longer
//   is safer, but might irritate the user
#define I_ESC_TIMEOUT 100

#define INTERFACE_STORAGE_BLOCK_SIZE 4096
//TODO
#define INTERFACE_STORAGE_BLOCK_COUNT 300 

BEGIN_DECLS

extern void  interface_init (void);
extern int   interface_get_char (void);
extern int   interface_get_char_timeout (int msec);
extern void  interface_write_endl (void);
extern void  interface_write_char (char c);
extern void  interface_write_buff (const char *s, int len);
extern void  interface_write_string (const char *s);
extern void  interface_cleanup (void);
extern void  interface_write_stringln (const char *str);

// LittleFS interface functions
extern BOOL interface_block_init ();
extern void interface_block_cleanup ();
extern int  interface_block_sync (const struct lfs_config *cfg);
extern int  interface_block_erase (const struct lfs_config *cfg, 
             lfs_block_t block);
extern int interface_block_prog (const struct lfs_config *cfg, 
             lfs_block_t block, lfs_off_t off, const void *buffer, 
	     lfs_size_t size);
extern int interface_block_read (const struct lfs_config *cfg, 
             lfs_block_t block, lfs_off_t off, void *buffer, 
	     lfs_size_t size);

// Return TRUE is the interrupt key was pressed. Don't block.
extern BOOL interface_is_interrupt_key (void);

extern void interface_adc_init (void);
extern void interface_adc_pin_init (uint8_t pin);
extern void interface_adc_select_input (uint8_t input);
extern uint16_t interface_adc_get (void);

// GPIO functions
extern uint8_t interface_gpio_get (uint8_t pin);
extern void interface_gpio_put (uint8_t pin, uint8_t level);
extern void interface_gpio_set_dir (uint8_t pin, uint8_t dir);
extern void interface_gpio_set_function (uint8_t pin, uint8_t function);
extern void interface_gpio_pull_up (uint8_t pin);

extern void interface_sleep_ms (uint32_t val);

extern void interface_i2c_init (uint8_t port, uint32_t baud);
extern ErrCode interface_i2c_write_read (uint8_t port, uint8_t addr, 
         uint8_t num_write, const uint8_t *write, 
         uint8_t num_read, uint8_t *read);

  
extern void interface_pwm_pin_init (uint8_t pin);
extern void interface_pwm_pin_set_level (uint8_t pin, uint16_t level);

extern ErrCode interface_i2cdetect (uint8_t pin1, uint8_t pin2);

END_DECLS



