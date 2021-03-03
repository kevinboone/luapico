/*===========================================================================

  BUTE version 2

  The "Basically Usable Text Editor"

  This editor is based on work by Michael Ringgaard, released under
  a BSD licemce. The original source is here:

  http://www.jbox.dk/downloads/edit.c

  I have tidied it up and extended it to make it more usable in the 
  Pico environment.

===========================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <config.h> 
#include <errcodes.h> 
#include <klib/string.h> 
#include <klib/term.h> 
#include <storage/storage.h>
#include <interface/interface.h>
#include <shell/shell.h>

// Smallest memory allocation for the main buffer
#define MINTEXTBUFFER      8192 

// Extra space at the end of the general line buffer, to allow for
//   inserting format characters, etc.
#define LINEBUF_EXTRA  32

#ifndef TABSIZE
#define TABSIZE        8
#endif

// Amount of space to insert when using the "intent" function
#ifndef INDENT
#define INDENT "  "
#endif

// Terminal codes
// This is nasty -- these really ought to changed to functions interm.c. 
// However, by building them into a line buffer along with the text,
//  it does speed up some display operations considerable. 
// Of course, if we ever support anything except ANSI/VT100 terminals,
//  there's going to be a lot of work to do here.

#define TEXT_COLOR       "\033[0m"
#define SELECT_COLOR     "\033[7m\033[1m"
#define STATUS_COLOR     "\033[1m\033[7m"
#define CLREOL           "\033[K"

// Not implemented yet
#define KEY_CTRL_TAB         0x115

// Handy shortcut for keyboard switch cases
#define CTRL(c) ((c) - 0x60)

//
// Editor data block
//
// The editor uses a buffer that can be split in memory. The structure
//  of the buffer is as follows: 
//
//    +------------------+------------------+------------------+
//    | text before gap  |        gap       |  text after gap  |
//    +------------------+------------------+------------------+
//    ^                  ^                  ^                  ^     
//    |                  |                  |                  |
//  start               gap                rest               end
//

/*===========================================================================

  Data structures

===========================================================================*/

struct _ButeEnv;

struct undo 
  {
  int pos;                 // Editor position
  int erased;              // Size of erased contents
  int inserted;            // Size of inserted contents
  char *undobuf;           // Erased contents for undo
  char *redobuf;           // Inserted contents for redo
  struct undo *next;       // Next undo buffer
  struct undo *prev;       // Previous undo buffer
  };

typedef struct _BUTE
  {
  char *start;              // Start of text buffer
  char *gap;                // Start of gap
  char *rest;               // End of gap
  char *end;                // End of text buffer

  int toppos;                // Text position for current top screen line
  int topline;               // Line number for top of screen
  int margin;                // Position for leftmost column on screen

  int linepos;               // Text position for current line
  int line;                  // Current document line
  int col;                   // Current document column
  int lastcol;               // Column from last horizontal navigation
  int anchor;                // Anchor position for selection
  
  struct undo *undohead;     // Start of undo buffer list
  struct undo *undotail;     // End of undo buffer list
  struct undo *undo;         // Undo/redo boundary

  BOOL refresh;              // Flag to trigger screen redraw
  BOOL lineupdate;           // Flag to trigger redraw of current line
  BOOL dirty;                // Set when the editor buffer has been changed

  BOOL newfile;              // True if this is a new file 

  struct _ButeEnv *env;      // Reference to the editor environment
  struct _BUTE *next;        // Next editor
  struct _BUTE *prev;        // Previous editor

  char filename [MAX_PATH];  // Filename associated with this editor
  } BUTE;

typedef struct _ButeEnv 
  {
  BUTE *current;   // Current editor

  char *clipboard;          // Clipboard
  int clipsize;             // Clipboard size

  char *search;             // Search text
  char *linebuf;            // General-purpose buffer

  uint8_t cols;             // Console columns
  uint8_t lines;            // Console lines
 
  int untitled;             // Counter for untitled files
  } ButeEnv;

void free_undo (BUTE *ed); // FWD

/*==========================================================================

  Interface functions

  The functions in this section interface the editor with the terminal
  handler and shell. 

==========================================================================*/
/*==========================================================================

  mystrerror

==========================================================================*/
static const char *mystrerror (ErrCode err)
  {
  return shell_strerror (err);
  }

/*==========================================================================

  pause_after_message 
 
  Interrupt input for a couple of seconds after displaying an error
  message.

==========================================================================*/
static void pause_after_message (void)
  {
  interface_sleep_ms (2000);
  }

/*==========================================================================

  get_console_size

==========================================================================*/
static void get_console_size (ButeEnv *env) 
  {
  term_get_size (&env->lines, &env->cols);
  }

/*==========================================================================

  interface_write_stringln

  Helper function to tidy up writing strings with end-of-line

==========================================================================*/
void interface_write_stringln (const char *str) 
  {
  interface_write_string (str); 
  interface_write_endl();
  }

/*==========================================================================

  Editor constructor and destructor 

==========================================================================*/
/*==========================================================================

  bute_create 

==========================================================================*/
BUTE *bute_create (ButeEnv *env) 
  {
  BUTE *ed = (BUTE *) malloc(sizeof(BUTE));
  memset(ed, 0, sizeof(BUTE));
  if (env->current) 
    {
    ed->next = env->current->next;
    ed->prev = env->current;
    env->current->next->prev = ed;
    env->current->next = ed;
    } 
  else 
    {
    ed->next = ed->prev = ed;
    }
  ed->env = env;
  env->current = ed;
  return ed;
  }

/*==========================================================================

  bute_destroy 

==========================================================================*/
void bute_destroy (BUTE *self) 
  {
  if (self->next == self) 
    {
    self->env->current = NULL;
    } 
  else 
    {
    self->env->current = self->prev;
    }
  self->next->prev = self->prev;
  self->prev->next = self->next;
  if (self->start) free (self->start);
  free_undo (self);
  free (self);
  }

/*==========================================================================

  Editor buffer manipulation functions

==========================================================================*/
/*==========================================================================

  free_undo 

  Clear memory associated with the undo buffer chain

==========================================================================*/
void free_undo (BUTE *ed) 
  {
  struct undo *undo = ed->undohead;
  while (undo) 
    {
    struct undo *next = undo->next;
    free(undo->undobuf);
    free(undo->redobuf);
    free(undo);
    undo = next;
    }
  ed->undohead = ed->undotail = ed->undo = NULL;
  }

/*==========================================================================

  reset_undo

==========================================================================*/
void reset_undo (BUTE *ed) 
  {
  while (ed->undotail != ed->undo) 
    {
    struct undo *undo = ed->undotail;
    if (!undo) 
      {
      ed->undohead = NULL;
      ed->undotail = NULL;
      break;
      }
    ed->undotail = undo->prev;
    if (undo->prev) undo->prev->next = NULL;
    free (undo->undobuf);
    free (undo->redobuf);
    free (undo);
    }
  ed->undo = ed->undotail;
  }

/*==========================================================================

  find_editor

  Find the editor object that matches a specific filename, or NULL
  if there is not one 

==========================================================================*/
BUTE *find_editor (ButeEnv *env, char *filename) 
  {
  char fn[MAX_PATH];
  BUTE *ed = env->current;
  BUTE *start = ed;
  
  strcpy(fn, filename);

  do 
    {
    if (strcmp(fn, ed->filename) == 0) return ed;
    ed = ed->next;
    } while (ed != start);
  return NULL;  
  }

