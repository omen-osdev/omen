#include <generic/config.h>
#include <generic/devices.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/std/stddef.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/error.h>
#include <omen/managers/dev/devices.h>

struct device_driver char_device_drivers[MAX_DEVICES] = {0};
uint32_t char_device_count = 0;
struct device_driver block_device_drivers[MAX_DEVICES] = {0};
uint32_t block_device_count = 0;
struct device_driver net_device_drivers[MAX_DEVICES] = {0};
uint32_t net_device_count = 0;

struct device devices[MAX_DEVICES] = {0};
uint32_t device_count = 0;

status_t register_char(const uint8_t major, const char* name, struct file_operations* fops) {
    if (major < CHAR_MIN_MAJOR) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Major number too small");
    }

    if (strlen(name) > DEVICE_NAME_MAX_SIZE) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Name too long");
    }

    if (fops == NULL) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "File operations not provided");
    }

    if (char_device_drivers[major].registered) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Major number already registered");
    }

    uint8_t major_id = major & 0x7f;
    strncpy(char_device_drivers[major_id].name, name, strlen(name));
    char_device_drivers[major_id].fops = fops;
    char_device_drivers[major_id].registered = true;
    char_device_drivers[major_id].nops = NULL;

    kprintf("Registered char device %s with major number %d\n", name, major_id);

    return SUCCESS;
}

status_t register_block(const uint8_t major, const char* name, struct file_operations* fops) {

    if (strlen(name) > DEVICE_NAME_MAX_SIZE) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Name too long");
    }

    if (fops == NULL) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "File operations not provided");
    }

    if (block_device_drivers[major].registered) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Major number already registered");
    }

    strncpy(block_device_drivers[major].name, name, strlen(name));
    block_device_drivers[major].fops = fops;
    block_device_drivers[major].registered = true;
    block_device_drivers[major].nops = NULL;

    kprintf("Registered block device %s with major number %d\n", name, major);

    return SUCCESS;
}

status_t register_network(const uint8_t major, const char * name, struct network_operations* nops) {
    if (strlen(name) > DEVICE_NAME_MAX_SIZE) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Name too long");
    }

    if (nops == NULL) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Network operations not provided");
    }

    if (net_device_drivers[major].registered) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Major number already registered");
    }

    strncpy(net_device_drivers[major].name, name, strlen(name));
    net_device_drivers[major].nops = nops;
    net_device_drivers[major].registered = true;
    net_device_drivers[major].fops = NULL;

    kprintf("Registered network device %s with major number %d\n", name, major);

    return SUCCESS;
}

status_t unregister_char(const uint8_t major) {
    if (major < CHAR_MIN_MAJOR) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Major number too small");
    }

    uint8_t major_id = major & 0x7f;
    if (!char_device_drivers[major_id].registered) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Major number not registered");
    }

    char_device_drivers[major_id].registered = false;

    kprintf("Unregistered char device with major number %d\n", major_id);

    return SUCCESS;
}

status_t unregister_block(const uint8_t major) {

    if (!block_device_drivers[major].registered) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Major number not registered");
    }

    block_device_drivers[major].registered = false;

    kprintf("Unregistered block device with major number %d\n", major);

    return SUCCESS;
}

status_t unregister_network(const uint8_t major) {

    if (!net_device_drivers[major].registered) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Major number not registered");
    }

    net_device_drivers[major].registered = false;

    kprintf("Unregistered network device with major number %d\n", major);

    return SUCCESS;
}

//Device management operations
char * device_create(void * device_control_structure, const uint8_t major, const uint64_t id) {

    uint16_t minor = 0;
    int32_t index = -1;
    if (DEVICE_IDENTIFIERS[major] == NULL) {
        return NULL;
    }

    for (uint32_t i = 0; i < MAX_DEVICES; i++) {
        if (devices[i].valid) {
            if (devices[i].major == major && devices[i].minor >= minor) {
                minor = devices[i].minor + 1;
            }
        } else {
            if (index < 0) index = i;
        }
    }

    if (index < 0) {
        return NULL;
    }

    char name[DEVICE_NAME_MAX_SIZE];
    snprintf(name, DEVICE_NAME_MAX_SIZE, "%s%x", DEVICE_IDENTIFIERS[major], minor+0xA);

    devices[index].major = major & 0x7f;
    devices[index].bc = ((major & 0x80) >> 7);
    devices[index].minor = minor;
    devices[index].internal_id = id;
    devices[index].device_control_structure = device_control_structure;
    strncpy(devices[index].name, name, strlen(name));
    devices[index].valid = 1;

    device_count++;

    return devices[index].name;
}

