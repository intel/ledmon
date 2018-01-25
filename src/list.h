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

#include <stdlib.h>

#include "status.h"

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
 * @brief Creates a list object.
 *
 * The function allocates memory for a new list object and initializes its
 * fields to reflect an empty state.
 *
 * @param [in,out] ptr            placeholder where the pointer to the new list
 *                                will be stored. In case of an error the NULL
 *                                pointer is stored instead.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t error code.
 */
status_t list_init(struct list **ptr);

/**
 * @brief Finalizes a list object.
 *
 * This function releases the memory allocated for a list object. The function
 * does not release the memory allocated by other function and stored
 * in elements. It only releases a placeholder where the elements are stored.
 * It is user responsibility to free allocated other memory before
 * this function is called.
 *
 * @param[in]      ptr            pointer to a list object.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t list_fini(struct list *ptr);

/**
 * @brief Removes an element from the list.
 *
 * This function removes an element from the list. It only detaches the element
 * and does not release the memory allocated for the element. To free memory
 * allocated for an element use free() on ptr after calling this function.
 *
 * @param[in]      ptr            pointer to a node object.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t list_remove(struct node *ptr);

/**
 * @brief Clears a list.
 *
 * This function removes and deallocates all elements from the list.
 *
 * @param[in]      ptr            pointer to a list object.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t list_clear(struct list *ptr);

/**
 * @brief Adds an element in front.
 *
 * This function adds an element to head of the list.
 *
 * @param[in]      ptr            pointer to list object.
 * @param[in]      data           placeholder where the information to put on
 *                                a list is stored.
 * @param[in]      size           number of bytes stored in data parameter.
 *
 * @return Pointer to element on a list if successful, otherwise a NULL and
 *         this means out of memory in the system.
 */
void *list_add(struct list *ptr, void *data, size_t size);

/**
 * @brief Puts an element in back.
 *
 * This function puts an element on tail of a list.
 *
 * @param[in]      ptr            pointer to list object.
 * @param[in]      data           placeholder where the information to put on
 *                                a list is stored.
 * @param[in]      size           number of bytes stored in data parameter.
 *
 * @return Pointer to element on a list if successful, otherwise a NULL and
 *         this means out of memory in the system.
 */
void *list_put(struct list *ptr, void *data, size_t size);

/**
 * @brief Reruns next element.
 *
 * This function returns next element relatively to the given element.
 *
 * @param[in]      ptr            pointer to a node object.
 *
 * @return Pointer to an element if successful. The NULL pointer means
 *         that ptr is the last element on the list.
 */
struct node *list_next(struct node *ptr);

/**
 * @brief Returns previous element.
 *
 * This function returns previous element relatively to the given element.
 *
 * @param[in]      ptr            pointer to a node object.
 *
 * @return Pointer to an element if successful. The NULL pointer means
 *         that ptr is the first element on the list.
 */
struct node *list_prev(struct node *ptr);

/**
 * @brief Returns head of a list.
 *
 * This function returns a head of a list.
 *
 * @param[in]      ptr            pointer to a list object.
 *
 * @return Pointer to an element if successful. The NULL pointer means that
 *         there's no element on a list.
 */
struct node *list_head(struct list *ptr);

/**
 * @brief Returns tail of a list.
 *
 * This function returns a tail of a list.
 *
 * @param[in]      ptr            pointer to a list object.
 *
 * @return Pointer to an element if successful. The NULL pointer means that
 *         there's no element on a list.
 */
struct node *list_tail(struct list *ptr);

/**
 * @brief Checks if a list is empty.
 *
 * This function checks if a list object has elements.
 *
 * @param[in]      ptr            pointer to a list object.
 *
 * @return 1 if list is empty, otherwise the function returns 0.
 */
int list_is_empty(struct list *ptr);

/**
 * @brief Walks through each element.
 *
 * This function invokes the action function for each element on a list.
 * Refer to action_t data-type for details about the function prototype.
 *
 * @param[in]      ptr            pointer to list object.
 * @param[in]      action         pointer to an action function.
 * @param[in]      parm           additional parameter to pass directly to
 *                                'action' function.
 *
 * @return STATUS_SUCCESS if successful, otherwise a valid status_t status code.
 */
status_t __list_for_each(struct list *ptr, action_t action, void *parm);

/**
 * @brief Searches for an element.
 *
 * This function searches for an element on a list. The function 'test' is
 * called for each element on the list and if element matches the search will
 * stop. The function starts searching from head of the list and moves to
 * next element relatively.
 *
 * @param[in]      ptr            pointer to list object.
 * @param[in]      test           pointer to an test function.
 * @param[in]      parm           additional parameter to pass directly to
 *                                'test' function.
 *
 * @return Pointer to an element. If the function returns NULL that means there
 *         is no such an element on a list.
 */
void *list_last_that(struct list *ptr, test_t test, const void *parm);

/**
 * @brief Searches for an element backward.
 *
 * This function searches for an element on a list. The function 'test' is
 * called for each element on the list and if element matches the search will
 * stop. The function starts searching from tail of the list and moves to
 * previous element relatively.
 *
 * @param[in]      ptr            pointer to list object.
 * @param[in]      test           pointer to an test function.
 * @param[in]      parm           additional parameter to pass directly to
 *                                'test' function.
 *
 * @return Pointer to an element. If the function returns NULL that means there
 *         is no such an element on a list.
 */
void *list_first_that(struct list *ptr, test_t test, const void *parm);

#endif				/* _LIST_H_INCLUDED_ */