/*==========================================================================

  new_file

  Initialize an editor with a new filename, or set the file to "unititled"
  if the filename is NULL. This method can fail if there is insufficient
  memory.

==========================================================================*/
ErrCode new_file (BUTE *ed, const char *filename) 
  {
  if (*filename) 
    {
    strcpy (ed->filename, filename);
    } 
  else 
    {
    sprintf (ed->filename, "Untitled-%d", ++ed->env->untitled);
    ed->newfile = 1;
    }

  ed->start = (char *) malloc(MINTEXTBUFFER);
  if (!ed->start) return ERR_NOMEM;

  ed->gap = ed->start;
  ed->rest = ed->end = ed->gap + MINTEXTBUFFER;
  ed->anchor = -1;
  
  return 0;
  }

/*==========================================================================

  load_file

  Load a file into the editor, if it exists. Return an error code if
  the file cannot be read.

==========================================================================*/
ErrCode load_file (BUTE *ed, const char *filename) 
  {
  int length;
  char *buff;
  ErrCode ret = storage_read_file (filename, (uint8_t**) &buff, &length);
  if (ret == 0)
    {
    buff = realloc (buff, length + MINTEXTBUFFER); 
    ed->start = buff;
    ed->gap = ed->start + length;
    ed->rest = ed->end = ed->gap + MINTEXTBUFFER;
    ed->anchor = -1;
    strncpy (ed->filename, filename, sizeof (ed->filename));
    }
  return ret;
  }

/*==========================================================================

  save_file

  Save the current buffer, if possible, and set the status to clean

==========================================================================*/
ErrCode save_file (BUTE *ed) 
  {
  ErrCode ret = 0;
  int size1 = ed->gap - ed->start;
  int size2 = ed->end - ed->rest;
  int total = size1 + size2;
  char *buff = malloc (total);
  if (buff)
    {
    memcpy (buff, ed->start, size1);
    memcpy (buff + size1, ed->rest, size2);

    ret = storage_write_file (ed->filename, (uint8_t**) buff, total);
    free (buff);
    }
  else
    ret = ERR_NOMEM;

  if (ret == 0)
    {
    ed->dirty = FALSE;
    free_undo (ed);
    }
  return ret;
  }

/*==========================================================================

  text_length

  Returns the total length of text in the editor, allowing for the
  split buffer

==========================================================================*/
static int text_length (const BUTE *ed) 
  {
  return (ed->gap - ed->start) + (ed->end - ed->rest);
  }

/*==========================================================================

  text_ptr

  Find the location in the buffer that corresponds to a specific
  character position, bearing in mind that that buffer might be
  split

==========================================================================*/
static char *text_ptr (const BUTE *ed, int pos) 
  {
  char *p = ed->start + pos;
  if (p >= ed->gap) p += (ed->rest - ed->gap);
  return p;
  }

/*==========================================================================

  move_gap

==========================================================================*/
void move_gap (BUTE *ed, int pos, int minsize) 
  {
  int gapsize = ed->rest - ed->gap;
  char *p = text_ptr(ed, pos);
  if (minsize < 0) minsize = 0;

  if (minsize <= gapsize) 
    {
    if (p != ed->rest) 
      {
      if (p < ed->gap) 
        {
        memmove(p + gapsize, p, ed->gap - p);
        } 
      else 
        {
        memmove(ed->gap, ed->rest, p - ed->rest);
        }
      ed->gap = ed->start + pos;
      ed->rest = ed->gap + gapsize;
      }
    } 
  else 
    {
    int newsize;
    char *start;
    char *gap;
    char *rest;
    char *end;

    if (gapsize + MINTEXTBUFFER > minsize) minsize = gapsize + MINTEXTBUFFER;
    newsize = (ed->end - ed->start) - gapsize + minsize;
    start = (char *) malloc(newsize); // TODO check for out of memory
    gap = start + pos;
    rest = gap + minsize;
    end = start + newsize;

    if (p < ed->gap) 
      {
      memcpy(start, ed->start, pos);
      memcpy(rest, p, ed->gap - p);
      memcpy(end - (ed->end - ed->rest), ed->rest, ed->end - ed->rest);
      } 
    else 
      {
      memcpy(start, ed->start, ed->gap - ed->start);
      memcpy(start + (ed->gap - ed->start), ed->rest, p - ed->rest);
      memcpy(rest, p, ed->end - p);
      }

    free(ed->start);
    ed->start = start;
    ed->gap = gap;
    ed->rest = rest;
    ed->end = end;
    }
  }

/*==========================================================================

  close_gap 

==========================================================================*/
void close_gap (BUTE *ed) 
  {
  int len = text_length (ed);
  move_gap (ed, len, 1);
  ed->start[len] = 0;
  }

/*==========================================================================

  get_char

  Get the character at the specific position in the buffer, allowing 
  for the split.

  NOTE: there's something odd about signed character tests in the
  ARM compiler. This function should really return a char, but setting
  it to -1 seems to have strange results in comparisons like "< 0". 
  The behaviour is correct in the x86 compiler. Need to be a bit careful
  about this kind of thing, because debugging problems like this on the
  board is a bear.

==========================================================================*/
int get_char (const BUTE *ed, int pos) 
  {
  char *p = text_ptr (ed, pos);
  if (p >= ed->end) return -1;
  return *p;
  }

/*==========================================================================

  compare

  Return TRUE if the text at the current position matches the supplied
  buffer. At present, this is only used by the indentation functions.

==========================================================================*/
BOOL compare (const BUTE *ed, char *buf, int pos, int len) 
  {
  char *bufptr = buf;
  char *p = ed->start + pos;
  if (p >= ed->gap) p += (ed->rest - ed->gap);

  while (len > 0) 
    {
    if (p == ed->end) return FALSE;
    if (*bufptr++ != *p) return FALSE;
    len--;
    if (++p == ed->gap) p = ed->rest;
    }

  return TRUE;
  }

/*==========================================================================

  copy

  Copy the specified text into the buffer, allowing for the gap

==========================================================================*/
int copy (const BUTE *ed, char *buf, int pos, int len) 
  {
  char *bufptr = buf;
  char *p = ed->start + pos;
  if (p >= ed->gap) p += (ed->rest - ed->gap);

  while (len > 0) 
    {
    if (p == ed->end) break;
    *bufptr++ = *p;
    len--;
    if (++p == ed->gap) p = ed->rest;
    }

  return bufptr - buf;
  }

