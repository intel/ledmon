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

/**
 */
static void _put_front(struct list *list, struct node *elem)
{
	if (list->head == NULL) {
		elem->next = NULL;
		list->tail = elem;
	} else {
		elem->next = list->head;
		list->head->prev = elem;
	}
	list->head = elem;
	elem->list = list;
	elem->prev = NULL;
}

/**
 */
static void _put_back(struct list *list, struct node *elem)
{
	if (list->tail == NULL) {
		elem->prev = NULL;
		list->head = elem;
	} else {
		elem->prev = list->tail;
		list->tail->next = elem;
	}
	list->tail = elem;
	elem->list = list;
	elem->next = NULL;
}

/**
 */
struct list *list_alloc(void)
{
	struct list *t;

	t = malloc(sizeof(struct list));
	if (t == NULL)
		return NULL;
	list_init(t);

	return t;
}

/**
 */
void list_fini(struct list *ptr)
{
	struct node *node;
	struct node *next;

	node = ptr->head;
	while (node) {
		next = node->next;
		free(node->item);
		free(node);
		node = next;
	}
	free(ptr);
}

/**
 */
void list_remove(struct node *ptr)
{
	struct list *list = ptr->list;

	if (ptr->prev)
		ptr->prev->next = ptr->next;
	else
		list->head = ptr->next;
	if (ptr->next)
		ptr->next->prev = ptr->prev;
	else
		list->tail = ptr->prev;
	ptr->list = NULL;
	ptr->next = NULL;
	ptr->prev = NULL;
}

struct node *node_alloc(void)
{
	struct node *ret = malloc(sizeof(struct node));

	if (!ret) {
		log_error("Failed to allocate memory for list node.");
		exit(1);
	}

	return ret;
}

/**
 */
void list_add(struct list *ptr, void *data)
{
	struct node *result = node_alloc();

	_put_front(ptr, result);
	result->item = data;
}

/**
 */
void list_put(struct list *ptr, void *data)
{
	struct node *result = node_alloc();

	_put_back(ptr, result);
	result->item = data;
}

/**
 */
struct node *list_next(struct node *ptr)
{
	return ptr->next;
}

/**
 */
struct node *list_prev(struct node *ptr)
{
	return ptr->prev;
}

/**
 */
struct node *list_head(struct list *ptr)
{
	return ptr->head;
}

/**
 */
struct node *list_tail(struct list *ptr)
{
	return ptr->tail;
}

/**
 */
void list_clear(struct list *ptr)
{
	struct node *node;
	struct node *next;

	node = ptr->head;
	while (node) {
		next = node->next;
		free(node);
		node = next;
	}
	ptr->head = ptr->tail = NULL;
}

/**
 */
int list_is_empty(struct list *ptr)
{
	return (ptr->head == NULL);
}

/**
 */
void __list_for_each(struct list *ptr, action_t action, void *parm)
{
	assert(action != NULL);

	struct node *next_node, *node = list_head(ptr);
	while (node != NULL) {
		next_node = list_next(node);
		action(node->item, parm);
		node = next_node;
	}
}

/**
 */
void *list_first_that(struct list *ptr, test_t test, const void *parm)
{
	assert(test != NULL);

	struct node *node = list_head(ptr);
	while (node) {
		if (test(node->item, parm))
			return node->item;
		node = list_next(node);
	}
	return NULL;
}

/**
 */
void *list_last_that(struct list *ptr, test_t test, const void *parm)
{
	assert(test != NULL);

	struct node *node = list_tail(ptr);
	while (node) {
		if (test(node->item, parm))
			return node->item;
		node = list_prev(node);
	}
	return NULL;
}
