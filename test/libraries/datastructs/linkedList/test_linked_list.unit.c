#include "test_linked_list.unit.h"

void setUp() {}

void tearDown() {}

void setup_items(struct list_item **items, size_t num_items) {
    *items = calloc(num_items, sizeof(struct list_item));
    TEST_ASSERT_NOT_NULL(*items);

    for (size_t i = 0; i < num_items; i++) {
        (*items)[i].i = i;
    }
}

void setup_list_with_items(struct list_head *list, size_t num_items) {
    INIT_LIST_HEAD(list);
    TEST_ASSERT_TRUE(list_empty(list));

    struct list_item *items;
    setup_items(&items, num_items);

    for (size_t i = 0; i < num_items; i++) {
        list_add_tail(&(items)[i].list, list);
    }
}

void setup_list_and_items(struct list_head *list, struct list_item **items, size_t num_items) {
    INIT_LIST_HEAD(list);
    TEST_ASSERT_TRUE(list_empty(list));

    setup_items(items, num_items);
}

void test_list_add() {
    struct list_head list;
    struct list_item *items;
    struct list_item *item = NULL;

    setup_list_and_items(&list, &items, 4);

    list_add(&items[3].list, &list);
    list_add(&items[2].list, &list);
    list_add(&items[1].list, &list);
    list_add(&items[0].list, &list);

    int i = 0;
    list_for_each_entry(item, &list, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        i++;
    }
    TEST_ASSERT_EQUAL_INT(4, i);
}

void test_list_add_tail() {
    struct list_head list;
    struct list_item *items;
    struct list_item *item = NULL;

    setup_list_and_items(&list, &items, 4);

    list_add_tail(&items[0].list, &list);
    list_add_tail(&items[1].list, &list);
    list_add_tail(&items[2].list, &list);
    list_add_tail(&items[3].list, &list);

    int i = 0;
    list_for_each_entry(item, &list, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        i++;
    }
    TEST_ASSERT_EQUAL_INT(4, i);
}

void test_list_del() {
    struct list_head list;
    struct list_item item;

    INIT_LIST_HEAD(&list);
    TEST_ASSERT_TRUE(list_empty(&list));

    list_add(&item.list, &list);
    TEST_ASSERT_FALSE(list_empty(&list));

    list_del(&item.list);
    TEST_ASSERT_TRUE(list_empty(&list));
}

void test_list_del_init() {
    struct list_head list;
    struct list_item item;

    INIT_LIST_HEAD(&list);
    TEST_ASSERT_TRUE(list_empty(&list));

    list_add(&item.list, &list);
    TEST_ASSERT_FALSE(list_empty(&list));

    list_del_init(&item.list);
    TEST_ASSERT_TRUE(list_empty(&list));
    TEST_ASSERT_TRUE(list_empty(&item.list));
}

void test_list_move() {
    struct list_head list, list2;
    struct list_item *item, *is;

    setup_list_with_items(&list, 5);
    list_for_each_entry(item, &list, list) { item->i += 5; }
    TEST_ASSERT_FALSE(list_empty(&list));

    setup_list_with_items(&list2, 5);
    TEST_ASSERT_FALSE(list_empty(&list2));
    int i = 5;
    list_for_each_entry(item, &list2, list) {
        i--;
        item->i = i;
    }
    TEST_ASSERT_EQUAL_INT(0, i);

    list_for_each_entry_safe(item, is, &list2, list) { list_move(&item->list, &list); }
    TEST_ASSERT_TRUE(list_empty(&list2));

    i = 0;
    list_for_each_entry_safe(item, is, &list, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        list_del(&item->list);
        i++;
    }

    TEST_ASSERT_EQUAL_INT(10, i);
    TEST_ASSERT_TRUE(list_empty(&list));
}

void test_list_move_tail() {
    struct list_head list, list2;
    struct list_item *item, *is;

    setup_list_with_items(&list, 5);
    TEST_ASSERT_FALSE(list_empty(&list));

    setup_list_with_items(&list2, 5);
    TEST_ASSERT_FALSE(list_empty(&list2));
    list_for_each_entry(item, &list2, list) { item->i += 5; }

    list_for_each_entry_safe(item, is, &list2, list) { list_move_tail(&item->list, &list); }
    TEST_ASSERT_TRUE(list_empty(&list2));

    int i = 0;
    list_for_each_entry_safe(item, is, &list, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        list_del(&item->list);
        i++;
    }

    TEST_ASSERT_EQUAL_INT(10, i);
    TEST_ASSERT_TRUE(list_empty(&list));
}

