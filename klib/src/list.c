/*============================================================================

  klib
  list.c
  Copyright (c)2000-2021 Kevin Boone, GPL v3.0

  Methods for maintaining a single-linked list.

============================================================================*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include "../include/klib/list.h"
#include "../include/klib/string.h" 

#define LOG_IN
#define LOG_OUT

typedef struct _ListItem
  {
  struct _ListItem *next;
  void *data;
  } ListItem;

struct _List
  {
  ListItemFreeFn free_fn; 
  ListItem *head;
  };

/*==========================================================================
list_create
*==========================================================================*/
List *list_create (ListItemFreeFn free_fn)
  {
  LOG_IN
  List *list = malloc (sizeof (List));
  memset (list, 0, sizeof (List));
  list->free_fn = free_fn;
  LOG_OUT
  return list;
  }

/*==========================================================================
  list_create_strings
 
  This is a helper function for creating a list of C strings -- not
  string objects
*==========================================================================*/
List *list_create_strings (void)
  {
  return list_create (free);
  }

/*==========================================================================
  list_destroy
*==========================================================================*/
void list_destroy (List *self)
  {
  LOG_IN
  if (self) 
    {
    ListItem *l = self->head;
    while (l)
      {
      if (self->free_fn)
        self->free_fn (l->data);
      ListItem *temp = l;
      l = l->next;
      free (temp);
      }

    free (self);
    }
  LOG_OUT
  }


/*==========================================================================
list_prepend
Note that the caller must not modify or free the item added to the list. It
will remain on the list until free'd by the list itself, by calling
the supplied free function
*==========================================================================*/
void list_prepend (List *self, void *item)
  {
  LOG_IN
  ListItem *i = malloc (sizeof (ListItem));
  i->data = item;
  i->next = NULL;

  if (self->head)
    {
    i->next = self->head;
    self->head = i;
    }
  else
    {
    self->head = i;
    }
  LOG_OUT
  }


/*==========================================================================
  list_append
  Note that the caller must not modify or free the item added to the list. 
  It will remain on the list until free'd by the list itself, by calling
    the supplied free function
*==========================================================================*/
void list_append (List *self, void *item)
  {
  LOG_IN
  ListItem *i = malloc (sizeof (ListItem));
  i->data = item;
  i->next = NULL;

  if (self->head)
    {
    ListItem *l = self->head;
    while (l->next)
      l = l->next;
    l->next = i;
    }
  else
    {
    self->head = i;
    }
  LOG_OUT
  }


/*==========================================================================
  list_length
*==========================================================================*/
int list_length (List *self)
  {
  LOG_IN
  ListItem *l = self->head;

  int i = 0;
  while (l != NULL)
    {
    l = l->next;
    i++;
    }

  LOG_OUT
  return i;
  }

/*==========================================================================
  list_get
*==========================================================================*/
void *list_get (List *self, int index)
  {
  LOG_IN
  ListItem *l = self->head;
  int i = 0;
  while (l != NULL && i != index)
    {
    l = l->next;
    i++;
    }
  LOG_OUT
  return l->data;
  }


/*==========================================================================
  list_dump
  For debugging purposes -- will only work at all if the list contains
  C strings
*==========================================================================*/
void list_dump (List *self)
  {
  int i, l = list_length (self);
  for (i = 0; i < l; i++)
    {
    const char *s = list_get (self, i);
    printf ("%s\n", s);
    }
  }


/*==========================================================================
  list_contains
*==========================================================================*/
BOOL list_contains (List *self, const void *item, ListCompareFn fn)
  {
  LOG_IN
  ListItem *l = self->head;
  BOOL found = FALSE;
  while (l != NULL && !found)
    {
    if (fn (l->data, item, NULL) == 0) found = TRUE; 
    l = l->next;
    }
  LOG_OUT
  return found; 
  }


