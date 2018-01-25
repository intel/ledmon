/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009-2017 Intel Corporation.
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

#include <stdlib.h>
#include <assert.h>

#include "list.h"
#include "utils.h"

struct list *list_alloc(void)
{
	struct list *t;

	t = malloc(sizeof(struct list));
	if (t == NULL)
		return NULL;
	list_init(t);

	return t;
}

void list_fini(struct list *list)
{
	struct node *node;
	struct node *next;

	node = list->head;
	while (node) {
		next = node->next;
		free(node->item);
		free(node);
		node = next;
	}
	free(list);
}

void list_remove(struct node *node)
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

void list_clear(struct list *list)
{
	struct node *node;
	struct node *next;

	node = list->head;
	while (node) {
		next = node->next;
		free(node);
		node = next;
	}
	list->head = list->tail = NULL;
}

void __list_for_each(struct list *list, action_t action, void *parm)
{
	assert(action != NULL);

	struct node *next_node, *node = list_head(list);
	while (node != NULL) {
		next_node = list_next(node);
		action(node->item, parm);
		node = next_node;
	}
}

void *list_first_that(struct list *list, test_t test, const void *parm)
{
	assert(test != NULL);

	struct node *node = list_head(list);
	while (node) {
		if (test(node->item, parm))
			return node->item;
		node = list_next(node);
	}
	return NULL;
}

void *list_last_that(struct list *list, test_t test, const void *parm)
{
	assert(test != NULL);

	struct node *node = list_tail(list);
	while (node) {
		if (test(node->item, parm))
			return node->item;
		node = list_prev(node);
	}
	return NULL;
}