/*==========================================================================

  replace

==========================================================================*/
static void replace (BUTE *ed, int pos, int len, char *buf, 
       int bufsize, BOOL do_undo) 
  {
  char *p;

  // Store undo information
  if (do_undo) 
    {
    reset_undo (ed);
    struct undo *undo = ed->undotail;
    if (undo && len == 0 && bufsize == 1 
          && undo->erased == 0 && pos == undo->pos + undo->inserted) 
      {
      // Insert character at end of current redo buffer
      undo->redobuf = realloc(undo->redobuf, undo->inserted + 1);
      undo->redobuf[undo->inserted] = *buf;
      undo->inserted++;
      } 
    else if (undo && len == 1 && bufsize == 0 && undo->inserted == 0 
           && pos == undo->pos) 
      {
      // Erase character at end of current undo buffer
      undo->undobuf = realloc(undo->undobuf, undo->erased + 1);
      undo->undobuf[undo->erased] = get_char(ed, pos);
      undo->erased++;
      } 
    else if (undo && len == 1 && bufsize == 0 && undo->inserted == 0 
         && pos == undo->pos - 1) 
      {
      // Erase character at beginning of current undo buffer
      undo->pos--;
      undo->undobuf = realloc(undo->undobuf, undo->erased + 1);
      memmove(undo->undobuf + 1, undo->undobuf, undo->erased);
      undo->undobuf[0] = get_char(ed, pos);
      undo->erased++;
      } 
    else 
      {
      // Create new undo buffer
      undo = (struct undo *) malloc(sizeof(struct undo));
      if (ed->undotail) ed->undotail->next = undo;
      undo->prev = ed->undotail;
      undo->next = NULL;
      ed->undotail = ed->undo = undo;
      if (!ed->undohead) ed->undohead = undo;

      undo->pos = pos;
      undo->erased = len;
      undo->inserted = bufsize;
      undo->undobuf = undo->redobuf = NULL;
      if (len > 0) 
        {
        undo->undobuf = malloc(len);
        copy(ed, undo->undobuf, pos, len);      
        }
      if (bufsize > 0) 
        {
        undo->redobuf = malloc(bufsize);
        memcpy(undo->redobuf, buf, bufsize);
        }
      }
    }

  p = ed->start + pos;
  if (bufsize == 0 && p <= ed->gap && p + len >= ed->gap)  
    {
    // Handle deletions at the edges of the gap
    ed->rest += len - (ed->gap - p);
    ed->gap = p;
    } 
  else 
    {
    // Move the gap
    move_gap(ed, pos + len, bufsize - len);

    // Replace contents
    memcpy(ed->start + pos, buf, bufsize);
    ed->gap = ed->start + pos + bufsize;
    }

    // Mark buffer as dirty
  ed->dirty = 1;
  }

/*==========================================================================

  insert

==========================================================================*/
void insert (BUTE *ed, int pos, char *buf, int bufsize) 
  {
  replace (ed, pos, 0, buf, bufsize, 1);
  }

/*==========================================================================

  erase 

==========================================================================*/
void erase (BUTE *ed, int pos, int len) 
  {
  replace (ed, pos, len, NULL, 0, 1);
  }

/*==========================================================================

  Navigation functions 

==========================================================================*/
/*==========================================================================

  line_length

  Return the length of the remaining line from the specified
  position

==========================================================================*/
int line_length (const BUTE *ed, int linepos) 
  {
  int pos = linepos;
  while (1) 
    {
    int ch = get_char (ed, pos);
    if (ch < 0 || ch == '\n') break;
    pos++;
    }

  return pos - linepos;
  }

/*==========================================================================

  line_start

  Return the position of the start of the line containing the specified
  position

==========================================================================*/
int line_start (const BUTE *ed, int pos) 
  {
  while (1) 
    {
    if (pos == 0) break;
    if (get_char(ed, pos - 1) == '\n') break;
    pos--;
    }

  return pos;
  }

/*==========================================================================

  next_line 

  Return the position of the start of the next line after the 
  line containing the specified position

==========================================================================*/
int next_line (const BUTE *ed, int pos) 
  {
  while (1) 
    {
    int ch = get_char (ed, pos);
    if (ch < 0) return -1;
    pos++;
    if (ch == '\n') return pos;
    }
  }

/*==========================================================================

  prev_line 

  Return the position of the start of the line before the line 
  containing the specified position

==========================================================================*/
int prev_line (const BUTE *ed, int pos) 
  {
  if (pos == 0) return -1;

  while (pos > 0) 
    {
    int ch = get_char(ed, --pos);
    if (ch == '\n') break;
    }

  while (pos > 0) 
    {
    int ch = get_char(ed, --pos);
    if (ch == '\n') return pos + 1;
    }

  return 0;
  }

/*==========================================================================

  column

  Get the screen column contain corresponding to the position on the
  specified line, allowing for tabs.

==========================================================================*/
int column (const BUTE *ed, int linepos, int col) 
  {
  char *p = text_ptr (ed, linepos);
  int c = 0;
  while (col > 0) 
    {
    if (p == ed->end) break;
    if (*p == '\t') 
      {
      int spaces = TABSIZE - c % TABSIZE;
      c += spaces;
      } 
    else 
      {
      c++;
      }
    col--;
    if (++p == ed->gap) p = ed->rest;
    }
  return c;
  }

/*==========================================================================

  moveto

  Set the editing point to the specified character location in
  the buffer. Adjust the screen so that this point is visible.

==========================================================================*/
void moveto (BUTE *ed, int pos, int center) 
  {
  int scroll = 0;
  for (;;) 
    {
    int cur = ed->linepos + ed->col;
    if (pos < cur) 
      {
      if (pos >= ed->linepos) 
        {
        ed->col = pos - ed->linepos;
        } 
      else 
        {
        ed->col = 0;
        ed->linepos = prev_line(ed, ed->linepos);
        ed->line--;

        if (ed->topline > ed->line) 
          {
          ed->toppos = ed->linepos;
          ed->topline--;
          ed->refresh = 1;
          scroll = 1;
          }
        }
      } 
    else if (pos > cur) 
      {
      int next = next_line(ed, ed->linepos);
      if (next == -1) 
        {
        ed->col = line_length(ed, ed->linepos);
        break;
        } 
      else if (pos < next) 
        {
        ed->col = pos - ed->linepos;
        } 
      else 
        {
        ed->col = 0;
        ed->linepos = next;
        ed->line++;

        if (ed->line >= ed->topline + ed->env->lines) 
          {
          ed->toppos = next_line(ed, ed->toppos);
          ed->topline++;
          ed->refresh = 1;
          scroll = 1;
          }
        }
      } 
    else 
      {
      break;
      }
    }

  if (scroll && center) 
    {
    int tl = ed->line - ed->env->lines / 2;
    if (tl < 0) tl = 0;
    for (;;) 
      {
      if (ed->topline > tl) 
        {
        ed->toppos = prev_line(ed, ed->toppos);
        ed->topline--;
        } 
      else if (ed->topline < tl) 
        {
        ed->toppos = next_line(ed, ed->toppos);
        ed->topline++;
        } 
      else 
        {
        break;
        }
      }
    }
  }

/*==========================================================================

  Selection management functions 

==========================================================================*/
/*==========================================================================

  get_selection 

  Get the start and end positions of the selection. Returns FALSE
  if there is no selection.

==========================================================================*/
BOOL get_selection (const BUTE *ed, int *start, int *end) 
  {
  if (ed->anchor == -1) 
    {
    *start = *end = -1;
    return FALSE;
    } 
  else 
    {
    int pos = ed->linepos + ed->col;
    if (pos == ed->anchor) 
      {
      *start = *end = -1;
      return FALSE;
      } 
    else if (pos < ed->anchor) 
      {
      *start = pos;
      *end = ed->anchor;
      } 
    else 
      {
      *start = ed->anchor;
      *end = pos;
      }
    }
  return TRUE;
  }

