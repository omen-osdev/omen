#ifndef LIST_H
#define LIST_H

#include "modules/basics/standardLibrary/stdbool.h"

struct list_head {
    struct list_head *prev, *next;
};

/*
 * It would be nice to make these constants architecture dependant as Linux does
 * in https://github.com/torvalds/linux/blob/master/include/linux/poison.h
 */
#define LIST_POISON1 ((void *)0x00100100)
#define LIST_POISON2 ((void *)0x00200200)

#define container_of(ptr, type, member)                                                                                \
    ({                                                                                                                 \
        const typeof(((type *)0)->member) *__mptr = (ptr);                                                             \
        (type *)((char *)__mptr - offsetof(type, member));                                                             \
    })

#define LIST_HEAD_INIT(name) {&(name), &(name)}

/**
 * LIST_HEAD - Declare a list head and initialize it
 * @name: name
 */
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

/**
 * INIT_LIST_HEAD - Initialize an empty list head
 * @head: pointer to the list head
 */
static inline void INIT_LIST_HEAD(struct list_head *head) {
    head->prev = head;
    head->next = head;
}

/* List manipulation */

/**
 * _list_add() - Insert a new entry between two known consecutive entries
 * @_new: pointer to the new node
 * @prev: pointer to the previous node
 * @next: pointer to the next node
 */
