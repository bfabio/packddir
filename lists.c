/*  upackddir - extracts files from PackdDir archives

 *  Copyright (C) 2003-2004 Fabio Bonelli <fabiobonelli@libero.it>

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  See the GNU General Public License for more details
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/* $Id: lists.c,v 1.6 2005/01/13 23:53:43 fabiob Exp $ */

/* lists.c - Lists handling */

#include <stdlib.h>

#include "log.h"
#include "lists.h"

list_t list_new()
{
	list_t l;

	l = malloc(sizeof(list_t));
	if (!l) {
		LOG("malloc() failed.\n");
		exit(EXIT_FAILURE);
	}

	l->first = l->last = NULL;

	return l;
}

void list_delete(list_t list, list_node_t node)
{
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

int list_empty(list_t list)
{
	return list->first ? 1 : 0;
}
