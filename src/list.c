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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "config.h"
#include "list.h"
#include "version.h"

/**
 */
static struct node *_new(void *data, size_t size)
{
	struct node *t;

	t = malloc(size + sizeof(struct node));
	if (t != NULL) {
		t->list = NULL;
		t->prev = NULL;
		t->next = NULL;
		t->item = t + 1;
		memcpy(t->item, data, size);
	}

	return t;
}

/**
 */
static void _delete(struct node *node)
{
	struct node *next;

	while (node) {
		next = node->next;
		free(node);
		node = next;
	}
}

/**
 */
static void _fini(struct list *list)
{
	_delete(list->head);
	free(list);
}

/**
 */
static void _list_delete(struct list *list)
{
	_delete(list->head);
	list->head = list->tail = NULL;
}

/**
 */
static status_t _remove(struct node *node)
{
	struct list *list;

	list = node->list;
	if (list == NULL)
		return STATUS_INVALID_NODE;
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

	return STATUS_SUCCESS;
}

/**
 */
static struct node *_next(struct node *ptr)
{
	return ptr->next;
}

/**
 */
static struct node *_prev(struct node *ptr)
{
	return ptr->prev;
}

/**
 */
static void _put_front(struct list *list, struct node *elem)
{
	if (list->head == NULL)
		list->tail = elem;
	else {
		elem->next = list->head;
		list->head->prev = elem;
	}
	list->head = elem;
	elem->list = list;
}

/**
 */
static void _put_back(struct list *list, struct node *elem)
{
	if (list->tail == NULL)
		list->head = elem;
	else {
		elem->prev = list->tail;
		list->tail->next = elem;
	}
	list->tail = elem;
	elem->list = list;
}

/**
 */
static struct node *_tail(struct list *ptr)
{
	return ptr->tail;
}

/**
 */
static struct node *_head(struct list *ptr)
{
	return ptr->head;
}

/**
 */
status_t list_init(struct list **ptr)
{
	struct list *t;

	t = malloc(sizeof(struct list));
	if (t == NULL)
		return STATUS_OUT_OF_MEMORY;
	t->head = NULL;
	t->tail = NULL;

	*ptr = t;
	return STATUS_SUCCESS;
}

/**
 */
status_t list_fini(struct list *ptr)
{
	if (ptr != NULL)
		_fini(ptr);
	return STATUS_SUCCESS;
}

/**
 */
status_t list_remove(struct node *ptr)
{
	if (ptr == NULL)
		return STATUS_NULL_POINTER;
	return _remove(ptr);
}

/**
 */
void *list_add(struct list *ptr, void *data, size_t size)
{
	if (ptr && data && (size > 0)) {
		struct node *result = _new(data, size);

		if (result != NULL) {
			_put_front(ptr, result);
			return result->item;
		}
	}
	return NULL;
}

/**
 */
void *list_put(struct list *ptr, void *data, size_t size)
{
	if (ptr && data && (size > 0)) {
		struct node *result = _new(data, size);

		if (result != NULL) {
			_put_back(ptr, result);
			return result->item;
		}
	}
	return NULL;
}

/**
 */
struct node *list_next(struct node *ptr)
{
	if (ptr) {
		struct node *node = _next(ptr);
		if (node)
			return node;
	}
	return NULL;
}

/**
 */
struct node *list_prev(struct node *ptr)
{
	if (ptr) {
		struct node *node = _prev(ptr);
		if (node)
			return node;
	}
	return NULL;
}

/**
 */
struct node *list_head(struct list *ptr)
{
	if (ptr) {
		struct node *node = _head(ptr);
		if (node)
			return node;
	}
	return NULL;
}

/**
 */
struct node *list_tail(struct list *ptr)
{
	if (ptr) {
		struct node *node = _tail(ptr);
		if (node)
			return node;
	}
	return NULL;
}

/**
 */
status_t list_delete(struct list *ptr)
{
	if (ptr == NULL)
		return STATUS_NULL_POINTER;
	_list_delete(ptr);
	return STATUS_SUCCESS;
}

/**
 */
int list_is_empty(struct list *ptr)
{
	if (ptr == NULL)
		return STATUS_NULL_POINTER;
	return (ptr->head == NULL);
}

/**
 */
status_t __list_for_each(struct list *ptr, action_t action, void *parm)
{
	assert(action != NULL);

	struct node *next_node, *node = list_head(ptr);
	while (node != NULL) {
		next_node = list_next(node);
		action(node->item, parm);
		node = next_node;
	}
	return STATUS_SUCCESS;
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
