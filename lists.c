#include <stdlib.h>

#include "lists.h"

list_t list_new()
{
	list_t l;

	l = malloc(sizeof(list_t));
	l->first = l->last = NULL;

	return l;
}

void list_delete(list_t list, list_node_t node)
{
	if (node->element)
		free(node->element);
	
	if (list->first == list->last) {
		list->first = list->last = NULL;

	} else if (node == list->first) {
		list->first = node->next;

	} else if (node == list->last) {
		list->last = node->prev;

	} else {
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}

	free(node);
}

void list_append(list_t list, void *element)
{
	list_node_t node;

	node = malloc(sizeof(struct list_node));
	node->element = element;
	node->next = NULL;

	if (list->first) {
		list->last->next = node;
		node->prev = list->last;

	} else {
		list->first = node;
		node->prev = NULL;

	}

	list->last = node;
}
