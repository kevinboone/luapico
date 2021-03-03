/*==========================================================================

  term.c

  Functions for interacting with ANSI-like terminals

  (c)2021 Kevin Boone, GPLv3.0

==========================================================================*/
#include <string.h> 
#include <ctype.h> 
#include <interface/interface.h>
#include <interface/compat.h>
#include <klib/string.h>
#include <klib/list.h>
#include "../include/klib/term.h"

// ANSI/VT100 control codes
#define TERM_CLEAR "\033[2J"
#define TERM_CLEAREOL "\033[K"
#define TERM_HOME "\033[1;1H"
#define TERM_CUR_BLOCK "\033[?6c"
#define TERM_ERASE_LINE "\033[K"
#define TERM_SET_CURSOR "\033[%d;%dH"
#define TERM_SHOW_CURSOR "\033[?25h"
#define TERM_HIDE_CURSOR "\033[?25l"

#define TERM_ROWS 23
#define TERM_COLS 80

#define TAB_SIZE 8

static BOOL enabled = TRUE;

/*==========================================================================

  term_write_char

==========================================================================*/
static void term_write_char (char c)
  {
  interface_write_char (c);
  }

/*==========================================================================

  term_write_string

==========================================================================*/
static void term_write_string (const char *s)
  {
  interface_write_string (s);
  }

/*==========================================================================

  term_enable

==========================================================================*/
void term_enable (BOOL enable)
  {
  enabled = enable;
  }

/*==========================================================================

  term_get_key 

==========================================================================*/
int term_get_key (void)
  {
  char c = (char)interface_get_char ();
  if (c == '\x1b') 
    {
    char c1 = (char)interface_get_char_timeout (I_ESC_TIMEOUT);
    if (c1 == 0xFF) return VK_ESC;
    //printf ("c1 a =%d %c\n", c1, c1);
    if (c1 == '[') 
      {
      char c2 = (char) interface_get_char ();
      //printf ("c2 a =%d %c\n", c2, c2);
      if (c2 >= '0' && c2 <= '9') 
        {
        char c3 = (char) interface_get_char ();
        //printf ("c3 a =%d %c\n", c3, c3);
        if (c3 == '~') 
          {
          switch (c2) 
            {
            case '0': return VK_END;
            case '1': return VK_HOME;
            case '2': return VK_INS;
            case '3': return VK_DEL; // Usually the key marked "del"
            case '5': return VK_PGUP;
            case '6': return VK_PGDN;
            }
          }
        else if (c3 == ';')
          {
          if (c2 == '1') 
            {
            char c4 = (char) interface_get_char (); // Modifier
            char c5 = (char) interface_get_char (); // Direction
            //printf ("c4 b =%d %c\n", c4, c4);
            //printf ("c5 b =%d %c\n", c5, c5);
            if (c4 == '5') // ctrl
              {
              switch (c5) 
        	{
        	case 'A': return VK_CTRLUP;
        	case 'B': return VK_CTRLDOWN;
        	case 'C': return VK_CTRLRIGHT;
        	case 'D': return VK_CTRLLEFT;
        	case 'H': return VK_CTRLHOME;
        	case 'F': return VK_CTRLEND;
        	}
              }
            else if (c4 == '2') // shift 
              {
              switch (c5) 
        	{
        	case 'A': return VK_SHIFTUP;
        	case 'B': return VK_SHIFTDOWN;
        	case 'C': return VK_SHIFTRIGHT;
        	case 'D': return VK_SHIFTLEFT;
        	case 'H': return VK_SHIFTHOME;
        	case 'F': return VK_SHIFTEND;
        	}
              }
            else if (c4 == '6') // shift-ctrl
              {
              switch (c5) 
        	{
        	case 'A': return VK_CTRLSHIFTUP;
        	case 'B': return VK_CTRLSHIFTDOWN;
        	case 'C': return VK_CTRLSHIFTRIGHT;
        	case 'D': return VK_CTRLSHIFTLEFT;
        	case 'H': return VK_CTRLSHIFTHOME;
        	case 'F': return VK_CTRLSHIFTEND;
        	}
              }
            else 
              {
              // Any other modifier, we don't support. Just return
              //   the raw direction code
              switch (c5) 
        	{
        	case 'A': return VK_UP;
        	case 'B': return VK_DOWN;
        	case 'C': return VK_RIGHT;
        	case 'D': return VK_LEFT;
        	case 'H': return VK_HOME;
        	case 'F': return VK_END;
        	}
              }
            }
          else return VK_ESC;
          }
        }
      else
        {
        //printf ("c2 b =%d %c\n", c2, c2);
        switch (c2) 
          {
          case 'A': return VK_UP;
          case 'B': return VK_DOWN;
          case 'C': return VK_RIGHT;
          case 'D': return VK_LEFT;
          case 'H': return VK_HOME;
          case 'F': return VK_END;
          case 'Z': return VK_SHIFTTAB;
          }
        }
      }
    return '\x1b';
    } 
 else 
    {
    if (c == I_BACKSPACE) return VK_BACK;
    if (c == I_DEL) return VK_DEL;
    if (c == I_INTR) return VK_INTR;
    if (c == I_EOI) return VK_EOI;
    if (c == I_EOL) return VK_ENTER;
    return c;
    } 
  }