/*==========================================================================
list_contains_string
*==========================================================================*/
BOOL list_contains_string (List *self, const char *item)
  {
  return list_contains (self, item, (ListCompareFn)strcmp);
  }


/*==========================================================================
list_remove_object
Remove the specific item from the list, if it is present. The object's
remove function will be called. This method can't be used to remove an
object by value -- that is, you can't pass "dog" to the method to remove
all strings whose value is "dog". Use list_remove() for that.
*==========================================================================*/
void list_remove_object (List *self, const void *item)
  {
  LOG_IN
  ListItem *l = self->head;
  ListItem *last_good = NULL;
  while (l != NULL)
    {
    if (l->data == item)
      {
      if (l == self->head)
        {
        self->head = l->next; // l-> next might be null
        }
      else
        {
        if (last_good) last_good->next = l->next;
        }
      self->free_fn (l->data);  
      ListItem *temp = l->next;
      free (l);
      l = temp;
      } 
    else
      {
      last_good = l;
      l = l->next;
      }
    }
  LOG_OUT
  }


/*==========================================================================
list_remove
Remove all items from the last that are a match for 'item', as
determined by a comparison function.

IMPORTANT -- The "item" argument cannot be a direct reference to an
item already in the list. If that item is removed from the list its
memory will be freed. The "item" argument will thus be an invalid
memory reference, and the program will crash when it is next used. 
It is necessary to provide a comparison function, and items will be
removed (and freed) that satisfy the comparison function. 

To remove one specific, known, item from the list, uselist_remove_object()
*==========================================================================*/
void list_remove (List *self, const void *item, ListCompareFn fn)
  {
  LOG_IN
  ListItem *l = self->head;
  ListItem *last_good = NULL;
  while (l != NULL)
    {
    if (fn (l->data, item, NULL) == 0)
      {
      if (l == self->head)
        {
        self->head = l->next; // l-> next might be null
        }
      else
        {
        if (last_good) last_good->next = l->next;
        }
      self->free_fn (l->data);  
      ListItem *temp = l->next;
      free (l);
      l = temp;
      } 
    else
      {
      last_good = l;
      l = l->next;
      }
    }
  LOG_OUT
  }

/*==========================================================================
list_remove_string
*==========================================================================*/
void list_remove_string (List *self, const char *item)
  {
  list_remove (self, item, (ListCompareFn)strcmp);
  }


/*==========================================================================
list_clone
*==========================================================================*/
List *list_clone (List *self, ListCopyFn copyFn)
  {
  LOG_IN
  ListItemFreeFn free_fn = self->free_fn; 
  List *new = list_create (free_fn);

  ListItem *l = self->head;
  while (l != NULL)
    {
    void *data = copyFn (l->data);
    list_append (new, data);
    l = l->next;
    }

  LOG_OUT
  return new;
  }


/*==========================================================================
  list_sort

  Sort the list according to the supplied list sort function. This should
  return -1, 0, or 1 in the usual way. The arguments to this function are
  pointers to pointers to objects supplied by list_append, etc., not direct
  pointers.

  The implementation is rather ugly -- we copy the list data pointers to a
  flat array, then use qsort() on the array. Then we copy the list data
  pointers back into the list. So the original chain of links remains
  unchanged, but each link is now associated with a new data item.
*==========================================================================*/
void list_sort (List *self, ListSortFn fn, void *user_data)
  {
  LOG_IN

  int length = list_length (self);
  
  void **temp = malloc (length * sizeof (void *));
  ListItem *l = self->head;
  int i = 0;
  while (l != NULL)
    {
    temp[i] = l->data;
    l = l->next;
    i++;
    }

  // Sort temp

  qsort_r (temp, length, sizeof (void *), fn, user_data); 
  
  // Copy the sorted data back
   
  l = self->head;
  i = 0;
  while (l != NULL)
    {
    l->data = temp[i];
    l = l->next;
    i++;
    }
  
  free (temp);

  LOG_OUT
  }

