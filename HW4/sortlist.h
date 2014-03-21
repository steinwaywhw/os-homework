

#ifndef _SORT_LIST_H
#define _SORT_LIST_H

#include "ring_buffer.h"

typedef struct _sortlist_t {
	struct _sortlist_t *next;
	buffer_entry_t *entry;
} sortlist_t;

sortlist_t* sortlist_insert (sortlist_t *head, buffer_entry_t *entry);
sortlist_t* sortlist_remove (sortlist_t *head, buffer_entry_t **entry);
void sortlist_destroy (sortlist_t *head);

#endif