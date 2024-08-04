#include "test_bitmap.unit.h"

void setUp(void) {}

void tearDown(void) {}

void test_bitmap_get() {
    unsigned long data[] = {0x4142434445464748, 0x6968676665646362};
    char *bits[] = {"0100000101000010010000110100010001000101010001100100011101001000",
                    "0110100101101000011001110110011001100101011001000110001101100010"};
    bitmap_t bitmap = data;
    char buf[256];

    for (size_t j = 0; j < 2; j++) {
        for (unsigned long i = 0; i < BITMAP_NUM_BITS; i++) {
            sprintf(buf, "failed at index %lu", BITMAP_NUM_BITS * j + i);
            TEST_ASSERT_TRUE_MESSAGE(
                bitmap_get(bitmap, BITMAP_NUM_BITS * j + i) == (bits[j][BITMAP_NUM_BITS - 1 - i] != '0'), buf);
        }
    }
}

void test_bitmap_set() {
    unsigned long data[] = {0x4142434445464748, 0x6968676665646362};
    unsigned long expected[] = {0xffffffffffffffff, 0xffffffffffffffff};
    bitmap_t bitmap = data;

    for (unsigned long i = 0; i < 2 * BITMAP_NUM_BITS; i++) {
        bitmap_set(bitmap, i);
    }
    
    TEST_ASSERT_EQUAL_UINT64_ARRAY(expected, data, 2);
}

void test_bitmap_clear() {
    unsigned long data[] = {0x4142434445464748, 0x6968676665646362};
    unsigned long expected[] = {0x0, 0x0};
    bitmap_t bitmap = data;

    for (unsigned long i = 0; i < 2 * BITMAP_NUM_BITS; i++) {
        bitmap_clear(bitmap, i);
    }
    
    TEST_ASSERT_EQUAL_UINT64_ARRAY(expected, data, 2);
}

void test_bitmap_toggle() {
    unsigned long data[] = {0x4142434445464748, 0x6968676665646362};
    unsigned long expected[] = {0xbebdbcbbbab9b8b7, 0x969798999a9b9c9d};
    bitmap_t bitmap = data;

    for (unsigned long i = 0; i < 2 * BITMAP_NUM_BITS; i++) {
        bitmap_toggle(bitmap, i);
    }
    
    TEST_ASSERT_EQUAL_UINT64_ARRAY(expected, data, 2);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_bitmap_get);
    RUN_TEST(test_bitmap_set);
    RUN_TEST(test_bitmap_clear);
    RUN_TEST(test_bitmap_toggle);

    return UNITY_END();
}
