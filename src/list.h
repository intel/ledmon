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

#ifndef _LIST_H_INCLUDED_
#define _LIST_H_INCLUDED_

struct node {
	struct node *next, *prev;
	struct list *list;
	void *item;
};

struct list {
	struct node *head, *tail;
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
	     __node && ((__item = __node->item) || (!__node->item)); \
	     __node = __iter_fn(__node))

#define list_for_each(__list, __item) \
	__list_for_each(__list, __item, list_head, list_next)

#define list_for_each_reverse(__list, __item) \
	__list_for_each(__list, __item, list_tail, list_prev)

/**
 * @brief Initializes a list object.
 *
 * Initializes a list object to reflect an empty state.
 */
static inline void list_init(struct list *list)
{
	list->head = NULL;
	list->tail = NULL;
}

/**
 * @brief Clears a list and frees the items it contains.
 *
 * This function releases the memory allocated for a list object. It also frees
 * the data items attached to list nodes. It does not free the list itself.
 *
 * @param[in]      list           pointer to a list object.
 */
void list_erase(struct list *list);

/**
 * @brief Removes an element from the list.
 *
 * This function removes an element from the list. It only detaches the element
 * and does not release the memory allocated for the element. To free memory
 * allocated for an element use free() on node after calling this function.
 *
 * @param[in]      node           pointer to a node object.
 */
void list_remove(struct node *node);

/**
 * @brief Clears a list.
 *
 * This function removes and deallocates all elements from the list.
 *
 * @param[in]      list           pointer to a list object.
 */
void list_clear(struct list *list);

/**
 * @brief Inserts an element into the list.
 *
 * This function puts an element after a given element.
 *
 * @param[in]      list           pointer to list object.
 * @param[in]      item           data item to be inserted into the list.
 * @param[in]      after          list node after which to insert the element.
 *                                If NULL, then insert at the head of the list.
 */
void list_insert(struct list *list, void *item, struct node *after);

/**
 * @brief Appends an element to the end of the list.
 *
 * This function puts an element on tail of a list.
 *
 * @param[in]      list           pointer to list object.
 * @param[in]      item           data item to be inserted into the list.
 */
static inline void list_append(struct list *list, void *item)
{
	list_insert(list, item, list->tail);
}

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

#endif				/* _LIST_H_INCLUDED_ */
