/*============================================================================

  boilerplate 
  list.h
  Copyright (c)2000-2020 Kevin Boone, GPL v3.0

============================================================================*/

#pragma once

#include "defs.h" 

struct _List;
typedef struct _List List;

// The comparison function should return -1, 0, +1, like strcmp. In practice
//   however, the functions that use this only care whether too things 
//   are equal -- ordering is not important. The i1,i2 arguments are 
//   pointers to the actual objects in the list. user_data is not used
//   at present
typedef int (*ListCompareFn) (const void *i1, const void *i2, 
          void *user_data);

// A comparison function for list_sort. Rather confusingly, it looks exactly
//   like ListCompareFn, but the argument type is different. Here the i1,i2
//   are the addresses of pointers to objects in the list, not pointers.
//   So they are pointers to pointers. Sorry, but this is the way the
//   underlying qsort implementation works. For an example of coding a
//   sort function, see string_alpha_sort_fn. The user_data argument is
//   the value passed to the list_sort function itself, and is relevant
//   only to the caller
typedef int (*ListSortFn) (const void *i1, const void *i2, 
          void *user_data);

typedef void* (*ListCopyFn) (const void *orig);
typedef void (*ListItemFreeFn) (void *);

List   *list_create (ListItemFreeFn free_fn);
void    list_destroy (List *);
void    list_append (List *self, void *item);
void    list_prepend (List *self, void *item);
void   *list_get (List *self, int index);
void    list_dump (List *self);
int     list_length (List *self);
BOOL    list_contains (List *self, const void *item, ListCompareFn fn);
BOOL    list_contains_string (List *self, const char *item);
void    list_remove (List *self, const void *item, ListCompareFn fn);
void    list_remove_string (List *self, const char *item);
List   *list_clone (List *self, ListCopyFn copyFn);
List   *list_create_strings (void);
void    list_remove_object (List *self, const void *item);
void    list_sort (List *self, ListSortFn fn, void *user_data);

