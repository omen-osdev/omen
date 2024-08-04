#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/managers/dev/devices.h>
#include <omen/libraries/std/stddef.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stdint.h>

#include "dcon.h"

uint64_t dcon_dd_read(uint64_t id, uint64_t size, uint64_t offset, uint8_t* buffer) {
   return 0;
}

uint64_t dcon_dd_write(uint64_t id, uint64_t size, uint64_t offset, uint8_t* buffer) {
   void (*terminal_writer)(const char*, uint64_t) = (void (*)(const char*, uint64_t))id;
   terminal_writer((const char*)buffer, size);
}

uint64_t dcon_dd_ioctl(uint64_t id, uint32_t request, void* data) {
   return 0;
}

struct file_operations dcon_fops = {
   .read = dcon_dd_read,
   .write = dcon_dd_write,
   .ioctl = dcon_dd_ioctl
};

void init_dcon_dd() {
   register_char(DEVICE_DCON, DCON_DD_NAME, &dcon_fops);
   //We register now a virtual device for the DCON

   void (*terminal_writer)(const char*, uint64_t) = get_terminal_writer();
   char * name = device_create(NULL, DEVICE_DCON, (uint64_t)terminal_writer);

   const char str[] = "DCON device registered\n";
   device_write(name, strlen(str), 0, (uint8_t*)str);
}

void hook_dcon(void * writer) {
   void (*terminal_writer)(const char*, uint64_t) = (void (*)(const char*, uint64_t))writer;
   char * name = device_create(NULL, DEVICE_DCON, (uint64_t)terminal_writer);

   const char str[] = "DCON device registered\n";
   device_write(name, strlen(str), 0, (uint8_t*)str);
}