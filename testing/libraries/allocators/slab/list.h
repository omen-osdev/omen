#ifndef _LIST_H
#define _LIST_H

struct list_head {
    struct list_head *next, *prev;
};

//Functions to manipulate the list
void list_add(struct list_head *new, struct list_head *head);
void list_del(struct list_head *entry);
void list_del_init(struct list_head *entry);
void list_add_tail(struct list_head *new, struct list_head *head);
void list_move(struct list_head *list, struct list_head *head);
void list_move_tail(struct list_head *list, struct list_head *head);
int list_empty(const struct list_head *head);

#endif