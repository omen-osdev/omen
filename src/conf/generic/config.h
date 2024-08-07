#ifndef _CONFIG_H
#define _CONFIG_H

#define ARCH x86_64

#define PAGE_SIZE 0x1000

#define PROCESS_STACK_SIZE 0x1000
#define PROCESS_SIGNAL_MAX 0x20

#define BOOTLOADER_USE_LIMINE

#define MAX_NUMA_NODES 16

//Per type of device/driver
#define MAX_DEVICES 256
#define MAX_DEVICE_DRIVERS 256
#define DEVICE_NAME_MAX_SIZE 256

//The debugger will work with a circular buffer
#define DEBUG_MESSAGE_BUFFER 0x1000
#endif