/*==========================================================================

  get_selected_text

  Copies the text between the start and end of selection, if there is one,
  into the specified buffer. Returns the length of text copied, which 
  will be zero if there is no selection.

==========================================================================*/
int get_selected_text (const BUTE *ed, char *buffer, int size) 
  {
  int selstart, selend;

  if (!get_selection (ed, &selstart, &selend)) return 0;

  int len = selend - selstart;
  if (len >= size) return 0;
  copy(ed, buffer, selstart, len);
  buffer[len] = 0;
  return len;
  }

/*==========================================================================

  Update selection

==========================================================================*/
void update_selection (BUTE *ed, int select) 
  {
  if (select) 
    {
    if (ed->anchor == -1) ed->anchor = ed->linepos + ed->col;
    ed->refresh = 1;
    } 
  else 
    {
    if (ed->anchor != -1) ed->refresh = 1;
    ed->anchor = -1;
    }
  }

/*==========================================================================

  erase_selection 

==========================================================================*/
BOOL erase_selection (BUTE *ed) 
  {
  int selstart, selend;
  
  if (!get_selection(ed, &selstart, &selend)) return FALSE;
  moveto (ed, selstart, 0);
  erase (ed, selstart, selend - selstart);
  ed->anchor = -1;
  ed->refresh = 1;
  return TRUE;
  }

/*==========================================================================

  select_all 

==========================================================================*/
static void select_all (BUTE *ed) 
  {
  ed->anchor = 0;
  ed->refresh = 1;
  moveto(ed, text_length(ed), 0);
  }

/*==========================================================================

  prompt 
  
  Prompt the user and accept a string of text. Returns TRUE if the
  user did not abort by hitting EOI or Interrupt

==========================================================================*/
BOOL prompt (const BUTE *ed, char *msg) 
  {
  char *buf = ed->env->linebuf;

  term_set_cursor (ed->env->lines, 0);
  interface_write_string (STATUS_COLOR);
  interface_write_string (msg);
  term_clear_eol();

  BOOL interrupt = FALSE;
  BOOL ret = term_get_line (buf, 50, &interrupt, 0, NULL);

  if (!ret) return FALSE;
  if (interrupt) return FALSE;
  return TRUE;
  }


/*==========================================================================

  says_yes

  Returns true if the user hits 'y' or 'Y', otherwise false. Used in
  "y/n" prompts.

==========================================================================*/
int says_yes() 
  {
  int ch = term_get_key();
  return ch == 'y' || ch == 'Y';
  }

/*==========================================================================

  display_message

  Writes a message on the status line

==========================================================================*/
void display_message (BUTE *ed, const char *fmt, ...) 
  {
  va_list args;

  va_start (args, fmt);
  term_set_cursor (ed->env->lines, 0);
  interface_write_string (STATUS_COLOR);
  vprintf (fmt, args);
  term_clear_eol();
  interface_write_string (TEXT_COLOR);
  va_end (args);
  }

/*==========================================================================

  draw_full_statusline

  Draws the whole status line, allowing space for the filenane and position

==========================================================================*/
void draw_full_statusline (BUTE *ed) 
  {
  ButeEnv *env = ed->env;
  int namewidth = env->cols - 32;
  term_set_cursor (env->lines, 0);
  term_hide_cursor();
  interface_write_string (STATUS_COLOR);
  sprintf (env->linebuf,
    "%*.*sHelp:ctrl-@ %c Ln %-6dCol %-4d", -namewidth, 
    namewidth, ed->filename, ed->dirty ? '*' : ' ', ed->line + 1, 
    column (ed, ed->linepos, ed->col) + 1);
  interface_write_string (env->linebuf);
  term_clear_eol(); 
  interface_write_string (TEXT_COLOR);
  term_show_cursor(); 
  }

/*==========================================================================

  draw_statusline

  Draws the part of the status line that contains the file position 

==========================================================================*/
void draw_statusline(BUTE *ed) 
  {
  term_hide_cursor();
  interface_write_string (STATUS_COLOR);
  term_set_cursor (ed->env->lines, ed->env->cols - 20);
  sprintf (ed->env->linebuf, "%c Ln %-6dCol %-4d" , 
      ed->dirty ? '*' : ' ', ed->line + 1, 
      column (ed, ed->linepos, ed->col) + 1);
  interface_write_string(ed->env->linebuf);
  term_clear_eol(); 
  interface_write_string (TEXT_COLOR);
  term_show_cursor(); 
  }

/*==========================================================================

  display_line

  Draws a line, including selection highlight where appropriate.

==========================================================================*/
void display_line (const BUTE *ed, int pos, int fullline) 
  {
  BOOL highlight = FALSE;
  int col = 0;
  int margin = ed->margin;
  int maxcol = ed->env->cols + margin;
  char *bufptr = ed->env->linebuf;
  char *p = text_ptr(ed, pos);
  char *s;

  int selstart, selend, ch;
  get_selection (ed, &selstart, &selend);
  while (col < maxcol) 
    {
    if (margin == 0) 
      {
      if (!highlight && pos >= selstart && pos < selend) 
        {
        for (s = SELECT_COLOR; *s; s++) *bufptr++ = *s;
        highlight = TRUE;
        } 
      else if (highlight && pos >= selend) 
        {
        for (s = TEXT_COLOR; *s; s++) *bufptr++ = *s;
        highlight = FALSE;
        }
      }

    if (p == ed->end) break;
    ch = *p;
    if (ch == '\n') break;

    if (ch == '\t') 
      {
      int spaces = TABSIZE - col % TABSIZE;
      while (spaces > 0 && col < maxcol) 
        {
        if (margin > 0) 
          {
          margin--;
          } 
        else 
          {
          *bufptr++ = ' ';
          }
        col++;
        spaces--;
        }
      } 
    else 
      {
      if (margin > 0) 
        {
        margin--;
        } 
      else 
        {
        *bufptr++ = ch;
        }
      col++;
      }

    if (++p == ed->gap) p = ed->rest;
    pos++;
    }

  if (highlight) 
    {
    while (col < maxcol) 
      {
      *bufptr++ = ' ';
      col++;
      }
    } 
  else 
    {
    if (col == margin) *bufptr++ = ' ';
    }

  if (col < maxcol) 
    {
    for (s = CLREOL; *s; s++) *bufptr++ = *s;
    if (fullline) 
      {
      memcpy (bufptr, "\n", 2);
      bufptr += 2;
      }
    }

  if (highlight) 
    {
    for (s = TEXT_COLOR; *s; s++) *bufptr++ = *s;
    }

  interface_write_buff (ed->env->linebuf, bufptr - ed->env->linebuf);
  }

/*==========================================================================

  update_line

==========================================================================*/
void update_line (const BUTE *ed) 
  {
  term_set_cursor (ed->line - ed->topline, 0);
  display_line (ed, ed->linepos, 0);
  }

/*==========================================================================

  draw_screen 

==========================================================================*/
void draw_screen (const BUTE *ed) 
  {
  term_set_cursor (0, 0);
  interface_write_string (TEXT_COLOR);
  int pos = ed->toppos;

  for (int i = 0; i < ed->env->lines; i++) 
    {
    if (pos < 0) 
      {
      term_clear_eol();
      interface_write_endl();
      } 
    else 
      {
      display_line (ed, pos, 1);
      pos = next_line (ed, pos);
      }
    }
  term_show_cursor();
  }