void test_list_splice() {
    struct list_head list, list2;
    struct list_item *item, *is;

    INIT_LIST_HEAD(&list);
    TEST_ASSERT_TRUE(list_empty(&list));

    setup_list_with_items(&list2, 5);
    TEST_ASSERT_FALSE(list_empty(&list2));
    list_for_each_entry(item, &list2, list) { item->i += 5; }
    list_splice(&list2, &list);

    setup_list_with_items(&list2, 5);
    list_splice(&list2, &list);
    INIT_LIST_HEAD(&list2);

    int i = 0;
    list_for_each_entry(item, &list, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        i++;
    }
    TEST_ASSERT_EQUAL_INT(10, i);

    list_splice(&list, &list2);
    TEST_ASSERT_FALSE(list_empty(&list2));

    i = 0;
    list_for_each_entry_safe(item, is, &list2, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        list_del(&item->list);
        i++;
    }
    TEST_ASSERT_EQUAL_INT(10, i);
}

void test_list_splice_tail() {
    struct list_head list, list2;
    struct list_item *item, *is;

    INIT_LIST_HEAD(&list);
    TEST_ASSERT_TRUE(list_empty(&list));

    setup_list_with_items(&list2, 5);
    TEST_ASSERT_FALSE(list_empty(&list2));
    list_splice_tail(&list2, &list);

    setup_list_with_items(&list2, 5);
    list_for_each_entry(item, &list2, list) { item->i += 5; }
    list_splice_tail(&list2, &list);
    INIT_LIST_HEAD(&list2);

    int i = 0;
    list_for_each_entry(item, &list, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        i++;
    }
    TEST_ASSERT_EQUAL_INT(10, i);

    list_splice_tail(&list, &list2);
    TEST_ASSERT_FALSE(list_empty(&list2));

    i = 0;
    list_for_each_entry_safe(item, is, &list2, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        list_del(&item->list);
        i++;
    }
    TEST_ASSERT_EQUAL_INT(10, i);
}

void test_list_splice_init() {
    struct list_head list, list2;
    struct list_item *item, *is;

    INIT_LIST_HEAD(&list);
    TEST_ASSERT_TRUE(list_empty(&list));

    setup_list_with_items(&list2, 5);
    TEST_ASSERT_FALSE(list_empty(&list2));
    list_for_each_entry(item, &list2, list) { item->i += 5; }
    list_splice_init(&list2, &list);
    TEST_ASSERT_TRUE(list_empty(&list2));

    setup_list_with_items(&list2, 5);
    list_splice_init(&list2, &list);
    TEST_ASSERT_TRUE(list_empty(&list2));

    int i = 0;
    list_for_each_entry(item, &list, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        i++;
    }
    TEST_ASSERT_EQUAL_INT(10, i);

    list_splice_init(&list, &list2);
    TEST_ASSERT_TRUE(list_empty(&list));
    TEST_ASSERT_FALSE(list_empty(&list2));

    i = 0;
    list_for_each_entry_safe(item, is, &list2, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        list_del(&item->list);
        i++;
    }
    TEST_ASSERT_EQUAL_INT(10, i);
}

void test_list_splice_tail_init() {
    struct list_head list, list2;
    struct list_item *item, *is;

    INIT_LIST_HEAD(&list);
    TEST_ASSERT_TRUE(list_empty(&list));

    setup_list_with_items(&list2, 5);
    TEST_ASSERT_FALSE(list_empty(&list2));
    list_splice_tail_init(&list2, &list);
    TEST_ASSERT_TRUE(list_empty(&list2));

    setup_list_with_items(&list2, 5);
    list_for_each_entry(item, &list2, list) { item->i += 5; }
    list_splice_tail_init(&list2, &list);
    TEST_ASSERT_TRUE(list_empty(&list2));

    int i = 0;
    list_for_each_entry(item, &list, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        i++;
    }
    TEST_ASSERT_EQUAL_INT(10, i);

    list_splice_tail_init(&list, &list2);
    TEST_ASSERT_TRUE(list_empty(&list));
    TEST_ASSERT_FALSE(list_empty(&list2));

    i = 0;
    list_for_each_entry_safe(item, is, &list2, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        list_del(&item->list);
        i++;
    }
    TEST_ASSERT_EQUAL_INT(10, i);
}

void test_list_entry(void) {
    struct list_item item;

    TEST_ASSERT_EQUAL_PTR(&item, list_entry(&item.list, struct list_item, list));
}