/*==========================================================================

  term_clear

==========================================================================*/
void term_clear (void)
  {
  term_write_string (TERM_CLEAR);
  }

/*==========================================================================

  term_show_cursor

==========================================================================*/
void term_show_cursor (void)
  {
  term_write_string (TERM_SHOW_CURSOR);
  }

/*==========================================================================

  term_hide_cursor

==========================================================================*/
void term_hide_cursor (void)
  {
  term_write_string (TERM_HIDE_CURSOR);
  }

/*==========================================================================

  term_clear

==========================================================================*/
void term_clear_and_home (void)
  {
  term_write_string (TERM_CLEAR);
  term_write_string (TERM_HOME);
  }

/*==========================================================================

  term_clear_eol

==========================================================================*/
void term_clear_eol (void)
  {
  term_write_string (TERM_CLEAREOL);
  }

/*==========================================================================

  term_get_size

==========================================================================*/
void term_get_size (uint8_t *rows, uint8_t *cols)
  {
  *rows = TERM_ROWS;
  *cols = TERM_COLS;
  }

/*==========================================================================

  term_set_cursor

==========================================================================*/
void term_set_cursor (uint8_t row, uint8_t col)
  {
  char buff[16];
  sprintf(buff, TERM_SET_CURSOR, row + 1, col + 1);
  term_write_string (buff);
  }

/*==========================================================================

  term_truncate_line

  Truncate a line so that it fits in a certain number of terminal columns,
  allowing for the fact that tabs can expand to multiple spaces

==========================================================================*/
static void term_truncate_line (int columns, char *line, size_t *len)
  {
  uint8_t dlen = 0;
  char *p = line;
  *len = 0;

  while (*p && dlen < columns)
    {
    if (*p == '\t')
      {
      // TODO -- this logic only works with 8-space tabs
      dlen = (uint8_t) (dlen + (uint8_t) TAB_SIZE);
      dlen &= 0xF8;
      }
    else
      dlen++;
    p++;
    (*len)++;
    }
  *p = 0;
  }


/*==========================================================================

  term_write_line

==========================================================================*/
void term_write_line (uint8_t row, const char *line, BOOL truncate)
  {
  char buff[TERM_COLS + 1];
  uint8_t rows, cols;
  size_t len = strlen (line);
  term_get_size (&rows, &cols);
  term_set_cursor (row, 0);
  if (truncate)
    {
    strncpy (buff, line, TERM_COLS);
    buff[TERM_COLS] = 0;
    term_truncate_line (cols, buff, &len); 
    term_write_string (buff);
    }
  else
    {
    term_write_string (line);
    }
  if (len < cols && rows < rows - 1) interface_write_endl();
  }