/*==========================================================================

  position_cursor 

  Position the terminal cursor on the basis of the file line and column

==========================================================================*/
void position_cursor (const BUTE *ed) 
  {
  int col = column (ed, ed->linepos, ed->col);
  term_set_cursor (ed->line - ed->topline, col - ed->margin);
  }

/*==========================================================================

  adjust_layout

  Adjusts the whole-screen position, according to the position of the
  cursor on the current line.

==========================================================================*/
void adjust_layout (BUTE *ed) 
  {
  int ll = line_length (ed, ed->linepos);
  ed->col = ed->lastcol;
  if (ed->col > ll) ed->col = ll;

  int col = column (ed, ed->linepos, ed->col);
  while (col < ed->margin) 
    {
    ed->margin -= 4;
    if (ed->margin < 0) ed->margin = 0;
    ed->refresh = 1;
    }

  while (col - ed->margin >= ed->env->cols) 
    {
    ed->margin += 4;
    ed->refresh = 1;
    }
  }

/*==========================================================================

  up

==========================================================================*/
void up (BUTE *ed, int select) 
  {
  update_selection (ed, select);

  int newpos = prev_line (ed, ed->linepos);
  if (newpos < 0) return;

  ed->linepos = newpos;
  ed->line--;
  if (ed->line < ed->topline) 
    {
    ed->toppos = ed->linepos;
    ed->topline = ed->line;
    ed->refresh = 1;
    }

  adjust_layout (ed);
  }

/*==========================================================================

  up

==========================================================================*/
void down (BUTE *ed, BOOL select) 
  {
  update_selection (ed, select);

  int newpos = next_line(ed, ed->linepos);
  if (newpos < 0) return;

  ed->linepos = newpos;
  ed->line++;

  if (ed->line >= ed->topline + ed->env->lines) 
    {
    ed->toppos = next_line(ed, ed->toppos);
    ed->topline++;
    ed->refresh = 1;
    }

  adjust_layout(ed);
  }

/*==========================================================================

 left 

==========================================================================*/
void left (BUTE *ed, int select) 
  {
  update_selection (ed, select);
  if (ed->col > 0) 
    {
    ed->col--;
    } 
   else 
    {
    int newpos = prev_line (ed, ed->linepos);
    if (newpos < 0) return;

    ed->col = line_length(ed, newpos);
    ed->linepos = newpos;
    ed->line--;
    if (ed->line < ed->topline) 
      {
      ed->toppos = ed->linepos;
      ed->topline = ed->line;
      ed->refresh = 1;
      }
    }

  ed->lastcol = ed->col;
  adjust_layout (ed);
  }

/*==========================================================================

 right 

==========================================================================*/
void right (BUTE *ed, int select) 
  {
  update_selection(ed, select);
  if (ed->col < line_length(ed, ed->linepos)) 
    {
    ed->col++;
    } 
  else 
    {
    int newpos = next_line(ed, ed->linepos);
    if (newpos < 0) return;

    ed->col = 0;
    ed->linepos = newpos;
    ed->line++;

    if (ed->line >= ed->topline + ed->env->lines) 
      {
      ed->toppos = next_line(ed, ed->toppos);
      ed->topline++;
      ed->refresh = 1;
      }
    }

  ed->lastcol = ed->col;
  adjust_layout(ed);
  }

/*==========================================================================

  wordleft

==========================================================================*/
void wordleft (BUTE *ed, int select) 
  {
  update_selection(ed, select);
  int pos = ed->linepos + ed->col;
  int phase = 0;
  while (pos > 0) 
    {
    char ch = get_char (ed, pos - 1);
    if (phase == 0) 
      {
      if (isalnum (ch)) phase = 1;
      } 
    else 
      {
      if (!isalnum (ch)) break;
      }

    pos--;
    if (pos < ed->linepos) 
      {
      ed->linepos = prev_line(ed, ed->linepos);
      ed->line--;
      ed->refresh = 1;
      }
    }
  ed->col = pos - ed->linepos;
  if (ed->line < ed->topline) 
    {
    ed->toppos = ed->linepos;
    ed->topline = ed->line;
    }

  ed->lastcol = ed->col;
  adjust_layout (ed);
  }

/*==========================================================================

  wordright

==========================================================================*/
void wordright (BUTE *ed, int select) 
  {
  update_selection (ed, select);
  int pos = ed->linepos + ed->col;
  int end = text_length(ed);
  int next = next_line(ed, ed->linepos);
  int phase = 0;
  while (pos < end)   
    {
    int ch = get_char(ed, pos);
    if (phase == 0) 
      {
      if (isalnum(ch)) phase = 1;
      } 
    else 
      {
      if (!isalnum(ch)) break;
      }

    pos++;
    if (pos == next) 
      {
      ed->linepos = next;
      next = next_line(ed, ed->linepos);
      ed->line++;
      ed->refresh = 1;
      }
    }
  ed->col = pos - ed->linepos;
  if (ed->line >= ed->topline + ed->env->lines) 
    {
    ed->toppos = next_line(ed, ed->toppos);
    ed->topline++;
    }

  ed->lastcol = ed->col;
  adjust_layout(ed);
  }

/*==========================================================================

  home 

==========================================================================*/
void home (BUTE *ed, int select) 
  {
  update_selection (ed, select);
  ed->col = ed->lastcol = 0;
  adjust_layout (ed);
  }

/*==========================================================================

  end 

==========================================================================*/
void end (BUTE *ed, int select) 
  {
  update_selection (ed, select);
  ed->col = ed->lastcol = line_length (ed, ed->linepos);
  adjust_layout (ed);
  }

/*==========================================================================

  top 

==========================================================================*/
void top (BUTE *ed, int select) 
  {
  update_selection(ed, select);
  ed->toppos = ed->topline = ed->margin = 0;
  ed->linepos = ed->line = ed->col = ed->lastcol = 0;
  ed->refresh = TRUE;
  }

/*==========================================================================

  bottom 

==========================================================================*/
void bottom (BUTE *ed, int select) 
  {
  update_selection (ed, select);
  for (;;) 
    {
    int newpos = next_line (ed, ed->linepos);
    if (newpos < 0) break;

    ed->linepos = newpos;
    ed->line++;

    if (ed->line >= ed->topline + ed->env->lines) 
      {
      ed->toppos = next_line (ed, ed->toppos);
      ed->topline++;
      ed->refresh = TRUE;
      }
    }
  ed->col = ed->lastcol = line_length (ed, ed->linepos);
  adjust_layout(ed);
  }

/*==========================================================================

  bottom 

==========================================================================*/
void pageup (BUTE *ed, int select) 
  {
  update_selection (ed, select);
  if (ed->line < ed->env->lines) 
    {
    ed->linepos = ed->toppos = 0;
    ed->line = ed->topline = 0;
    } 
   else 
    {
    for (int i = 0; i < ed->env->lines; i++) 
      {
      int newpos = prev_line (ed, ed->linepos);
      if (newpos < 0) return;

      ed->linepos = newpos;
      ed->line--;

      if (ed->topline > 0) 
        {
        ed->toppos = prev_line(ed, ed->toppos);
        ed->topline--;
        }
      }
    }

  ed->refresh = TRUE;
  adjust_layout(ed);
  }

