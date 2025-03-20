#include "mbr.h"
#include <disk/disk_interface.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/apps/debug/debug.h>

uint32_t read_mbr(const char* disk, struct vfs_partition* partitions, void (*add_part)(struct vfs_partition*, uint32_t, uint32_t, uint8_t, uint8_t)) {
    uint8_t mount_buffer[512];
	memset(mount_buffer, 0, 512);
	
	// Read MBR sector at LBA address zero
	if (!disk_read(disk, mount_buffer, 0, 1)) return 0;
    
	// Check the boot signature in the MBR
	if (load16(mount_buffer + MBR_BOOT_SIG) != MBR_BOOT_SIG_VALUE) {
		return 0;
	}
    
    struct mbr_header *mbr_header = (struct mbr_header *)mount_buffer;
    printf("[MBR] compatible disk found on %s [sig: %llx, boot sig: %lx\n", disk, mbr_header->disk_signature, mbr_header->signature);

    uint32_t valid_partitions = 0;

    for (int i = 0; i < 4; i++) {

        //For some reason, fdisk initialices attributes as 0, so we have to check 
        //Validity by sector size. I'm guessing a zero size partition must be bogus.
        if (mbr_header->partitions[i].number_of_sectors != 0x0) {
            add_part(partitions, mbr_header->partitions[i].lba_partition_start, mbr_header->partitions[i].number_of_sectors, 1, mbr_header->partitions[i].partition_type);
		    printf("[MBR] Partition %d: LBA %d, size %d\n", i, mbr_header->partitions[i].lba_partition_start, mbr_header->partitions[i].number_of_sectors);

            valid_partitions++;
        }
    }

    return valid_partitions;
}