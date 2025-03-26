#include "gpt.h"
#include <disk/disk_interface.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/apps/debug/debug.h>

uint32_t read_gpt(const char* disk, struct vfs_partition* partitions, void (*add_part)(struct vfs_partition*, uint32_t, uint32_t, uint8_t, uint8_t)) {
	uint8_t mount_buffer[512];
	memset(mount_buffer, 0, 512);
	
	// Read MBR sector at LBA address zero
	if (!disk_read(disk, mount_buffer, 0, 1)) return 0;

	// Check the boot signature in the MBR
	if (load16(mount_buffer + MBR_BOOT_SIG) != MBR_BOOT_SIG_VALUE) {
		return 0;
	}

	struct gpt_header *gpt_header = (struct gpt_header *)kmalloc(sizeof(struct gpt_header));
	memset((uint8_t*)gpt_header, 0, sizeof(struct gpt_header));
	if (!disk_read(disk, (uint8_t*)gpt_header, 1, 1)) return 0;

	if (gpt_header->signature != GPT_SIGNATURE)
		return 0;
	kprintf("[GPT] Compatible disk found on %s [sig: %llx]\n", disk, gpt_header->signature);
	struct gpt_entry gpt_entries[gpt_header->partition_count];
	for (uint32_t i = 0; i < gpt_header->partition_count; i+=4) {
		uint32_t lba = gpt_header->partition_table_lba + ((i * gpt_header->partition_entry_size) >> 9);
		if (!disk_read(disk, (uint8_t*)(&gpt_entries[i]), lba, 1)) return 0;
	}

	uint32_t valid_partitions = 0;

	char buffer[36];
	for (uint32_t i = 0; i < gpt_header->partition_count; i++) {

		sprintf(buffer, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",\
			load32(&(gpt_entries[i].type_guid[0])),\
			load16(&(gpt_entries[i].type_guid[4])),\
			load16(&(gpt_entries[i].type_guid[6])),\
			gpt_entries[i].type_guid[8],\
			gpt_entries[i].type_guid[9],\
			gpt_entries[i].type_guid[10],\
			gpt_entries[i].type_guid[11],\
			gpt_entries[i].type_guid[12],\
			gpt_entries[i].type_guid[13],\
			gpt_entries[i].type_guid[14],\
			gpt_entries[i].type_guid[15]\
		);

		if (!memcmp(buffer, GPT_NO_ENTRY, strlen(GPT_NO_ENTRY))) {
			//We asume valid partitions are sequential!
			break;
		}

		if (!memcmp(buffer, GPT_EFI_ENTRY, strlen(GPT_EFI_ENTRY))) {
			kprintf("[GPT] EFI partition found at LBA 0x%llx\n", gpt_entries[i].first_lba);
		} else {
			kprintf("[GPT] Non EFI partition found at LBA 0x%llx\n", gpt_entries[i].first_lba);
		}
		valid_partitions++;
	}

	// Retrieve the partition info from all four partitions, thus avoiding 
	// multiple accesses to the MBR sector

	for (uint8_t i = 0; i < valid_partitions; i++) {
		add_part(partitions, gpt_entries[i].first_lba, (gpt_entries[i].last_lba - gpt_entries[i].first_lba), 0, 0);
		kprintf("[GPT] Partition %d: LBA %d, size %d\n", i, gpt_entries[i].first_lba, (gpt_entries[i].last_lba - gpt_entries[i].first_lba));
	}

	return valid_partitions;
}