/*==========================================================================

  pagedown 

==========================================================================*/
void pagedown (BUTE *ed, int select) 
  {
  update_selection (ed, select);
  for (int i = 0; i < ed->env->lines; i++) 
    {
    int newpos = next_line(ed, ed->linepos);
    if (newpos < 0) break;

    ed->linepos = newpos;
    ed->line++;

    ed->toppos = next_line(ed, ed->toppos);
    ed->topline++;
    }

  ed->refresh = TRUE;
  adjust_layout(ed);
  }

/*==========================================================================

  Text editing functions 

==========================================================================*/
/*==========================================================================

  insert_char 

==========================================================================*/
void insert_char (BUTE *ed, char ch) 
  {
  erase_selection (ed);
  insert (ed, ed->linepos + ed->col, &ch, 1);
  ed->col++;
  ed->lastcol = ed->col;
  adjust_layout(ed);
  if (!ed->refresh) ed->lineupdate = TRUE;
  }

/*==========================================================================

  newline 

==========================================================================*/
void newline (BUTE *ed) 
  {
  erase_selection (ed);
  insert (ed, ed->linepos + ed->col, "\n", 1);
  ed->col = ed->lastcol = 0;
  ed->line++;
  int p = ed->linepos;
  ed->linepos = next_line (ed, ed->linepos);
  for (;;) 
    {
    char ch = get_char (ed, p++);
    if (ch == ' ' || ch == '\t') 
      {
      insert (ed, ed->linepos + ed->col, &ch, 1);
      ed->col++;
      } 
     else 
      {
      break;
      }
    }

  ed->lastcol = ed->col;
  ed->refresh = TRUE;

  if (ed->line >= ed->topline + ed->env->lines) 
    {
    ed->toppos = next_line (ed, ed->toppos);
    ed->topline++;
    ed->refresh = TRUE;
    }

  adjust_layout (ed);
  }

/*==========================================================================

  newline 

==========================================================================*/
void backspace (BUTE *ed) 
  {
  if (erase_selection(ed)) return;
  if (ed->linepos + ed->col == 0) return;
  if (ed->col == 0) 
    {
    int pos = ed->linepos;
    erase(ed, --pos, 1);

    ed->line--;
    ed->linepos = line_start(ed, pos);
    ed->col = pos - ed->linepos;
    ed->refresh = 1;

    if (ed->line < ed->topline) 
      {
      ed->toppos = ed->linepos;
      ed->topline = ed->line;
      }
    } 
  else 
    {
    ed->col--;
    erase(ed, ed->linepos + ed->col, 1);
    ed->lineupdate = 1;
    }

  ed->lastcol = ed->col;
  adjust_layout(ed);
  }

/*==========================================================================

  del 

==========================================================================*/
void del (BUTE *ed) 
  {
  if (erase_selection(ed)) return;
  int pos = ed->linepos + ed->col;
  int ch = get_char (ed, pos); // Should be "char" but for a weird bug
  if (ch < 0) return;

  erase (ed, pos, 1);

  if (ch == '\n') 
    {
    ed->refresh = TRUE;
    } 
  else 
    {
    ed->lineupdate = TRUE;
    }
  }

/*==========================================================================

  indent 

==========================================================================*/
void indent (BUTE *ed, char *indentation) 
  {
  char *buffer, *p;
  int buflen;
  int width = strlen(indentation);
  int pos = ed->linepos + ed->col;

  int start, end;
  if (!get_selection(ed, &start, &end)) 
    {
    insert_char(ed, '\t');
    return;
    }

  int lines = 0;
  int toplines = 0;
  BOOL newline = TRUE;
  for (int i = start; i < end; i++) 
    {
    if (i == ed->toppos) toplines = lines;
    if (newline) 
      {
      lines++;
      newline = 0;
      }
    if (get_char(ed, i) == '\n') newline = 1;
    }

  buflen = end - start + lines * width;
  buffer = malloc(buflen);
  if (!buffer) return;

  newline = 1;
  p = buffer;
  for (int i = start; i < end; i++) 
    {
    if (newline) 
      {
      memcpy (p, indentation, width);
      p += width;
      newline = 0;
      }
    char ch = get_char(ed, i);
    *p++ = ch;
    if (ch == '\n') newline = 1;
    }

  replace (ed, start, end - start, buffer, buflen, 1);
  free(buffer);  

  if (ed->anchor < pos) 
    {
    pos += width * lines;
    } 
  else 
    {
    ed->anchor += width * lines;
    }

  ed->toppos += width * toplines;
  ed->linepos = line_start(ed, pos);
  ed->col = ed->lastcol = pos - ed->linepos;

  adjust_layout(ed);
  ed->refresh = TRUE;
  }

/*==========================================================================

  unindent 

==========================================================================*/
void unindent (BUTE *ed, char *indentation) 
  {
  int start, end, i, newline, ch, shrinkage, topofs;
  char *buffer, *p;
  int width = strlen(indentation);
  int pos = ed->linepos + ed->col;

  if (!get_selection (ed, &start, &end)) return;

  buffer = malloc (end - start);
  if (!buffer) return;

  newline = 1;
  p = buffer;
  i = start;
  shrinkage = 0;
  topofs = 0;
  while (i < end)
    {
    if (newline) 
      {
      newline = 0;
      if (compare(ed, indentation, i, width)) 
        {
        i += width;
        shrinkage += width;
        if (i < ed->toppos) topofs -= width;
        continue;
        }
      }
    ch = get_char(ed, i++);
    *p++ = ch;
    if (ch == '\n') newline = 1;
    }

  if (!shrinkage) 
    {
    free(buffer);
    return;
    }

  replace (ed, start, end - start, buffer, p - buffer, 1);
  free(buffer);

  if (ed->anchor < pos) 
    {
    pos -= shrinkage;
    } 
  else 
    {
    ed->anchor -= shrinkage;
    }

  ed->toppos += topofs;
  ed->linepos = line_start(ed, pos);
  ed->col = ed->lastcol = pos - ed->linepos;

  ed->refresh = 1;
  adjust_layout (ed);
  }

void undo(BUTE *ed) {
  if (!ed->undo) return;
  moveto(ed, ed->undo->pos, 0);
  replace(ed, ed->undo->pos, ed->undo->inserted, ed->undo->undobuf, ed->undo->erased, 0);
  ed->undo = ed->undo->prev;
  if (!ed->undo) ed->dirty = 0;
  ed->anchor = -1;
  ed->lastcol = ed->col;
  ed->refresh = 1;
}

void redo (BUTE *ed) 
  {
  if (ed->undo) 
    {
    if (!ed->undo->next) return;
    ed->undo = ed->undo->next;
    } 
  else 
    {
    if (!ed->undohead) return;
    ed->undo = ed->undohead;
    }
  replace (ed, ed->undo->pos, ed->undo->erased, ed->undo->redobuf, 
     ed->undo->inserted, 0);
  moveto(ed, ed->undo->pos, 0);
  ed->dirty = 1;
  ed->anchor = -1;
  ed->lastcol = ed->col;
  ed->refresh = 1;
}

//
// Clipboard
//

