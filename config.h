/*=========================================================================

  picolua

  config.h

  This file defines various limits for picolua, such as line lengths.

=========================================================================*/

#pragma once

// Longest line that can be handled by the mini-editor.
#define EDIT_MAX_LINE 200 

// Longest line that can be entered at the Lua REPL prompt. There's not
//    limit on this, apart from memory, but the line editor gets real
//    clunky when used with long lines over a serial connection
#define LUA_MAXINPUT 200 

// Longest line that can be entered in the pico.readline() function. See
//     above.
#define READLINE_MAXINPUT 200 

#define READLINE_MAX_HISTORY 4

#define MAX_FNAME 255
#define MAX_PATH 255

// Largest file that can be received using XModem. This is to prevent a
//   runaway sender eating the entire storage.
#define XMODEM_MAX 100000

