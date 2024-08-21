#include <generic/config.h>
#include <omen/managers/mem/pmm.h>
#include <omen/libraries/std/error.h>
#include <omen/libraries/std/math.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stdint.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/allocators/buddy_allocator.h>
#include <omen/managers/boot/bootloaders/limine/limine.h>

#define BOOTLOADER_MEMMAP_USABLE			0
#define BOOTLOADER_MEMMAP_RESERVED			1
#define BOOTLOADER_MEMMAP_ACPI_RECLAIMABLE		2
#define BOOTLOADER_MEMMAP_ACPI_NVS			3
#define BOOTLOADER_MEMMAP_BAD_MEMORY			4
#define BOOTLOADER_MEMMAP_BOOTLOADER_RECLAIMABLE	5
#define BOOTLOADER_MEMMAP_KERNEL_AND_MODULES		6
#define BOOTLOADER_MEMMAP_FRAMEBUFFER			7

__attribute__((used, section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST,
	.revision = 0
};

struct pmm_block {
    uint64_t base;
    uint64_t size;
    uint64_t type;
};

struct pmm_block main_memory;
buddy_allocator_t * buddy;
uint8_t ready = 0;

void pmm_init() {
    if (ready) {
        kprintf("PMM already initialized\n");
        return;
    }
    uint64_t entries = memmap_request.response->entry_count;
    uint64_t biggest = 0;
    int64_t biggest_index = -1;
    for (uint64_t i = 0; i < entries; i++) {
	    uint64_t type = memmap_request.response->entries[i]->type;
	    uint64_t length = memmap_request.response->entries[i]->length;
        if (type == BOOTLOADER_MEMMAP_USABLE && length > biggest) {
            biggest = length;
            biggest_index = i;
            break;
        }
    }

    if (biggest_index == -1) {
        kprintf("No usable memory found\n");
        return;
    }

    main_memory.base = memmap_request.response->entries[biggest_index]->base;
    main_memory.size = memmap_request.response->entries[biggest_index]->length;
    main_memory.type = memmap_request.response->entries[biggest_index]->type;

    //TODO: Jonbardo, xq?
    uint64_t power = 0;
    uint64_t size = main_memory.size;
    while (size > 0) {
        size >>= 1;
        power++;
    }
    power--;
    kprintf("Real: %lu Used: %lu Lost: %lu\n", main_memory.size, 1 << power, main_memory.size - (1 << power));
    main_memory.size = 1 << power;

    buddy = buddy_create((void*)main_memory.base, main_memory.size, PAGE_SIZE);

    if (buddy == NULL) {
        kprintf("Failed to create buddy allocator\n");
        ready = 0;
    } else {
        ready = 1;
    }
}

void pmm_list_map() {
    if (!ready) {
        return;
    }
    uint64_t entries = memmap_request.response->entry_count;
    kprintf("Memory map entries: %d\n", entries);
    for (uint64_t i = 0; i < entries; i++) {
    	uint64_t base = memmap_request.response->entries[i]->base;
    	uint64_t length = memmap_request.response->entries[i]->length;
    	uint64_t type = memmap_request.response->entries[i]->type;
        kprintf("Entry %d: base: 0x%x, length: 0x%x, type: %d\n", i, base, length, type);
    }
}

void * pmm_alloc(uint64_t size) {
    if (!ready) {
        return NULL;
    }
    return buddy_alloc(buddy, size);
}

void pmm_free(void * ptr) {
    if (!ready) {
        return;
    }
    buddy_free(buddy, ptr);
}
