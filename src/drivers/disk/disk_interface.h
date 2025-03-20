//------------------------------------------------------------------------------
// MIT license
// Copyright (c) 2020 StrawberryHacker
//------------------------------------------------------------------------------

#ifndef DISK_INTERFACE_H
#define DISK_INTERFACE_H

#define OP_SUCCESS                  0    
#define OP_FAILURE                  1

#define STATUS_NOT_READY            0
#define STATUS_READY                1
#define STATUS_BUSY                 2
#define STATUS_NON_PRESENT          3
#define STATUS_UNCONTROLLED_ERROR   4

#define IOCTL_ATAPI_IDENTIFY   0x1
#define IOCTL_GENERIC_STATUS   0x2
#define IOCTL_INIT             0x3
#define IOCTL_CTRL_SYNC        0x4
#define IOCTL_CTRL_TRIM        0x5
#define IOCTL_GET_SECTOR_SIZE  0x6
#define IOCTL_GET_SECTOR_COUNT 0x7
#define IOCTL_GET_BLOCK_SIZE   0x8

#include <omen/libraries/std/stdint.h>

/// Returns the status of the MSD (mass storage device)
uint8_t disk_get_status(const char * disk);
int get_disk_status(const char * drive);

/// Initializes at disk intrface
uint8_t disk_initialize(const char * disk);
int init_disk(const char * drive);

/// Read a number of sectors from the MSD
uint8_t disk_read(const char * disk, uint8_t* buffer, uint32_t lba, uint32_t count);
int read_disk(const char* drive, void *buffer, int sector, int count);

/// Write a number of sectors to the MSD
uint8_t disk_write(const char * disk, uint8_t* buffer, uint32_t lba, uint32_t count);
int write_disk(const char * drive, void *buffer, int sector, int count);

uint64_t disk_ioctl (const char * device, uint32_t op, void* buffer);
int ioctl_disk(const char * drive, int request, void *buffer);

uint64_t disk_identify(const char * device);
#endif