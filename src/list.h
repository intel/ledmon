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

/**
 * This data-type represents a prototype of test function. Test function is used
 * by list_first_that() and list_last_that() functions as 'test' parameter. The
 * function should return 0 if search should continue, otherwise it should
 * return 1.
 */
typedef int (*test_t) (const void *item, const void *param);

/**
 * This data-type represents a prototype of action function. Action function is
 * used by list_for_each() function as 'action' parameter.
 */
typedef void (*action_t) (void *item, void *param);

/**
 */
#define list_for_each_parm(__list, __action, __parm) \
	__list_for_each((void *)(__list), \
				(action_t)(__action), (void *)(__parm))

/**
 */
#define list_for_each(__list, __action) \
	__list_for_each((void *)(__list), (action_t)(__action), (void *)0)

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
 * @brief Creates a list object.
 *
 * The function allocates memory for a new list object and initializes its
 * fields to reflect an empty state.
 *
 * @return pointer to allocated list if successful, otherwise NULL.
 */
struct list *list_alloc(void);

/**
 * @brief Finalizes a list object.
 *
 * This function releases the memory allocated for a list object. It also frees
 * the data items attached to list nodes.
 *
 * @param[in]      list           pointer to a list object.
 */
void list_fini(struct list *list);

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

/**
 * @brief Walks through each element.
 *
 * This function invokes the action function for each element on a list.
 * Refer to action_t data-type for details about the function prototype.
 *
 * @param[in]      list           pointer to list object.
 * @param[in]      action         pointer to an action function.
 * @param[in]      parm           additional parameter to pass directly to
 *                                'action' function.
 */
void __list_for_each(struct list *list, action_t action, void *parm);

/**
 * @brief Searches for an element.
 *
 * This function searches for an element on a list. The function 'test' is
 * called for each element on the list and if element matches the search will
 * stop. The function starts searching from head of the list and moves to
 * next element relatively.
 *
 * @param[in]      list           pointer to list object.
 * @param[in]      test           pointer to an test function.
 * @param[in]      parm           additional parameter to pass directly to
 *                                'test' function.
 *
 * @return Pointer to an element. If the function returns NULL that means there
 *         is no such an element on a list.
 */
void *list_last_that(struct list *list, test_t test, const void *parm);

/**
 * @brief Searches for an element backward.
 *
 * This function searches for an element on a list. The function 'test' is
 * called for each element on the list and if element matches the search will
 * stop. The function starts searching from tail of the list and moves to
 * previous element relatively.
 *
 * @param[in]      list           pointer to list object.
 * @param[in]      test           pointer to an test function.
 * @param[in]      parm           additional parameter to pass directly to
 *                                'test' function.
 *
 * @return Pointer to an element. If the function returns NULL that means there
 *         is no such an element on a list.
 */
void *list_first_that(struct list *list, test_t test, const void *parm);

#endif				/* _LIST_H_INCLUDED_ */
