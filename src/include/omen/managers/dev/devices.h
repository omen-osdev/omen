#ifndef _DEVICES_H
#define _DEVICES_H

#include <omen/libraries/std/stddef.h>
#include <omen/libraries/std/stdint.h>

#define DEVICE_BLOCK 0
#define DEVICE_CHAR 1

struct file_operations {
    uint64_t (*read) (uint64_t, uint64_t, uint64_t, uint8_t*);
    uint64_t (*write) (uint64_t, uint64_t, uint64_t, uint8_t*);
    uint64_t (*ioctl) (uint64_t, uint32_t, void*);
};

struct network_operations {
    uint64_t (*send)(void *, uint8_t *, uint16_t);
    uint64_t (*recv)(void *, uint8_t *, uint16_t );
    uint64_t (*ioctl)(void *, uint32_t, void*);
};

typedef struct device_driver {
    struct file_operations *fops;
    struct network_operations *nops;
    uint8_t registered;
    char name[DEVICE_NAME_MAX_SIZE];
} driver_t;

typedef struct device {
    uint8_t bc: 1; // 0 = block, 1 = char
    uint8_t valid: 1;
    uint8_t major;
    uint8_t minor;
    char name[DEVICE_NAME_MAX_SIZE];
    uint64_t internal_id;
    void * device_control_structure;
} device_t;

//Device driver operations
status_t register_char(const uint8_t major, const char* name, const struct file_operations* fops);
status_t register_block(const uint8_t major, const char* name, const struct file_operations* fops);
status_t register_network(const uint8_t major, const char * name, const struct network_operations* nops);
status_t unregister_char(const uint8_t major);
status_t unregister_block(const uint8_t major);
status_t unregister_network(const uint8_t major);

//Device management operations
char * device_create(void * device_control_structure, const uint8_t major, const uint64_t id);
status_t device_destroy(const char * name);
status_t device_list(const uint8_t mode);

//Device retrieval operations
uint32_t get_device_count();
uint32_t get_device_count_by_major(const uint8_t major);
device_t* device_search(const char* name);

//Operations on devices
status_t device_write(const char * name, const uint64_t size,const uint64_t offset, uint8_t * buffer);
status_t device_read(const char * name, const uint64_t size, const uint64_t offset, uint8_t * buffer);
status_t device_ioctl(const char * name, const uint64_t request, void* data);
status_t device_identify(const char* device, const char* driver);

//Generic functions
void init_devices();
#endif