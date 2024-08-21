#include <omen/managers/boot/boot.h>
#include <omen/managers/dev/devices.h>
#include <omen/libraries/std/stddef.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/string.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/cpu/cpu.h>
#include <omen/libraries/std/stdio.h>
#include <omen/apps/debug/debug.h>

__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;

static void hlt() {
	for(;;) {
		__asm__ __volatile__("hlt");
	}
}

void boot_startup() {

	if(LIMINE_BASE_REVISION_SUPPORTED == false) {
		hlt();
	}

	gop_initialize();
	
	gop_clear(0xffffffff);

    //pmm_init();
    //pmm_list_map();

    char * buffer = pmm_alloc(4024);
    if (buffer == NULL) {
        kprintf("Failed to allocate buffer, jonbardo plz fix\n");
        return;
    }

    kprintf("Allocated buffer at 0x%x\n", buffer);
    strcpy(buffer, "Hello, world!\n");
    strcat(buffer, "This is a test buffer\n");
    kprintf("Buffer contents: %s\n", buffer);
    pmm_free(buffer);

    kprintf("Booting kernel\n");

}