void copy_selection (BUTE *ed) 
  {
  int selstart, selend;

  if (!get_selection(ed, &selstart, &selend)) return;
  ed->env->clipsize = selend - selstart;
  ed->env->clipboard = (char *) realloc(ed->env->clipboard, ed->env->clipsize);
  if (!ed->env->clipboard) return;
  copy (ed, ed->env->clipboard, selstart, ed->env->clipsize);
  }

void cut_selection(BUTE *ed) {
  copy_selection(ed);
  erase_selection(ed);
}

void paste_selection(BUTE *ed) {
  erase_selection(ed);
  insert (ed, ed->linepos + ed->col, ed->env->clipboard, ed->env->clipsize);
  moveto (ed, ed->linepos + ed->col + ed->env->clipsize, 0);
  ed->refresh = 1;
}

/*==========================================================================

  prompt_open_editor 

  Prompt for a filename, and open it. If there is already a buffer 
  with the specified name, switch to that buffer

==========================================================================*/
void prompt_open_editor (BUTE *ed) 
  {
  char *filename;
  ButeEnv *env = ed->env;

  if (!prompt (ed, "Open file: ")) 
    {
    ed->refresh = 1;
    return;
    }

  filename = ed->env->linebuf;
  
  ed = find_editor (ed->env, filename);
  if (ed)  
    {
    env->current = ed;
    } 
  else 
    {
    ed = bute_create (env);
    ErrCode err = load_file(ed, filename);
    if (err != 0) 
      {
      display_message (ed, mystrerror (err));
      pause_after_message();
      bute_destroy (ed);
      ed = env->current;
      }
    }
  ed->refresh = TRUE;
  }
  

void new_editor(BUTE *ed) {
  ed = bute_create(ed->env);
  new_file (ed, "");
  ed->refresh = 1;
}

/*==========================================================================

  Save editor

  Prompt for a filename if there is not one, then save

==========================================================================*/
ErrCode save_editor (BUTE *ed) 
  {
  if (!ed->dirty && !ed->newfile) return 0;
  
  if (ed->newfile) 
    {
    if (!prompt (ed, "Save as: ")) 
      {
      ed->refresh = 1;
      return ERR_ABANDONED;
      }

    if (storage_file_exists (ed->env->linebuf)) 
      {
      display_message (ed, "Overwrite %s (y/n)? ", ed->env->linebuf);
      if (!says_yes()) 
        {
        ed->refresh = 1;
        return ERR_ABANDONED;
        }
      }
    strcpy(ed->filename, ed->env->linebuf);
    ed->newfile = 0;
    }

  ErrCode err = save_file(ed);
  if (err != 0) 
    {
    display_message (ed, mystrerror (err));
    pause_after_message(); 
    }

  ed->refresh = TRUE;
  return err;
  }

/*==========================================================================

  Close_editor

==========================================================================*/
void close_editor(BUTE *ed) 
  {
  ButeEnv *env = ed->env;
  
  if (ed->dirty) 
    {
    display_message(ed, 
        "Close %s without saving changes (y/n)? ", ed->filename);
    if (!says_yes()) 
      {
      ed->refresh = 1;
      return;
      }
    }
  
  bute_destroy(ed);

  ed = env->current;
  if (!ed) 
    {
    ed = bute_create(env);
    new_file (ed, "");
    }
  ed->refresh = TRUE;
  }

/*==========================================================================

  quit_editor

  Returns TRUE if it is safe to quit

==========================================================================*/
static BOOL quit_editor (ButeEnv *env)
  {
  BUTE *ed = env->current;
  BUTE *start = ed;

  do 
    {
    if (ed->dirty) 
      {
      display_message (ed, "Close %s without saving changes (y/n)? ", 
        ed->filename);
      if (!says_yes()) 
        {
        draw_full_statusline (ed);
        return FALSE;
        }
      }
    ed = ed->next;
    } while (ed != start);

  return TRUE;
  }


/*==========================================================================

  prompt_find_text 

  if next == TRUE, try to find another instance of the previous
  search text. Otherwise, prompt and try to find first instance of 
  the text entered by the user.

==========================================================================*/
void find_text (BUTE *ed, BOOL next) 
  {
  if (!next) 
    {
    if (!prompt (ed, "Find: ")) 
      {
      ed->refresh = 1;
      return;
      }    
    if (ed->env->search) free (ed->env->search);
    ed->env->search = strdup (ed->env->linebuf);
    }

  if (!ed->env->search) return;

  int slen = strlen(ed->env->search);
  if (slen > 0) 
    {
    char *match;
    
    close_gap(ed);

    match = strstr (ed->start + ed->linepos + ed->col, ed->env->search);

    if (match != NULL) 
      {
      int pos = match - ed->start;
      ed->anchor = pos;
      moveto (ed, pos + slen, 1);
      } 
    else 
      {
      // TODO signal error
      }
    }
  ed->refresh = TRUE;
  }

/*==========================================================================

  prompt_goto_line

  Prompt and go to selected line

==========================================================================*/
void goto_line (BUTE *ed) 
  {
  int lineno, l, pos;

  ed->anchor = -1;
  if (prompt(ed, "Go to line: ")) 
    {
    lineno = atoi (ed->env->linebuf);
    // atoi() works here because "0" is not a valid line number
    if (lineno > 0) 
      {
      pos = 0;
      for (l = 0; l < lineno - 1; l++) 
        {
        pos = next_line(ed, pos);
        if (pos < 0) break;
        } 
      } 
    else 
      {
      pos = -1;
      }

    if (pos >= 0) 
      {
      moveto (ed, pos, 1);
      } 
    else 
      {
      // TODO -- signal error
      }
    }
  ed->refresh = TRUE;
  }

/*==========================================================================

  next_buffer

  Find and return the next editor buffer. This will cause a screen
  refresh.

==========================================================================*/
BUTE *next_buffer (BUTE *ed) 
  {
  ed = ed->env->current = ed->next;
  ed->refresh = TRUE;
  return ed;
  }

/*==========================================================================

  redraw_screen 

  This method can, in principle, respond to changes in the terminal
  size. However, there is no robust way to get the size from a 
  serial terminal, so this facility is left in for potential future
  expansion.

==========================================================================*/
void redraw_screen (BUTE *ed) 
  {
  get_console_size (ed->env);
  ed->env->linebuf = realloc 
    (ed->env->linebuf, ed->env->cols + LINEBUF_EXTRA);
  draw_screen(ed);
  }

/*==========================================================================

  intr 

==========================================================================*/
static void intr (BUTE *ed) 
  {
  display_message (ed, "Ctrl+Q to exit");
  pause_after_message();
  draw_full_statusline (ed);
  }

/*==========================================================================

  run 

==========================================================================*/
static void run (BUTE *ed) 
  {
  ErrCode err = save_editor (ed);
  if (err == 0)
    {
    term_clear_and_home();
    shell_runlua (ed->filename);
    interface_write_string ("Press any key...");
    term_get_key();
    ed->refresh = TRUE;
    draw_full_statusline (ed);
    }
  else
    {
    display_message (ed, shell_strerror (err));
    pause_after_message();
    draw_full_statusline (ed);
    } 
  }