/*==========================================================================

  term_erase_current_line

==========================================================================*/
void term_erase_current_line (void)
  {
  term_write_string (TERM_ERASE_LINE);
  }

/*===========================================================================

  term_get_displayed_length

===========================================================================*/
uint8_t term_get_displayed_length (const char *line, uint8_t col)
  {
  uint8_t dlen = 0;
  int pos = 0;
  BOOL eol = FALSE;

  while (pos < col)
   {
   if (line[pos] == 0) eol = TRUE;
   if (eol)
     dlen++;
   else
     {
     if (line[pos] == '\t')
        {
        // TODO -- this logic only works with 8-space tabs
        dlen = (uint8_t) (dlen + TAB_SIZE);
        dlen &= 0xF8;
        }
     else
        dlen++;
     }
   pos++;
   }

  return dlen;
  }

/*=========================================================================

  term_add_list_to_history

=========================================================================*/
void term_add_line_to_history (List *history, int max_history, 
        const char *buff)
  {
  BOOL should_add = TRUE;
  int l = list_length (history);
  if (l < max_history)
    {
    for (int i = 0; i < l && should_add; i++)
      {
      if (strcmp (buff, list_get (history, i)) == 0)
        should_add = FALSE;
      }
    }

  if (should_add)
    {
    if (l >= max_history)
      {
      const char *s = list_get (history, 0);
      list_remove_object (history, s);
      }
    list_append (history, strdup (buff));
    }
  }

