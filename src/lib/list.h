/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2022-2024 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef _LIST_H_INCLUDED_
#define _LIST_H_INCLUDED_

#include <stdlib.h>
#include <stdbool.h>

struct led_ctx;

struct node {
	struct node *next, *prev;
	struct list *list;
	void *item;
};

typedef void (*item_free_t)(void *);

struct list {
	struct node *head, *tail;
	item_free_t item_free;
};

#define __list_for_each_node(__list, __node, __start_fn, __iter_fn) \
	for (struct node *__n = __start_fn(__list), *__next; \
	     __n && (__node = __n) && ((__next = __iter_fn(__n)) || (!__next)); \
	     __n = __next)

#define list_for_each_node(__list, __node) \
	__list_for_each_node(__list, __node, list_head, list_next)

#define list_for_each_node_reverse(__list, __node) \
	__list_for_each_node(__list, __node, list_tail, list_prev)

#define __list_for_each(__list, __item, __start_fn, __iter_fn) \
	for (struct node *__node = __start_fn(__list); \
	     __node && ((__item = __node->item) || (!__item)); \
	     __node = __iter_fn(__node))

#define list_for_each(__list, __item) \
	__list_for_each(__list, __item, list_head, list_next)

#define list_for_each_reverse(__list, __item) \
	__list_for_each(__list, __item, list_tail, list_prev)

/**
 * @brief Initializes a list object.
 *
 * Initializes a list object to reflect an empty state.
 *
 * @param[in]      list           pointer to a list object.
 * @param[in]      item_free_fn   custom callback for deallocating list items.
 *                                If NULL, free() will be used.
 */
static inline void list_init(struct list *list, item_free_t item_free_fn)
{
	list->head = NULL;
	list->tail = NULL;
	if (item_free_fn)
		list->item_free = item_free_fn;
	else
		list->item_free = free;
}

/**
 * @brief Clears a list and frees the items it contains.
 *
 * This function releases the memory allocated for a list object. It also frees
 * the data items attached to list nodes. It does not free the list itself.
 *
 * @param[in]      list           pointer to a list object.
 */
static inline void list_erase(struct list *list)
{
	void __list_erase(struct list *list, item_free_t free_fn);
	__list_erase(list, list->item_free);
}

/**
 * @brief Clears a list.
 *
 * This function removes and deallocates all nodes from the list. It does not
 * free the data items, to do that use list_erase().
 *
 * @param[in]      list           pointer to a list object.
 */
static inline void list_clear(struct list *list)
{
	void __list_erase(struct list *list, item_free_t free_fn);
	__list_erase(list, NULL);
}

/**
 * @brief Removes an element from the list.
 *
 * This function removes an element from the list. It only detaches the element
 * and does not release the memory allocated for the element. To free memory
 * allocated for an element use list_delete() instead.
 *
 * @param[in]      node           pointer to a node object.
 */
static inline void list_remove(struct node *node)
{
	void __list_remove(struct node *node, item_free_t free_fn);
	__list_remove(node, NULL);
}

/**
 * @brief Removes an element from the list and releases its memory.
 *
 * This function removes an element from the list and frees the memory allocated
 * for the list node and data item.
 *
 * @param[in]      node           pointer to a node object.
 */
static inline void list_delete(struct node *node)
{
	void __list_remove(struct node *node, item_free_t free_fn);
	__list_remove(node, node->list->item_free);
	free(node);
}

/**
 * @brief Inserts an element into the list.
 *
 * This function puts an element after a given element.
 *
 * @param[in]      list           pointer to list object.
 * @param[in]      item           data item to be inserted into the list.
 * @param[in]      after          list node after which to insert the element.
 *                                If NULL, then insert at the head of the list.
 *
 * @return true if inserted, else false
 */
bool list_insert(struct list *list, void *item, struct node *after);

/**
 * @brief Appends an element to the end of the list.
 *
 * @param[in]      list           pointer to list object.
 * @param[in]      item           data item to be inserted into the list.
 *
 * @return true on success, else false on memory allocation failure.
 */
static inline bool list_append(struct list *list, void *item)
{
	return list_insert(list, item, list->tail);
}

/**
 * @brief Appends an element to the end of the list.
 *
 * @param[in]      list           pointer to list object.
 * @param[in]      item           data item to be inserted into the list.
 * @param[in]      ctx            library context (sets ctx deferred error).
 */
void list_append_ctx(struct list *list, void *item, struct led_ctx *ctx);

/**
 * @brief Reruns next element.
 *
 * This function returns next element relatively to the given element.
 *
 * @param[in]      node           pointer to a node object.
 *
 * @return Pointer to an element if successful. The NULL pointer means
 *         that node is the last element on the list.
 */
static inline struct node *list_next(const struct node *node)
{
	return node->next;
}

/**
 * @brief Returns previous element.
 *
 * This function returns previous element relatively to the given element.
 *
 * @param[in]      node           pointer to a node object.
 *
 * @return Pointer to an element if successful. The NULL pointer means
 *         that node is the first element on the list.
 */
static inline struct node *list_prev(const struct node *node)
{
	return node->prev;
}

/**
 * @brief Returns head of a list.
 *
 * This function returns a head of a list.
 *
 * @param[in]      list           pointer to a list object.
 *
 * @return Pointer to an element if successful. The NULL pointer means that
 *         there's no element on a list.
 */
static inline struct node *list_head(const struct list *list)
{
	return list->head;
}

/**
 * @brief Returns tail of a list.
 *
 * This function returns a tail of a list.
 *
 * @param[in]      list           pointer to a list object.
 *
 * @return Pointer to an element if successful. The NULL pointer means that
 *         there's no element on a list.
 */
static inline struct node *list_tail(const struct list *list)
{
	return list->tail;
}

/**
 * @brief Checks if a list is empty.
 *
 * This function checks if a list object has elements.
 *
 * @param[in]      list           pointer to a list object.
 *
 * @return 1 if list is empty, otherwise the function returns 0.
 */
static inline int list_is_empty(const struct list *list)
{
	return (list->head == NULL);
}

/**
 * @brief Appends an item before, if compar_fn returns true.
 *
 * @param[in]      list           pointer to a list object.
 * @param[in]      item           data item to be inserted into the list.
 * @param[in]      compar_fn      function to compare item with items from the list.
 *
 * @return true on success, else false on memory allocation failure.
 */
bool list_insert_compar(struct list *list, void *item, bool compar_fn(void *item1, void *item2));

#endif				/* _LIST_H_INCLUDED_ */