/*==========================================================================

  help

==========================================================================*/
static void help (BUTE *ed) 
  {
  term_set_cursor (0, 0);
  term_clear ();
  interface_write_stringln 
  ("Arrow and page keys move cursor. Shift+movement keys select text"); 
  interface_write_stringln 
  ("<back>             Delete back     <del>              Delete forward"); 
  interface_write_stringln 
  ("Ctrl+<left>        Prev. word      Ctrl+<right>       Next word"); 
  interface_write_stringln 
  ("Ctrl+A             Select all      Ctrl-B, Ctrl-end   Bottom of file"); 
  interface_write_stringln 
  ("Ctrl+E             Switch buffers  Ctrl+F             Find text"); 
  interface_write_stringln 
  ("Ctrl+G             Find next       Ctrl+L             Redraw screen"); 
  interface_write_stringln 
  ("Ctrl+L             Go to line      Ctrl+N             New buffer" ); 
  interface_write_stringln 
  ("Ctrl+O             Open file       Ctrl+Q             Quit" ); 
  interface_write_stringln 
  ("Ctrl+R             Redo            Ctrl+S             Save" ); 
  interface_write_stringln 
  ("Ctrl+T, Ctrl+Home  Top of file     Ctrl+V             Paste"); 
  interface_write_stringln 
  ("Ctrl+W             Close buffer    Ctrl+X             Cut"); 
  interface_write_stringln 
  ("Ctrl+Y             Copy            Ctrl+Z             Undo"); 
  interface_write_stringln 
  ("Ctrl+\\             Run Lua"); 
  interface_write_endl();
  interface_write_stringln 
  ("In a selection, <tab> indents and Shift-<tab> unindents");
  interface_write_stringln 
  ("Press any key to continue...");

  term_get_key();
  draw_screen (ed);
  draw_full_statusline (ed);
  }


/*==========================================================================

  edit

  This is the main keyboard loop for the editor 

==========================================================================*/
static void edit (BUTE *ed) 
  {
  BOOL done = FALSE;

  ed->refresh = TRUE; // Start with a full refresh

  while (!done)  // Continue until told to stop
    {
    // Carry out the redraw actions from the last loop
    if (ed->refresh) 
      {
      draw_screen(ed);
      draw_full_statusline(ed);
      ed->refresh = FALSE;
      ed->lineupdate = FALSE;
      } 
   else if (ed->lineupdate) 
      {
      update_line(ed);
      ed->lineupdate = FALSE;
      draw_statusline(ed);
      } 
   else 
      {
      draw_statusline(ed);
      }

    // Line up the screen cursor with the editor state
    position_cursor (ed);

    int key = term_get_key();

    if (key >= ' ' && key < 0x7F) // Don't try to insert DEL
      {
      insert_char (ed, (unsigned char) key);
      } 
    else 
      {
      switch (key) 
        {
        case 9999: redraw_screen (ed); break; // Not implemented yet

        case VK_UP: up(ed, 0); break;
        case VK_DOWN: down(ed, 0); break;
        case VK_LEFT: left(ed, 0); break;
        case VK_RIGHT: right(ed, 0); break;
        case VK_HOME: home(ed, 0); break;
        case VK_END: end(ed, 0); break;
        case VK_PGUP: pageup(ed, 0); break;
        case VK_PGDN: pagedown(ed, 0); break;

        case VK_CTRLRIGHT: wordright (ed, 0); break;
        case VK_CTRLLEFT: wordleft (ed, 0); break;
        case VK_CTRLHOME: top (ed, 0); break;
        case VK_CTRLEND: bottom (ed, 0); break;

        case VK_TAB: indent (ed, INDENT); break;

        case VK_SHIFTUP: up (ed, 1); break;
        case VK_SHIFTDOWN: down(ed, 1); break;
        case VK_SHIFTLEFT: left(ed, 1); break;
        case VK_SHIFTRIGHT: right(ed, 1); break;
     
        case VK_CTRLSHIFTRIGHT: wordright (ed, 1); break;
        case VK_CTRLSHIFTLEFT: wordleft(ed, 1); break;
        case VK_CTRLSHIFTHOME: top(ed, 1); break;
        case VK_CTRLSHIFTEND: bottom(ed, 1); break;

        case VK_SHIFTTAB: unindent (ed, INDENT); break;
        case VK_ENTER: newline (ed); break;
        case VK_BACK: backspace (ed); break;
        case VK_DEL: del (ed); break;
        case VK_INTR: intr (ed); break;

        case '@' - 64: help (ed); break;
        case '\\' - 64: run (ed); break;
        case CTRL('a'): select_all (ed); break;
        case CTRL('b'): bottom (ed, 0); break;
        case CTRL('e'): ed = next_buffer (ed); break;
        case CTRL('f'): find_text (ed, 0); break;
        case CTRL('g'): find_text (ed, 1); break;
        case CTRL('k'): redraw_screen (ed); break; 
        case CTRL('l'): goto_line(ed); break;
        case CTRL('n'): new_editor(ed); ed = ed->env->current; break;
        case CTRL('o'): prompt_open_editor(ed); ed = ed->env->current; break;
        case CTRL('q'): done = quit_editor (ed->env); break;
        case CTRL('r'): redo (ed); break;
        case CTRL('s'): save_editor (ed); break;
        case CTRL('t'): top (ed, 0); break;
        case CTRL('v'): paste_selection(ed); break;
        case CTRL('w'): close_editor (ed); ed = ed->env->current; break;
        case CTRL('x'): cut_selection (ed); break;
        case CTRL('y'): copy_selection (ed); break;
        case CTRL('z'): undo (ed); break;
        }
      }
    }
  }

/*==========================================================================

  env_create

==========================================================================*/
ButeEnv *buteenv_create (void) 
  {
  ButeEnv *self = malloc (sizeof (ButeEnv));
  if (self)
    {
    memset (self, 0, sizeof (ButeEnv));
    get_console_size (self);
    self->linebuf = realloc (self->linebuf, self->cols + LINEBUF_EXTRA);
    }
  return self;
  }

/*==========================================================================

  buteenv_create_editor

==========================================================================*/
ErrCode buteenv_add_editor (ButeEnv *self, const char *filename) 
  {
  ErrCode ret = 0;

  BUTE *ed = bute_create (self);

  if (filename)
    {
    if (storage_file_exists (filename))
      ret = load_file (ed, filename);
    else
      {
      new_file (ed, filename);
      }
    }
  else
    new_file (ed, "");

  return ret;
  }

/*==========================================================================

  buteenv_run 

==========================================================================*/
void buteenv_run (ButeEnv *self)
  {
  term_set_cursor (0, 0);
  term_clear();
  self->current = self->current->next;
  edit (self->current);
  term_clear_and_home();
  }

/*==========================================================================

  buteenv_destroy

==========================================================================*/
void buteenv_destroy (ButeEnv *self) 
  {
  while (self->current) bute_destroy (self->current);
  if (self->clipboard) free (self->clipboard);
  if (self->search) free (self->search);
  if (self->linebuf) free (self->linebuf);
  free (self);
  }


/*==========================================================================

  bute_run

==========================================================================*/
void bute_run (const char *filename) 
  {
  ButeEnv *env = buteenv_create ();

  buteenv_add_editor (env, filename);
  
  buteenv_run (env);

  buteenv_destroy (env);
  }



