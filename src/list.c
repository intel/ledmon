/* -*- mode: c; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ex: set tabstop=2 softtabstop=2 shiftwidth=2 expandtab: */

/*
 * Intel(R) Enclosure LED Utilities
 * Copyright (C) 2009, Intel Corporation.
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

#include <config.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <stdio.h>
#include <assert.h>

#if _HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include "status.h"
#include "list.h"
#include "version.h"

/**
 */
enum type {
  TYPE_LIST = 0,
  TYPE_NODE,
};

/**
 */
struct node {
  struct node *next, *prev;
  void *list;
  enum type type;
} __attribute__((packed));

/**
 */
struct list {
  struct node *head, *tail;
  enum type type;
} __attribute__((packed));

/**
 */
#define _Type(_ptr) \
  *(((enum type *)(_ptr)) - 1)

/**
 */
#ifdef _DEBUG
#define Check(_ptr, _type) \
  assert(_Type(_ptr) == (_type))
#else /* _DEBUG */
#define Check(_item, _type)
#endif /* _DEBUG */


/**
 */
#define _Node(_ptr) \
  (((struct node *)(_ptr)) - 1)

/**
 */
#define _List(_ptr) \
  (((struct list *)(_ptr)) - 1)

/**
 */
static struct node *_new(void *data, size_t size)
{
  struct node *t;

  t = malloc(size + sizeof(struct node));
  if (t != NULL) {
    t->list = NULL;
    t->type = TYPE_NODE;
    t->prev = NULL;
    t->next = NULL;
    memcpy(t + 1, data, size);
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
  if (list == NULL) {
    return STATUS_INVALID_NODE;
  }  
  if (node->prev) {
    node->prev->next = node->next;
  } else {
    list->head = node->next;
  }
  if (node->next) {
    node->next->prev = node->prev;
  } else {
    list->tail = node->prev;
  }
  node->list = NULL;
  node->next = NULL;
  node->prev = NULL;
  
  return STATUS_SUCCESS;
}

/**
 */
static struct node *_next(void *ptr)
{
  switch (_Type(ptr)) {
  case TYPE_NODE:
    return _Node(ptr)->next;
  case TYPE_LIST:
    return _List(ptr)->head;
  }
  abort();
}

/**
 */
static struct node *_prev(void *ptr)
{
  switch (_Type(ptr)) {
  case TYPE_NODE:
    return _Node(ptr)->prev;
  case TYPE_LIST:
    return _List(ptr)->tail;
  }
  abort();
}

/**
 */
static struct node *_get_last(struct node *node)
{
  while (node) {
    if (node->next == NULL)
      break;
    node = node->next;
  }
  return node;
}

/**
 */
static struct node *_get_first(struct node *node)
{
  while (node) {
    if (node->prev == NULL)
      break;
    node = node->prev;
  }
  return node;
}

/**
 */
static void _put_before(
    struct node *node, struct node *elem)
{
  struct node *t;

  t = node->prev;
  node->prev = elem;
  elem->prev = t;
  if (t != NULL) {
    t->next = elem;
  }
  elem->next = node;
  elem->list = node->list;
}

/**
 */
static void _put_after(
    struct node *node, struct node *elem)
{
  struct node *t;

