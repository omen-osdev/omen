#ifndef BITMAP_H
#define BITMAP_H

#include <stdbool.h>

typedef unsigned long int *bitmap_t;

#define BITMAP_NUM_BITS (8 * sizeof(unsigned long int))
#define BITMAP_INDEX_SHIFT (__builtin_ctzl(BITMAP_NUM_BITS))
#define BITMAP_INDEX_MASK (BITMAP_NUM_BITS - 1UL)

/**
 * bitmap_set - Sets the bit at the specified position in the bitmap
 * @bitmap: bitmap
 * @index: index of the bit in the bitmap
 */
static inline void bitmap_set(bitmap_t bitmap, unsigned long int index) {
    bitmap[index >> BITMAP_INDEX_SHIFT] |= (1UL << (index & BITMAP_INDEX_MASK));
}

/**
 * bitmap_clear - Clears the bit at the specified position in the bitmap
 * @bitmap: bitmap
 * @index: index of the bit in the bitmap
 */
static inline void bitmap_clear(bitmap_t bitmap, unsigned long int index) {
    bitmap[index >> BITMAP_INDEX_SHIFT] &= ~(1UL << (index & BITMAP_INDEX_MASK));
}

/**
 * bitmap_get - Gets the value of the bit at the specified position in the bitmap
 * @bitmap: bitmap
 * @index: index of the bit in the bitmap
 *
 * Return: the value of the desired bit
 */
static inline bool bitmap_get(const bitmap_t bitmap, unsigned long int index) {
    return (bitmap[index >> BITMAP_INDEX_SHIFT] & (1UL << (index & BITMAP_INDEX_MASK))) != 0;
}

/**
 * bitmap_toggle - Toggles the value of the bit at the specified position in the bitmap
 * @bitmap: bitmap
 * @index: index of the bit in the bitmap
 */
static inline void bitmap_toggle(bitmap_t bitmap, unsigned long int index) {
    register unsigned long int array_index = index >> BITMAP_INDEX_SHIFT;
    register unsigned long int bit_value = (1UL << (index & BITMAP_INDEX_MASK));

    if (bitmap[array_index] & bit_value)
        bitmap[array_index] &= ~bit_value;
    else
        bitmap[array_index] |= bit_value;
}

#endif
