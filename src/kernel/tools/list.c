#include "tools/list.h"

void list_init(list_t *list) {
  list->first = list->last = NULL;
  list->count = 0;
}

void list_insert_first(list_t *list, list_node_t *node) {
  node->next = list->first;
  node->prev = NULL;

  if (list_is_empty(list))
    list->last = node;
  else
    list->first->prev = node;

  list->first = node;
  list->count++;
}

void list_insert_last(list_t *list, list_node_t *node) {
  node->next = NULL;
  node->prev = list->last;

  if (list_is_empty(list))
    list->first = node;
  else
    list->last->next = node;

  list->last = node;
  list->count++;
}

list_node_t *list_remove_first(list_t *list) {
  if (list_is_empty(list))
    return NULL;

  list_node_t *removed_node = list->first;
  list->first = removed_node->next;

  if (list->first == NULL)
    list->last = NULL;
  else
    list->first->prev = NULL;

  removed_node->next = removed_node->prev = NULL;
  list->count--;
  return removed_node;
}

list_node_t *list_remove(list_t *list, list_node_t *node) {
  if (node == list->first)
    list->first = node->next;

  if (node == list->last)
    list->last = node->prev;

  if (node->prev)
    node->prev->next = node->next;

  if (node->next)
    node->next->prev = node->prev;

  node->next = node->prev = NULL;
  list->count--;
  return node;
}
