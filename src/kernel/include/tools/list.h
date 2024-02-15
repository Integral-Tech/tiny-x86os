// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LIST_H
#define LIST_H

#include "comm/types.h"

#define offset_in_parent(parent_type, node_name)                               \
  ((uint32_t) & (((parent_type *)0)->node_name))

#define parent_addr(node, parent_type, node_name)                              \
  (((uint32_t)(node)) - (offset_in_parent(parent_type, node_name)))

#define list_node_parent(node, parent_type, node_name)                         \
  ((parent_type *)((node) ? (parent_addr(node, parent_type, node_name)) : 0))

typedef struct _list_node_t {
  struct _list_node_t *prev, *next;
} list_node_t;

#define list_node_init(node) ((node)->prev) = ((node)->next) = NULL
#define list_node_prev(node) ((node)->prev)
#define list_node_next(node) ((node)->next)

typedef struct _list_t {
  list_node_t *first, *last;
  int count;
} list_t;

void list_init(list_t *list);

#define list_is_empty(list) ((list)->count == 0)
#define list_cnt(list) ((list)->count)
#define list_first(list) ((list)->first)
#define list_last(list) ((list)->last)

void list_insert_first(list_t *list, list_node_t *node);
void list_insert_last(list_t *list, list_node_t *node);
list_node_t *list_remove_first(list_t *list);
list_node_t *list_remove(list_t *list, list_node_t *node);

#endif