static inline void _list_add(struct list_head *_new, struct list_head *prev, struct list_head *next) {
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

/**
 * list_add() - Add a list node to the beginning of the list
 * @_new: pointer to the new node
 * @head: pointer to the head of the list
 */
static inline void list_add(struct list_head *_new, struct list_head *head) { _list_add(_new, head, head->next); }

/**
 * list_add_tail() - Add a list node to the end of the list
 * @_new: pointer to the new node
 * @head: pointer to the head of the list
 */
static inline void list_add_tail(struct list_head *_new, struct list_head *head) { _list_add(_new, head->prev, head); }

/**
 * _list_del() - Delete a list node by making the prev/next nodes point to each other
 * @prev: pointer to the previous node
 * @next: pointer to the next node
 */
static inline void _list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

/**
 * list_del() - Delete a list node from the list
 * @node: pointer to the node
 *
 * List poisoning is used to prevent an invalid memory access when the memory behind the prev/next pointer is used after
 * a call to list_del
 */
static inline void list_del(struct list_head *node) {
    _list_del(node->prev, node->next);
    node->prev = (struct list_head *)LIST_POISON1;
    node->next = (struct list_head *)LIST_POISON2;
}

/**
 * list_del_init() - Delete a list node from the list and reinitialize it
 * @node: pointer to the node
 */
static inline void list_del_init(struct list_head *node) {
    _list_del(node->prev, node->next);
    INIT_LIST_HEAD(node);
}

/**
 * list_move() - Move a list node to the beginning of the list
 * @node: pointer to the node
 * @head: pointer to the head of the list
 *
 * The @node is removed from its old position/node and add to the beginning of @head
 */
static inline void list_move(struct list_head *node, struct list_head *head) {
    list_del(node);
    list_add(node, head);
}

/**
 * list_move_tail() - Move a list node to the end of the list
 * @node: pointer to the node
 * @head: pointer to the head of the list
 *
 * The @node is removed from its old position/node and add to the end of @head
 */
static inline void list_move_tail(struct list_head *node, struct list_head *head) {
    list_del(node);
    list_add_tail(node, head);
}

/**
 * list_empty() - Checks if a list is empty
 * @head: pointer to the head of the list
 */
static inline bool list_empty(const struct list_head *head) { return head->next == head; }

static inline void _list_splice(struct list_head *list, struct list_head *head) {
    struct list_head *first = list->next;
    struct list_head *last = list->prev;
    struct list_head *at = head->next;

    first->prev = head;
    head->next = first;

    last->next = at;
    at->prev = last;
}

static inline void _list_splice_tail(struct list_head *list, struct list_head *head) {
    struct list_head *first = list->next;
    struct list_head *last = list->prev;
    struct list_head *at = head->prev;

    first->prev = at;
    at->next = first;

    last->next = head;
    head->prev = last;
}

/**
 * list_splice() - Add list nodes from a list to the beginning of another list
 * @list: pointer to the head of the list to add
 * @head: pointer to the head of the other list
 */
static inline void list_splice(struct list_head *list, struct list_head *head) {
    if (list_empty(list))
        return;

    _list_splice(list, head);
}

/**
 * list_splice_tail() - Add list nodes from a list to the end of another list
 * @list: pointer to the head of the list to add
 * @head: pointer to the head of the other list
 */
static inline void list_splice_tail(struct list_head *list, struct list_head *head) {
    if (list_empty(list))
        return;

    _list_splice_tail(list, head);
}

/**
 * list_splice_init() - Add list nodes from a list to the beginning of another
 * list and initialize the first one
 * @list: pointer to the head of the list to add
 * @head: pointer to the head of the other list
 */
static inline void list_splice_init(struct list_head *list, struct list_head *head) {
    if (list_empty(list))
        return;

    _list_splice(list, head);
    INIT_LIST_HEAD(list);
}

/**
 * list_splice_tail_init() - Add list nodes from a list to the end of another
 * list and initialize the first one
 * @list: pointer to the head of the list to add
 * @head: pointer to the head of the other list
 */
static inline void list_splice_tail_init(struct list_head *list, struct list_head *head) {
    if (list_empty(list))
        return;

    _list_splice_tail(list, head);
    INIT_LIST_HEAD(list);
}

/* List iteration */

/**
 * list_entry() - Calculate address of entry that contains list node
 * @node: pointer to list node
 * @type: type of the entry containing the list node
 * @member: name of the list_head member variable in struct @type
 *
 * Return: @type pointer of entry containing node
 */
#define list_entry(node, type, member) container_of(node, type, member)

/**
 * list_first_entry() - Get the first entry of the list
 * @head: pointer to the head of the list
 * @type: type of the entry containing the list node
 * @member: name of the list_head member variable in struct @type
 *
 * Return: @type pointer of the first entry in the list
 */
#define list_first_entry(head, type, member) list_entry((head)->next, type, member)

/**
 * list_last_entry() - Get the last entry of the list
 * @head: pointer to the head of the list
 * @type: type of the entry containing the list node
 * @member: name of the list_head member variable in struct @type
 *
 * Return: @type pointer of the last entry in the list
 */
#define list_last_entry(head, type, member) list_entry((head)->prev, type, member)

/**
 * list_for_each() - Iterate over list nodes
 * @node: list_head pointer used as iterator
 * @head: pointer to the head of the list
 *
 * The nodes and the head of the list must must be kept unmodified while iterating through it. Any modifications to the
 * the list will cause undefined behavior.
 */
#define list_for_each(node, head) for (node = (head)->next; node != (head); node = node->next)

/**
 * list_for_each_entry() - Iterate over list entries
 * @entry: pointer used as iterator
 * @head: pointer to the head of the list
 * @member: name of the list_head member variable in struct type of @entry
 *
 * The nodes and the head of the list must must be kept unmodified while iterating through it. Any modifications to the
 * the list will cause undefined behavior.
 */
#define list_for_each_entry(entry, head, member)                                                                       \
    for (entry = list_entry((head)->next, typeof(*entry), member); &entry->member != (head);                           \
         entry = list_entry(entry->member.next, typeof(*entry), member))

/**
 * list_for_each_entry_safe() - Iterate over list entries
 * @entry: pointer used as iterator
 * @head: pointer to the head of the list
 * @member: name of the list_head member variable in struct type of @entry
 *
 * The current node (iterator) is allowed to be removed from the list. Any other modifications to the the list will
 * cause undefined behavior.
 */
#define list_for_each_entry_safe(entry, safe, head, member)                                                            \
    for (entry = list_entry((head)->next, typeof(*entry), member),                                                     \
        safe = list_entry(entry->member.next, typeof(*entry), member);                                                 \
         &entry->member != (head); entry = safe, safe = list_entry(safe->member.next, typeof(*entry), member))

#endif