status_t device_destroy(const char * name) {
    panic("device_destroy not implemented");
}
status_t device_list() {
    for (uint32_t i = 0; i < device_count; i++) {
        kprintf("Device %s with major number %d and minor number %d\n", devices[i].name, devices[i].major, devices[i].minor);
    }
}

uint32_t get_device_count() {
    return device_count;
}

uint32_t get_device_count_by_major(const uint8_t major) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < device_count; i++) {
        if (devices[i].major == major) count++;
    }
    return count;
}

device_t* device_search(const char* name) {
    for (uint32_t i = 0; i < device_count; i++) {
        if (strcmp(devices[i].name, name) == 0) return &devices[i];
    }
    return NULL;
}

driver_t* driver_get(const uint8_t major) {
    if (major < CHAR_MIN_MAJOR) {
        return &char_device_drivers[major];
    } else {
        return &block_device_drivers[major];
    }
}

//Operations on devices
status_t device_write(const char * name, const uint64_t size,const uint64_t offset, uint8_t * buffer) {
    device_t * device = device_search(name);
    if (device == NULL || !device->valid) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Device not found for name %s", name);
    }

    driver_t * driver = driver_get(device->major);
    if (driver == NULL || !driver->registered) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Driver not found for major number %d", device->major);
    }
    
    if (driver->fops == NULL || driver->fops->write == NULL) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Write operation not supported for major number %d", device->major);
    }

    return driver->fops->write(device->internal_id, size, offset, buffer);
}

status_t device_read(const char * name, const uint64_t size, const uint64_t offset, uint8_t * buffer) {
    device_t * device = device_search(name);
    if (device == NULL || !device->valid) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Device not found for name %s", name);
    }

    driver_t * driver = driver_get(device->major);
    if (driver == NULL || !driver->registered) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Driver not found for major number %d", device->major);
    }
    
    if (driver->fops == NULL || driver->fops->read == NULL) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Read operation not supported for major number %d", device->major);
    }

    return driver->fops->read(device->internal_id, size, offset, buffer);
}

status_t device_ioctl(const char * name, const uint64_t request, void* data) {
    device_t * device = device_search(name);
    if (device == NULL || !device->valid) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Device not found for name %s", name);
    }

    driver_t * driver = driver_get(device->major);
    if (driver == NULL || !driver->registered) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Driver not found for major number %d", device->major);
    }
    
    if (driver->fops == NULL || driver->fops->ioctl == NULL) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "IOCTL operation not supported for major number %d", device->major);
    }

    return driver->fops->ioctl(device->internal_id, request, data);
}

status_t device_identify(const char* device_name, const char* driver_name) {
    device_t * device = device_search(device_name);
    if (device == NULL || !device->valid) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Device not found for name %s", device_name);
    }

    driver_t * driver = driver_get(device->major);
    if (driver == NULL || !driver->registered) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Driver not found for major number %d", device->major);
    }

    if (strcmp(driver->name, driver_name) != 0) {
        RECOVERABLE_ERROR(INVALID_ARGUMENT, "Driver name %s does not match driver name %s", driver_name, driver->name);
    }

    return SUCCESS;
}

//Generic functions
void init_devices() {
    for (uint32_t i = 0; i < MAX_DEVICES; i++) {
        devices[i].valid = 0;
        char_device_drivers[i].registered = 0;
        block_device_drivers[i].registered = 0;
        net_device_drivers[i].registered = 0;
    }

    device_count = 0;
    char_device_count = 0;
    block_device_count = 0;

    kprintf("Initialized devices\n");
    
}
