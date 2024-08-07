#ifndef TEST_LINKED_LIST_H
#define TEST_LINKED_LIST_H

#include "omen/libraries/basic/linked_list.h"

#include "unity.h"

#include <stdio.h>
#include <stdlib.h>

// Structs
struct list_item {
    int i;
    struct list_head list;
};

// Setup functions
void setup_items(struct list_item **items, size_t num_items);
void setup_list_with_items(struct list_head *list, size_t num_items);
void setup_list_and_items(struct list_head *list, struct list_item **items, size_t num_items);

// Tests
void test_list_add();
void test_list_add_tail();
void test_list_del();
void test_list_del_init();
void test_list_move();
void test_list_move_tail();
void test_list_splice();
void test_list_splice_tail();
void test_list_splice_init();
void test_list_splice_tail_init();
void test_list_entry();
void test_list_first_entry();
void test_list_last_entry();
void test_list_for_each();
void test_list_for_each_entry();
void test_list_for_each_entry_safe();

#endif