/*=========================================================================

  term_get_line

=========================================================================*/
BOOL term_get_line (char *buff, int len, BOOL *interrupt, 
       int max_history, List *history)
  {
  int pos = 0;
  BOOL done = 0;
  BOOL got_line = TRUE;
  // The main input buffer
  String *sbuff = string_create_empty ();
  // A copy of the main input buffer, taken when we up-arrow back
  //  into the history. We might need to restore this on a down-arrow
  char *tempstr = NULL;

  int histpos = -1;

  while (!done)
    {
    int c = term_get_key();
    if (c == VK_INTR)
      {
      got_line = TRUE;
      done = TRUE;
      *interrupt = TRUE;
      }
    else if (c == VK_EOI) 
      {
      got_line = FALSE;
      done = TRUE;
      }
    else if (c == VK_DEL || c == VK_BACK)
      {
      if (pos > 0) 
        {
        pos--;
        string_delete_c_at (sbuff, pos);
        term_write_char (O_BACKSPACE);
        const char *s = string_cstr (sbuff);
        int l = string_length (sbuff);
        for (int i = pos; i < l; i++)
          {
          term_write_char (s[i]);
          }
        term_write_char (' ');
        for (int i = pos; i <= l; i++)
          {
          term_write_char (O_BACKSPACE);
          }
        }
      }
    else if (c == VK_ENTER)
      {
      //buff[pos] = 0;
      done = 1;
      }
    else if (c == VK_LEFT)
      {
      if (pos > 0)
        {
        pos--;
        term_write_char (O_BACKSPACE);
        }
      }
    else if (c == VK_CTRLLEFT)
      {
      if (pos == 1)
        {
        pos = 0;
        term_write_char (O_BACKSPACE);
        }
      else
        {
        const char *s = string_cstr (sbuff);
        while (pos > 0 && isspace (s[(pos - 1)]))
          {
          pos--;
          term_write_char (O_BACKSPACE);
          }
        while (pos > 0 && !isspace (s[pos - 1]))
          {
          pos--;
          term_write_char (O_BACKSPACE);
          }
        }
      }
    else if (c == VK_CTRLRIGHT)
      {
      const char *s = string_cstr (sbuff);

      while (s[pos] != 0 && !isspace (s[pos]))
        {
        term_write_char (s[pos]);
        pos++;
        }
      while (s[pos] != 0 && isspace (s[pos]))
        {
        term_write_char (s[pos]);
        pos++;
        }
      }
    else if (c == VK_RIGHT)
      {
      const char *s = string_cstr (sbuff);
      int l = string_length (sbuff);
      if (pos < l)
        {
        term_write_char (s[pos]);
        pos++;
        }
      }
    else if (c == VK_UP)
      {
      if (!history) continue;
      if (histpos == 0) continue;
      int histlen = list_length (history);
      if (histlen == 0) continue;
      //printf ("histlen=%d histpos=%d\n", histlen, histpos);

      if (histpos == -1)
        {
        // We are stepping out of the main line, into the
        //  top of the history
        if (!tempstr)
            tempstr = strdup (string_cstr (sbuff));
        histpos = histlen - 1;
         }
      else
        {
        histpos --;
        }

      int oldlen = string_length (sbuff);
      const char *newline = list_get (history, histpos); 
      int newlen = strlen (newline);
      // Move to the start of the line 
       for (int i = 0; i < pos; i++)
        term_write_char (O_BACKSPACE);
      // Write the new line
      for (int i = 0; i < newlen; i++)
        term_write_char (newline[i]);
      // Erase from the end of the new line to the end of the old 
      for (int i = newlen; i < oldlen; i++)
        term_write_char (' ');
      for (int i = newlen; i < oldlen; i++)
        term_write_char (O_BACKSPACE);
      pos = newlen;
      string_destroy (sbuff);
      sbuff = string_create (newline);
      }
    else if (c == VK_DOWN)
      {
      if (!history) continue;
      int histlen = list_length (history);
      if (histpos < 0) continue; 
      char *newline = "";
      BOOL restored_temp = FALSE;
      if (histpos == histlen - 1)
        {
        // We're about to move off the end of the history, back to 
        //   the main line. So restore the main line, if there is 
        //   one, or just set it to "" if there is not
        histpos = -1;
        if (tempstr)
          {
          newline = tempstr;
          restored_temp = TRUE;
          }
        }
      else
        {
        restored_temp = FALSE;
        histpos++;
        newline = list_get (history, histpos); 
        }

      int oldlen = string_length (sbuff);
      int newlen = strlen (newline);
      // Move to the start of the line 
       for (int i = 0; i < pos; i++)
          term_write_char (O_BACKSPACE);
      // Write the new line
      for (int i = 0; i < newlen; i++)
          term_write_char (newline[i]);
        // Erase from the end of the new line to the end of the old 
      for (int i = newlen; i < oldlen; i++)
          term_write_char (' ');
      for (int i = newlen; i < oldlen; i++)
          term_write_char (O_BACKSPACE);
      pos = newlen;
      string_destroy (sbuff);
      sbuff = string_create (newline);
      if (restored_temp)
        {
        free (tempstr); 
        tempstr = NULL;
        }
      }
    else if (c == VK_HOME)
      {
      if (pos > 0)
        {
        for (int i = 0; i < pos; i++)
          term_write_char (O_BACKSPACE);
        pos = 0;
        }
      }
    else if (c == VK_END)
      {
      const char *s = string_cstr (sbuff);
      int l = string_length (sbuff);
      for (int i = pos; i < l; i++)
          term_write_char (s[i]);
      pos = l;
      }
    else
      {
      int l = string_length (sbuff);

      if (l < len - 1) // Leave room for null
        {
        if ((c < 256 && c >= 32) || (c == 8))
          {
          string_insert_c_at (sbuff, pos, (char)c);
          pos++;
          int l = string_length (sbuff);
          if (pos >= l - 1)
            term_write_char ((char)c); 
          else
            {
            const char *s = string_cstr (sbuff);
            for (int i = pos - 1; i < l; i++)
              term_write_char (s[i]); 
            for (int i = pos; i < l; i++)
              term_write_char (O_BACKSPACE); 
            }
          }
        }
      }
    }

  strncpy (buff, string_cstr(sbuff), len);
  string_destroy (sbuff);
   //printf ("buff='%s'\n", buff);

  if (tempstr) free (tempstr);

  interface_write_endl(); 
  histpos = -1; 

  if (got_line)
    {
    if (history && !(*interrupt) && buff[0])
      {
      term_add_line_to_history (history, max_history, buff);
      }
    return TRUE;
    }
  else
    return FALSE;
  }



