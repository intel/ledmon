/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2018 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <assert.h>

#include "list.h"
#include "utils.h"

void __list_erase(struct list *list, item_free_t free_fn)
{
	struct node *node;

	list_for_each_node(list, node) {
		if (free_fn)
			free_fn(node->item);
		free(node);
	}
	list->head = list->tail = NULL;
}

void __list_remove(struct node *node, item_free_t free_fn)
{
	struct list *list = node->list;

	if (node->prev)
		node->prev->next = node->next;
	else
		list->head = node->next;
	if (node->next)
		node->next->prev = node->prev;
	else
		list->tail = node->prev;
	node->list = NULL;
	node->next = NULL;
	node->prev = NULL;

	if (free_fn)
		free_fn(node->item);
}

void list_insert(struct list *list, void *item, struct node *after)
{
	struct node *new;
	struct node **x;

	new = malloc(sizeof(struct node));
	if (!new) {
		log_error("Failed to allocate memory for list node.");
		exit(1);
	}

	new->list = list;
	new->item = item;

	if (after) {
		assert(list == after->list);
		x = &after->next;
	} else {
		x = &list->head;
	}

	if (*x == NULL)
		list->tail = new;
	else
		(*x)->prev = new;
	new->next = *x;
	*x = new;
	new->prev = after;
}