void test_list_first_entry() {
    struct list_head list;
    struct list_item *items;

    setup_list_and_items(&list, &items, 4);

    list_add_tail(&items[0].list, &list);
    TEST_ASSERT_EQUAL_INT(0, list_first_entry(&list, struct list_item, list)->i);

    list_add_tail(&items[1].list, &list);
    TEST_ASSERT_EQUAL_INT(0, list_first_entry(&list, struct list_item, list)->i);

    list_add(&items[2].list, &list);
    TEST_ASSERT_EQUAL_INT(2, list_first_entry(&list, struct list_item, list)->i);

    list_add(&items[3].list, &list);
    TEST_ASSERT_EQUAL_INT(3, list_first_entry(&list, struct list_item, list)->i);
}

void test_list_last_entry() {
    struct list_head list;
    struct list_item *items;

    setup_list_and_items(&list, &items, 4);

    list_add_tail(&items[0].list, &list);
    TEST_ASSERT_EQUAL_INT(0, list_last_entry(&list, struct list_item, list)->i);

    list_add_tail(&items[1].list, &list);
    TEST_ASSERT_EQUAL_INT(1, list_last_entry(&list, struct list_item, list)->i);

    list_add(&items[2].list, &list);
    TEST_ASSERT_EQUAL_INT(1, list_last_entry(&list, struct list_item, list)->i);

    list_add(&items[3].list, &list);
    TEST_ASSERT_EQUAL_INT(1, list_last_entry(&list, struct list_item, list)->i);
}

void test_list_for_each() {
    struct list_head list;
    struct list_item *item;
    struct list_head *node = NULL;

    INIT_LIST_HEAD(&list);
    TEST_ASSERT_TRUE(list_empty(&list));

    for (int i = 0; i < 6; i++) {
        item = (struct list_item *)malloc(sizeof(*item));
        TEST_ASSERT_NOT_NULL(item);
        item->i = i;
        list_add_tail(&item->list, &list);
    }

    TEST_ASSERT_FALSE(list_empty(&list));

    int i = 0;
    list_for_each(node, &list) {
        item = list_entry(node, struct list_item, list);
        TEST_ASSERT_EQUAL_INT(i, item->i);
        i++;
    }

    TEST_ASSERT_EQUAL_INT(5, item->i);
    TEST_ASSERT_FALSE(list_empty(&list));
}

void test_list_for_each_entry() {
    struct list_head list;
    struct list_item *item;

    INIT_LIST_HEAD(&list);
    TEST_ASSERT_TRUE(list_empty(&list));

    for (int i = 0; i < 6; i++) {
        item = (struct list_item *)malloc(sizeof(*item));
        TEST_ASSERT_NOT_NULL(item);
        item->i = i;
        list_add_tail(&item->list, &list);
    }

    TEST_ASSERT_FALSE(list_empty(&list));

    int i = 0;
    list_for_each_entry(item, &list, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        i++;
    }

    TEST_ASSERT_EQUAL_INT(6, i);
    TEST_ASSERT_FALSE(list_empty(&list));
}

void test_list_for_each_entry_safe() {
    struct list_head list;
    struct list_item *item, *is;

    INIT_LIST_HEAD(&list);
    TEST_ASSERT_TRUE(list_empty(&list));

    for (int i = 0; i < 6; i++) {
        item = (struct list_item *)malloc(sizeof(*item));
        TEST_ASSERT_NOT_NULL(item);
        item->i = i;
        list_add_tail(&item->list, &list);
    }

    TEST_ASSERT_FALSE(list_empty(&list));

    int i = 0;
    list_for_each_entry_safe(item, is, &list, list) {
        TEST_ASSERT_EQUAL_INT(i, item->i);
        list_del(&item->list);
        free(item);
        i++;
    }

    TEST_ASSERT_EQUAL_INT(6, i);
    TEST_ASSERT_TRUE(list_empty(&list));
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_list_add);
    RUN_TEST(test_list_add_tail);
    RUN_TEST(test_list_del);
    RUN_TEST(test_list_del_init);
    RUN_TEST(test_list_move);
    RUN_TEST(test_list_move_tail);
    RUN_TEST(test_list_splice);
    RUN_TEST(test_list_splice_tail);
    RUN_TEST(test_list_splice_init);
    RUN_TEST(test_list_splice_tail_init);
    RUN_TEST(test_list_entry);
    RUN_TEST(test_list_first_entry);
    RUN_TEST(test_list_last_entry);
    RUN_TEST(test_list_for_each);
    RUN_TEST(test_list_for_each_entry);
    RUN_TEST(test_list_for_each_entry_safe);

    return UNITY_END();
}
