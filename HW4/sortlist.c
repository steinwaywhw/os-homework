#include "sortlist.h"
#include <assert.h>
#include <malloc.h>


sortlist_t* sortlist_insert (sortlist_t *head, buffer_entry_t *entry) {
	assert (entry != NULL);

	if (head == NULL) {
		head = (sortlist_t *)malloc (sizeof (sortlist_t));
		assert (head != NULL);

		head->next = NULL;
		head->entry = entry;
		return head;
	}

	sortlist_t *current = head;
	sortlist_t *previous = NULL;

	while (current != NULL && entry->response.request.priority <= current->entry->response.request.priority) {
		previous = current;
		current = current->next;
	}

	if (previous == NULL) {
		previous = (sortlist_t *)malloc (sizeof (sortlist_t));
		assert (previous != NULL);

		previous->next = current;
		previous->entry = entry;
		return previous;
	}

	if (current == NULL) {
		current = (sortlist_t *)malloc (sizeof (sortlist_t));
		assert (current != NULL);

		previous->next = current;
		current->next = NULL;
		current->entry = entry;
		return head;
	}

	sortlist_t *p = (sortlist_t *)malloc (sizeof (sortlist_t));
	assert (p != NULL);

	previous->next = p;
	p->next = current;
	p->entry = entry;

	return head;
}


sortlist_t* sortlist_remove (sortlist_t *head, buffer_entry_t **entry) {
	assert (head != NULL);
	assert (entry != NULL);

	*entry = head->entry;
	sortlist_t *p = head->next;
	free (head);
	return p;
}

void sortlist_destroy (sortlist_t *head) {
	buffer_entry_t *entry;
	while (head != NULL) {
		head = sortlist_remove (head, &entry);
	}
}
