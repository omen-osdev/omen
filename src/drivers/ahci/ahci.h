#ifndef _AHCI_H
#define _AHCI_H
#include <omen/libraries/std/stdint.h>

#define SATA_SIG_ATAPI 0xEB140101
#define SATA_SIG_ATA   0x00000101
#define SATA_SIG_SEMB  0xC33C0101
#define SATA_SIG_PM    0x96690101

#define HBA_PxCMD_CR   0x8000
#define HBA_PxCMD_FRE  0x0010
#define HBA_PxCMD_FR   0x4000
#define HBA_PxCMD_ST   0x0001

#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA_EX 0x35
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_PACKET 0xA0

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
#define HBA_PxIS_TFES (1 << 30)

#define ATAPI_READ_CMD 0xA8

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200


enum port_type {
    PORT_TYPE_NONE = 0,
    PORT_TYPE_SATA = 1,
    PORT_TYPE_SEMB = 2,
    PORT_TYPE_PM = 3,
    PORT_TYPE_SATAPI = 4
};

enum fis_type {
    FIS_TYPE_REG_H2D = 0x27,
    FIS_TYPE_REG_D2H = 0x34,
    FIS_TYPE_DMA_ACT = 0x39,
    FIS_TYPE_DMA_SETUP = 0x41,
    FIS_TYPE_DATA = 0x46,
    FIS_TYPE_BIST = 0x58,
    FIS_TYPE_PIO_SETUP = 0x5F,
    FIS_TYPE_DEV_BITS = 0xA1
};

struct hba_port {
    uint32_t command_list_base;
    uint32_t command_list_base_upper;
    uint32_t fis_base_address;
    uint32_t fis_base_address_upper;
    uint32_t interrupt_status;
    uint32_t interrupt_enable;
    uint32_t command_status;
    uint32_t reserved;
    uint32_t task_file_data;
    uint32_t signature;
    uint32_t sata_status;
    uint32_t sata_control;
    uint32_t sata_error;
    uint32_t sata_active;
    uint32_t command_issue;
    uint32_t sata_notification;
    uint32_t fis_based_switching_control;
    uint32_t device_sleep;
    uint32_t reserved2[10];
    uint32_t vendor[4];
} __attribute__ ((packed));

struct hba_memory {
    uint32_t host_capabilities;
    uint32_t global_host_control;
    uint32_t interrupt_status;
    uint32_t ports_implemented;
    uint32_t version;
    uint32_t ccc_control;
    uint32_t ccc_ports;
    uint32_t enclosure_management_location;
    uint32_t enclosure_management_control;
    uint32_t host_capabilities_extended;
    uint32_t bios_handoff_control_status;
    uint8_t reserved[116];
    uint8_t vendor_specific[96];
    struct hba_port ports[1];
} __attribute__ ((packed));

struct ahci_port {
    volatile struct hba_port* hba_port;
    enum port_type port_type;
    uint8_t *buffer;
    uint8_t port_number;
};

struct hba_command_header {
    uint8_t command_fis_length : 5;
    uint8_t atapi : 1;
    uint8_t write : 1;
    uint8_t prefetchable : 1;

    uint8_t reset : 1;
    uint8_t bist : 1;
    uint8_t clear_busy_on_ok : 1;
    uint8_t reserved0 : 1;
    uint8_t port_multiplier : 4;

    uint16_t prdt_length;
    uint32_t prdb_count;
    uint32_t command_table_base_address;
    uint32_t command_table_base_address_upper;
    uint32_t reserved1[4];
} __attribute__ ((packed));

struct hba_command_fis {
    uint8_t fis_type;

    uint8_t pm_port : 4;
    uint8_t reserved0 : 3;
    uint8_t command_control : 1;

    uint8_t command;
    uint8_t feature_low;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device_register;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t feature_high;

    uint8_t count_low;
    uint8_t count_high;
    uint8_t icc;
    uint8_t control;

    uint8_t reserved1[4];
} __attribute__ ((packed));

struct hba_prdt_entry {
    uint32_t data_base_address;
    uint32_t data_base_address_upper;
    uint32_t reserved0;

    uint32_t byte_count : 22;
    uint32_t reserved1 : 9;
    uint32_t interrupt_on_completion : 1;
} __attribute__ ((packed));

struct hba_command_table {
    uint8_t command_fis[64];
    uint8_t atapi_command[16];
    uint8_t reserved[48];
    struct hba_prdt_entry prdt_entry[32];
} __attribute__ ((packed));

void init_ahci(uint32_t);

uint8_t identify(uint8_t);
uint8_t read_port(uint8_t, uint64_t, uint32_t);
uint8_t write_port(uint8_t, uint64_t, uint32_t);

uint8_t write_atapi_port(uint8_t, uint64_t, uint32_t);
uint8_t read_atapi_port(uint8_t, uint64_t, uint32_t);

uint8_t * get_buffer(uint8_t);
uint8_t get_port_count();
enum port_type get_port_type(uint8_t);
#endif