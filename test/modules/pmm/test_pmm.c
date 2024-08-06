#include "omen/modules/pmm/pmm.h"

#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

#define ONE_MB 1048576
#define FOUR_KB 4096

void setUp(void) {}

void tearDown(void) {}

static void test_buddy_init() {
    const unsigned long int size = ONE_MB;
    const unsigned long int page_size = FOUR_KB;
    const unsigned long int num_levels = 9;
    const size_t metadata_size = buddy_sizeof_metadata(num_levels);

    buddy_allocator_t *allocator = malloc(metadata_size);
    void *address = malloc(size);

    buddy_init(allocator, address, size, page_size);

    TEST_ASSERT_NOT_NULL(allocator);
    TEST_ASSERT_TRUE(address == allocator->address);
    TEST_ASSERT_EQUAL_size_t(size, allocator->size);
    TEST_ASSERT_EQUAL_size_t(page_size, allocator->page_size);
    TEST_ASSERT_EQUAL_INT(num_levels, allocator->num_levels);
    TEST_ASSERT_EQUAL_size_t(ONE_MB, buddy_largest_available(allocator));
    TEST_ASSERT_EQUAL_size_t(ONE_MB, buddy_available(allocator));
    TEST_ASSERT_EQUAL_size_t(0, buddy_used(allocator));
}

static void test_buddy_level_alloc_and_free() {
    const unsigned long int size = ONE_MB;
    const unsigned long int page_size = FOUR_KB;
    const unsigned long int num_levels = 9;
    const size_t metadata_size = buddy_sizeof_metadata(num_levels);

    void *ptr, *ptr2;

    buddy_allocator_t *allocator = malloc(metadata_size);
    void *address = malloc(size);

    buddy_init(allocator, address, size, page_size);

    ptr = buddy_level_alloc(allocator, 8);
    TEST_ASSERT_EQUAL_size_t(FOUR_KB, buddy_used(allocator));

    buddy_level_free(allocator, ptr, 8);
    TEST_ASSERT_EQUAL_size_t(0, buddy_used(allocator));

    ptr = buddy_level_alloc(allocator, 6);
    TEST_ASSERT_EQUAL_size_t(4 * FOUR_KB, buddy_used(allocator));

    ptr2 = buddy_level_alloc(allocator, 6);
    TEST_ASSERT_EQUAL_size_t(8 * FOUR_KB, buddy_used(allocator));

    buddy_level_free(allocator, ptr, 6);
    TEST_ASSERT_EQUAL_size_t(4 * FOUR_KB, buddy_used(allocator));

    buddy_level_free(allocator, ptr2, 6);
    TEST_ASSERT_EQUAL_size_t(0, buddy_used(allocator));
}

static void test_buddy_alloc_and_free() {
    const unsigned long int size = ONE_MB;
    const unsigned long int page_size = FOUR_KB;
    const unsigned long int num_levels = 9;
    const size_t metadata_size = buddy_sizeof_metadata(num_levels);

    void *ptr, *ptr2;

    buddy_allocator_t *allocator = malloc(metadata_size);
    void *address = malloc(size);

    buddy_init(allocator, address, size, page_size);

    ptr = buddy_alloc(allocator, page_size);
    TEST_ASSERT_EQUAL_size_t(page_size, buddy_used(allocator));

    buddy_free(allocator, ptr);
    TEST_ASSERT_EQUAL_size_t(0, buddy_used(allocator));

    ptr = buddy_alloc(allocator, 4 * page_size);
    TEST_ASSERT_EQUAL_size_t(4 * page_size, buddy_used(allocator));

    ptr2 = buddy_alloc(allocator, 3 * page_size);
    TEST_ASSERT_EQUAL_size_t(8 * page_size, buddy_used(allocator));

    buddy_free(allocator, ptr);
    TEST_ASSERT_EQUAL_size_t(4 * page_size, buddy_used(allocator));

    buddy_free(allocator, ptr2);
    TEST_ASSERT_EQUAL_size_t(0, buddy_used(allocator));
}