  t = node->next;
  node->next = elem;
  elem->next = t;
  if (t != NULL) {
    t->prev = elem;
  }
  elem->prev = node;
  elem->list = node->list;
}

/**
 */
static void _put_front(
    struct list *list, struct node *elem)
{
  if (list->head == NULL) {
    list->tail = elem;
  } else {
    elem->next = list->head;
    list->head->prev = elem;
  }
  list->head = elem;
  elem->list = list;
}

/**
 */
static void _put_back(
    struct list *list, struct node *elem)
{
  if (list->tail == NULL) {
    list->head = elem;
  } else {
    elem->prev = list->tail;
    list->tail->next = elem;
  }
  list->tail = elem;
  elem->list = list;
}

/**
 */
static struct node *_tail(void *ptr)
{
  switch (_Type(ptr)) {
  case TYPE_NODE:
    return _get_last(_Node(ptr));
  case TYPE_LIST:
    return _List(ptr)->tail;
  }
  abort();
}

/**
 */
static struct node *_head(void *ptr)
{
  switch (_Type(ptr)) {
  case TYPE_NODE:
    return _get_first(_Node(ptr));
  case TYPE_LIST:
    return _List(ptr)->head;
  }
  abort();
}

/**
 */
status_t list_init(void **ptr)
{
  struct list *t;

  t = malloc(sizeof(struct list));
  if (t == NULL) {
    return STATUS_OUT_OF_MEMORY;
  }
  t->head = NULL;
  t->tail = NULL;
  t->type = TYPE_LIST;

  *ptr = t + 1;
  return STATUS_SUCCESS;
}

/**
 */
status_t list_fini(void *ptr)
{
  if (ptr != NULL) {
    Check(ptr, TYPE_LIST);
    _fini(_List(ptr));
  }
  return STATUS_SUCCESS;
}

/**
 */
status_t list_remove(void *ptr)
{
  if (ptr == NULL) {
    return STATUS_NULL_POINTER;
  }  
  Check(ptr, TYPE_NODE);
  return _remove(_Node(ptr));
}

/**
 */
void *list_add(void *ptr, void *data, size_t size)
{
  void *result = NULL;

  if (ptr && data && (size > 0)) {
    if ((result = _new(data, size)) != NULL) {
      switch (_Type(ptr)) {
      case TYPE_LIST:
        _put_front(_List(ptr), result);
        break;
      case TYPE_NODE:
        _put_before(_Node(ptr), result);
        break;
      default:
        abort();
      }
    }
  }
  return result + sizeof(struct node);
}

/**
 */
void *list_put(void *ptr, void *data, size_t size)
{
  void *result = NULL;

  if (ptr && data && (size > 0)) {
    if ((result = _new(data, size)) != NULL) {
      switch (_Type(ptr)) {
      case TYPE_LIST:
        _put_back(_List(ptr), result);
        break;
      case TYPE_NODE:
        _put_after(_Node(ptr), result);
        break;
      default:
        abort();
      }
    }
  }
  return result + sizeof(struct node);
}

/**
 */
void *list_next(void *ptr) 
{
  if (ptr) {
    struct node *node = _next(ptr);
    if (node) {
      return node + 1;
    }
  }
  return NULL;
}

/**
 */
void *list_prev(void *ptr)
{
  if (ptr) {
    struct node *node = _prev(ptr);
    if (node) {
      return node + 1;
    }
  }
  return NULL;
}

/**
 */
void *list_head(void *ptr)
{
  if (ptr) {
    struct node *node = _head(ptr);
    if (node) {
      return node + 1;
    }
  }
  return NULL;
}

/**
 */
void *list_tail(void *ptr)
{
  if (ptr) {
    struct node *node = _tail(ptr);
    if (node) {
      return node + 1;
    }
  }
  return NULL;
}

/**
 */
status_t list_delete(void *ptr)
{
  if (ptr == NULL) {
    return STATUS_NULL_POINTER;
  }
  switch (_Type(ptr)) {
  case TYPE_NODE:
    _delete(_Node(ptr));
    break;
  case TYPE_LIST:
    _list_delete(_List(ptr));
    break;
  }
  return STATUS_SUCCESS;
}

/**
 */
int list_is_empty(void *ptr)
{
  if (ptr == NULL) {
    return STATUS_NULL_POINTER;
  }
  Check(ptr, TYPE_LIST);
  return (_List(ptr)->head == NULL);
}

/**
 */
status_t __list_for_each(void *ptr, action_t action, void *parm)
{
  assert(action != NULL);

  void *node = list_head(ptr);
  while (node != NULL) {
    action(node, parm);
    node = list_next(node);
  }
  return STATUS_SUCCESS;
}

/**
 */
void *__list_first_that(void *ptr, test_t test, void *parm)
{
  assert(test != NULL);

  void *node = list_head(ptr);
  while (node) {
    if (test(node, parm)) {
      break;
    }
    node = list_next(node);
  }
  return node;
}

/**
 */
void *__list_last_that(void *ptr, test_t test, void *parm)
{
  assert(test != NULL);

  void *node = list_tail(ptr);
  while (node) {
    if (test(node, parm)) {
      break;
    }
    node = list_prev(node);
  }
  return node;
}
