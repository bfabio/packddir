/* $Id: lists.h,v 1.2 2003/07/14 17:32:02 fabiob Exp $ */
#ifndef _LIST_H
#define _LIST_H

struct list_node {
	struct list_node *prev;
	struct list_node *next;

	void *element;
};

typedef struct list_node* list_node_t;

struct list {
	list_node_t first;
	list_node_t last;
};

typedef struct list * list_t;

#define foreach(x, l) for (list_node_t n = (l)->first; ((x) = n); n = n->next)

list_t list_new();
void list_append(list_t list, void *element);
void list_delete(list_t list, list_node_t node);

#endif /* !_LIST_H */