static void test_buddy_alloc_all() {
    const unsigned long int size = ONE_MB;
    const unsigned long int page_size = FOUR_KB;
    const unsigned long int num_levels = 9;
    const size_t metadata_size = buddy_sizeof_metadata(num_levels);

    buddy_allocator_t *allocator = malloc(metadata_size);
    void *address = malloc(size);
    buddy_init(allocator, address, size, page_size);

    const unsigned long num_pages = POW2(num_levels - 1);
    void **ptrs = malloc(sizeof(void *) * num_pages);
    for (unsigned long int i = 0; i < num_pages; i++) {
        ptrs[i] = buddy_alloc(allocator, page_size);
        TEST_ASSERT_EQUAL_size_t((i + 1) * page_size, buddy_used(allocator));
    }
    TEST_ASSERT_EQUAL_size_t(size, buddy_used(allocator));
    TEST_ASSERT_EQUAL_size_t(0, buddy_available(allocator));

    for (unsigned long int i = 0; i < num_pages; i++) {
        buddy_free(allocator, ptrs[i]);
        TEST_ASSERT_EQUAL_size_t((i + 1) * page_size, buddy_available(allocator));
    }
    TEST_ASSERT_EQUAL_size_t(0, buddy_used(allocator));
    TEST_ASSERT_EQUAL_size_t(size, buddy_available(allocator));
}

static void test_buddy_create() {
    const unsigned long int size = ONE_MB;
    const unsigned long int page_size = FOUR_KB;
    const unsigned long int num_levels = 9;
    const size_t metadata_size = buddy_sizeof_metadata(num_levels);

    void *address = malloc(size);
    buddy_allocator_t *allocator = buddy_create(address, size, page_size);

    TEST_ASSERT_NOT_NULL(allocator);
    TEST_ASSERT_TRUE(address == allocator->address);
    TEST_ASSERT_EQUAL_size_t(size, allocator->size);
    TEST_ASSERT_EQUAL_size_t(page_size, allocator->page_size);
    TEST_ASSERT_EQUAL_INT(num_levels, allocator->num_levels);
    TEST_ASSERT_EQUAL_size_t(BUDDY_NUM_PAGES(metadata_size, page_size) * page_size, buddy_used(allocator));
}

static void test_buddy_embedded_alloc_all() {
    const unsigned long int size = ONE_MB;
    const unsigned long int page_size = FOUR_KB;
    const unsigned long int num_levels = 9;
    const size_t metadata_size = buddy_sizeof_metadata(num_levels);

    void *address = malloc(size);
    buddy_allocator_t *allocator = buddy_create(address, size, page_size);

    const unsigned long num_pages = POW2(num_levels - 1);
    const unsigned long metadata_pages = BUDDY_NUM_PAGES(metadata_size, page_size);

    void **ptrs = malloc(sizeof(void *) * (num_pages - metadata_pages));
    for (unsigned long int i = 0; i < (num_pages - metadata_pages); i++) {
        ptrs[i] = buddy_alloc(allocator, page_size);
        TEST_ASSERT_EQUAL_size_t((i + 1 + metadata_pages) * page_size, buddy_used(allocator));
    }
    TEST_ASSERT_EQUAL_size_t(size, buddy_used(allocator));
    TEST_ASSERT_EQUAL_size_t(0, buddy_available(allocator));

    for (unsigned long int i = 0; i < (num_pages - metadata_pages); i++) {
        buddy_free(allocator, ptrs[i]);
        TEST_ASSERT_EQUAL_size_t((i + 1) * page_size, buddy_available(allocator));
    }
    TEST_ASSERT_EQUAL_size_t(metadata_pages * page_size, buddy_used(allocator));
    TEST_ASSERT_EQUAL_size_t(size - metadata_pages * page_size, buddy_available(allocator));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_buddy_init);
    RUN_TEST(test_buddy_level_alloc_and_free);
    RUN_TEST(test_buddy_alloc_and_free);
    RUN_TEST(test_buddy_alloc_all);
    RUN_TEST(test_buddy_create);
    RUN_TEST(test_buddy_embedded_alloc_all);

    return UNITY_END();
}
