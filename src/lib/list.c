// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 Intel Corporation.

/* System headers */
#include <assert.h>
#include <stdbool.h>

/* Local headers */
#include "list.h"
#include "libled_private.h"
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

bool list_insert(struct list *list, void *item, struct node *after)
{
	struct node *new;
	struct node **x;

	assert(item != NULL);
	new = malloc(sizeof(struct node));
	if (!new) {
		return false;
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
	return true;
}


void list_append_ctx(struct list *list, void *item, struct led_ctx *ctx)
{
	if (!list_insert(list, item, list->tail))
		ctx->deferred_error = LED_STATUS_OUT_OF_MEMORY;
}

bool list_insert_compar(struct list *list, void *item, bool compar_fn(void *item1, void *item2))
{
	struct node *tmp = NULL;

	list_for_each_node(list, tmp) {
		if (compar_fn(item, tmp->item) == true)
			return list_insert(list, item, list_prev(tmp));
	}
	return list_append(list, item);
}
