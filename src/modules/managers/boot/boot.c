#include <omen/managers/boot/boot.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/managers/dev/devices.h>
#include <omen/libraries/std/stddef.h>
#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/mem/pmm.h>
#include <omen/managers/mem/vmm.h>
#include <omen/managers/cpu/cpu.h>
#include <emulated/dcon.h>
#include <serial/serial.h>

void boot_startup() {
    init_bootloader();
    init_devices();
    char * dcon = init_dcon_dd();
    if (dcon == NULL) {
        DBG_ERROR("Failed to initialize DCON device\n");
    }

    init_debugger(dcon);
    set_current_tty(dcon);
    init_cpus();
    device_list();


    //Initialize the serial port
    if(init_serial_dd() != SUCCESS)
    {
        DBG_ERROR("Failed to initialize Serial driver\n");
    } else {
        DBG_INFO("Serial driver was initialized successfully!\n");
        char * main_serial = create_serial_dd(COM1_BASEADDR, baud_115200);
        if (main_serial == NULL) {
            DBG_ERROR("Failed to create serial device\n");
        } else {
            DBG_INFO("Serial device %s created successfully\n", main_serial);
        }
    }

    kprintf("Booting from %s %s\n", get_bootloader_name(), get_bootloader_version());

    pmm_init();
    vmm_init();
    pmm_list_map();

    kprintf("Booting kernel\n");

    kprintf("VMM Testing\n");

    char * test = (char*)pmm_alloc(0x1000);
    char * dummy_addr = (char*)pmm_alloc(0x1000);

    char * vmm_buffer_1 = 0x100000;
    char * vmm_buffer_2 = dummy_addr;
    pdTable* pml4 = vmm_get_pml4();
    kprintf("vmm_buffer_1 physical address before: %p\n", vmm_virt_to_phys(vmm_buffer_1, pml4));
    kprintf("vmm_buffer_2 physical address before: %p\n", vmm_virt_to_phys(vmm_buffer_2, pml4));
    vmm_map_current(vmm_buffer_1, test, VMM_WRITE_BIT);
    vmm_map_current(vmm_buffer_2, test, VMM_WRITE_BIT);
    pageMapIndex map;
    vmm_page_to_map(vmm_buffer_1, &map);
    kprintf("PML4: %d, PDPT: %d, PD: %d, PT: %d\n", map.PDP_i, map.PD_i, map.PT_i, map.P_i);
    vmm_page_to_map(vmm_buffer_2, &map);
    kprintf("PML4: %d, PDPT: %d, PD: %d, PT: %d\n", map.PDP_i, map.PD_i, map.PT_i, map.P_i);
    vmm_set_pml4(pml4); //Invalidate TLB
    kprintf("vmm_buffer_1 physical address after: %p\n", vmm_virt_to_phys(vmm_buffer_1, pml4));
    kprintf("vmm_buffer_2 physical address after: %p\n", vmm_virt_to_phys(vmm_buffer_2, pml4));

    strcpy(vmm_buffer_1, "Hello, World!\n");
    kprintf("Buffer 1: %s\n", vmm_buffer_1);
    kprintf("Buffer 2: %s\n", vmm_buffer_2);
}
