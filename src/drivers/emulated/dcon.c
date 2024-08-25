#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/managers/dev/devices.h>
#include <omen/libraries/std/stddef.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stdint.h>

#include "dcon.h"

const char dcon_hook_str[] = "DCON device registered\n";

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

void terminal_writer(const char* buffer, uint64_t size) {
    for(uint64_t i = 0; i < size; i++) {
   	outb(0xe9, buffer[i]);
   }
}

//Reading from a debug terminal is not supported, we want to just write to it
uint64_t dcon_dd_read(uint64_t id, uint64_t size, uint64_t offset, uint8_t* buffer) {
   return 0;
}

uint64_t dcon_dd_write(uint64_t id, uint64_t size, uint64_t offset, uint8_t* buffer) {
   void (*terminal_writer)(const char*, uint64_t) = (void (*)(const char*, uint64_t))id;
   terminal_writer((const char*)buffer, size);
}

//We don't support any ioctl operations for now, just write right away
uint64_t dcon_dd_ioctl(uint64_t id, uint32_t request, void* data) {
   switch (request) {
      case DCON_IOCTL_GET_DEV:
         return id;
      default:
         return 0;
   }
}

struct file_operations dcon_fops = {
   .read = dcon_dd_read,
   .write = dcon_dd_write,
   .ioctl = dcon_dd_ioctl
};

char * init_dcon_dd() {

   //Register the device driver as a character device
   register_char(DEVICE_DCON, DCON_DD_NAME, &dcon_fops);

   //We register now a virtual device for the DCON over the bootloader's debug device
   char * name = device_create(NULL, DEVICE_DCON, (uint64_t)terminal_writer);
   if (name == NULL) {
      return NULL;
   }
   device_write(name, strlen(dcon_hook_str), 0, (uint8_t*)dcon_hook_str);
   return name;
}

char * hook_dcon(void * writer) {

   void (*terminal_writer)(const char*, uint64_t) = (void (*)(const char*, uint64_t))writer;
   char * name = device_create(NULL, DEVICE_DCON, (uint64_t)terminal_writer);
   if (name == NULL) {
      return NULL;
   }
   device_write(name, strlen(dcon_hook_str), 0, (uint8_t*)dcon_hook_str);
   return name;
}
