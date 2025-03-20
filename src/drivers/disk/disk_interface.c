//------------------------------------------------------------------------------
// MIT license
// Copyright (c) 2020 StrawberryHacker
//------------------------------------------------------------------------------

#include "disk_interface.h"
#include "disk.h"
#include <omen/managers/dev/devices.h>
#include <omen/apps/debug/debug.h>


uint8_t disk_get_status(const char * disk) {
	return (device_search(disk) != 0) ? STATUS_READY : STATUS_NOT_READY;
}

int get_disk_status(const char * drive) {
	return (device_search(drive) != 0);
}

uint8_t disk_initialize(const char * disk) {
	return (device_ioctl(disk, IOCTL_INIT, 0) != 0);
}

int init_disk(const char * drive) {
	return (device_ioctl(drive, IOCTL_INIT, 0) != 0);
}

uint8_t disk_read(const char * disk, uint8_t* buffer, uint32_t lba, uint32_t count) {
	return device_read(disk, count, lba, buffer);
}
int read_disk(const char* drive, void *buffer, int sector, int count) {
	return device_read(drive, count, sector, buffer);
}

uint8_t disk_write(const char * disk, uint8_t* buffer, uint32_t lba, uint32_t count) {
	return device_write(disk, count, lba, buffer);
}
int write_disk(const char * drive, void *buffer, int sector, int count) {
	return device_write(drive, count, sector, buffer);
}

uint64_t disk_ioctl (const char * device, uint32_t op, void* buffer) {
	return (device_ioctl(device, op, buffer) != 0);
}
int ioctl_disk(const char * drive, int request, void *buffer) {
	return (device_ioctl(drive, request, buffer) != 0);
}
uint64_t disk_identify(const char * device) {
	uint8_t found = device_identify(device, DISK_ATA_DD_NAME);
	if (found == 0) {
		found = device_identify(device, DISK_ATAPI_DD_NAME);
	}

	return found;
}