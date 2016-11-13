/**
 * @file list.c
 * @brief Implemention of the doubly linked list API.
 *
 * This list is a circular list implementation. The list is empty when there is
 * only a dummy head in it.
 *
 * @author Zhan Chen (zhanc1)
 * @author X.D. Zhai (xingdaz)
 */

/* Public APIs */
#include <list.h>
#include <stddef.h> /* NULL */

/**
 * @brief Check if the list is empty.
 * @param l Pointer to dummy head.
 * @return 1 if only dummy head is in list, 0 otherwise.
 */
int list_empty(list_ptr l) {
  return l->prev == l;
}

/**
 * @brief Add an entry after prev
 * @param prev Entry after which the new entry is added.
 * @param entry Entry to be added.
 */
void list_add(list_ptr prev, list_ptr entry) {
  entry->next = prev->next;
  entry->prev = prev;
  prev->next->prev = entry;
  prev->next = entry;

}

void list_init(list_ptr head) {
  head->prev = head->next = head;
}

list_ptr list_remv(list_ptr entry) {
  entry->prev->next = entry->next;
  entry->next->prev = entry->prev;
  return entry;
}

list_ptr list_remv_head(list_ptr l) {
  if (list_empty(l))
    return NULL;
  /* The first data element is the one after the dummy head */
  return list_remv(l->next);
}

void list_add_tail(list_ptr l, list_ptr entry) {
  /* The list is circular, so the last element is l->prev */
  return list_add(l->prev, entry);
}

void list_add_head(list_ptr l, list_ptr entry){
  return list_add(l, entry);
}

void list_append(list_ptr head_1, list_ptr head_2){
      	head_1->prev->next = head_2->next;
      	head_2->next->prev = head_1->prev;
      	head_2->prev->next = head_1;
      	head_1->prev = head_2->prev;

	head_2->prev = head_2->next = head_2;
}

