#ifndef _PCI_H
#define _PCI_H
#include <acpi/acpi.h>

#define PCI_INTERRUPT_START 0x64

#define PCI_BAR_TYPE_NULL           0x0
#define PCI_BAR_TYPE_IO             0x1
#define PCI_BAR_TYPE_32             0x2
#define PCI_BAR_TYPE_64             0x3

#define PCI_CONFIGURATION_SPACE_SIZE 0x100
#define PCI_CONFIG_ADDR            0xCF8
#define PCI_CONFIG_DATA            0xCFC

//Unknown
//Class
#define PCI_CLASS_UNKNOWN       0x0

//Storage
//Class
#define PCI_CLASS_STORAGE       0x1
//Subclass
#define PCI_STORAGE_SCSI_BUS    0x0
#define PCI_STORAGE_IDE         0x1
#define PCI_STORAGE_FLOPPY      0x2
#define PCI_STORAGE_IPI         0x3
#define PCI_STORAGE_RAID        0x4
#define PCI_STORAGE_ATA         0x5
#define PCI_STORAGE_SATA        0x6
#define PCI_STORAGE_SAS         0x7
#define PCI_STORAGE_NVME        0x8
#define PCI_STORAGE_OTHER       0x80

//Network
//Class
#define PCI_CLASS_NETWORK       0x2
//Subclass
#define PCI_NETWORK_ETHERNET    0x0
#define PCI_NETWORK_TOKEN_RING  0x1
#define PCI_NETWORK_FDDI        0x2
#define PCI_NETWORK_ATM         0x3
#define PCI_NETWORK_ISDN        0x4
#define PCI_NETWORK_WORLDFIP    0x5
#define PCI_NETWORK_PICMG       0x6
#define PCI_NETWORK_INFINIBAND  0x7
#define PCI_NETWORK_FABRIC      0x8
#define PCI_NETWORK_OTHER       0x80

//Display
//Class
#define PCI_CLASS_DISPLAY       0x3
//Subclass
#define PCI_DISPLAY_VGA         0x0
#define PCI_DISPLAY_XGA         0x1
#define PCI_DISPLAY_3D          0x2
#define PCI_DISPLAY_OTHER       0x80

//Multimedia
//Class
#define PCI_CLASS_MULTIMEDIA        0x4
//Subclass
#define PCI_MULTIMEDIA_VIDEO        0x0
#define PCI_MULTIMEDIA_AUDIO        0x1
#define PCI_MULTIMEDIA_PHONE        0x2
#define PCI_MULTIMEDIA_AUDIO_DEV    0x3
#define PCI_MULTIMEDIA_OTHER        0x80

//Memory
//Class
#define PCI_CLASS_MEMORY        0x5
//Subclass
#define PCI_MEMORY_RAM          0x0
#define PCI_MEMORY_FLASH        0x1
#define PCI_MEMORY_OTHER        0x80

//Bridge
//Class
#define PCI_CLASS_BRIDGE        0x6
//Subclass
#define PCI_BRIDGE_HOST         0x0
#define PCI_BRIDGE_ISA          0x1
#define PCI_BRIDGE_EISA         0x2
#define PCI_BRIDGE_MCA          0x3
#define PCI_BRIDGE_PCI          0x4
#define PCI_BRIDGE_PCMCIA       0x5
#define PCI_BRIDGE_NUBUS        0x6
#define PCI_BRIDGE_CARDBUS      0x7
#define PCI_BRIDGE_RACEWAY      0x8
#define PCI_BRIDGE_SEMI_PCI     0x9
#define PCI_BRIDGE_INFINIBAND   0xA
#define PCI_BRIDGE_OTHER        0x80

//Simple Communication Controller
//Class
#define PCI_CLASS_COMMUNICATION         0x7
//Subclass
#define PCI_COMMUNICATION_SERIAL        0x0
#define PCI_COMMUNICATION_PARALLEL      0x1
#define PCI_COMMUNICATION_MULTISERIAL   0x2
#define PCI_COMMUNICATION_MODEM         0x3
#define PCI_COMMUNICATION_GPIB          0x4
#define PCI_COMMUNICATION_SMARTCARD     0x5
#define PCI_COMMUNICATION_OTHER         0x80

//Base System Peripheral
//Class
#define PCI_CLASS_SYSTEM      0x8
//Subclass
#define PCI_SYSTEM_PIC        0x0
#define PCI_SYSTEM_DMA        0x1
#define PCI_SYSTEM_TIMER      0x2
#define PCI_SYSTEM_RTC        0x3
#define PCI_SYSTEM_HOTPLUG    0x4
#define PCI_SYSTEM_SD_HOST    0x5
#define PCI_SYSTEM_IOMMU      0x6
#define PCI_SYSTEM_OTHER      0x80

//Input Device
//Class
#define PCI_CLASS_INPUT       0x9
//Subclass
#define PCI_INPUT_KEYBOARD    0x0
#define PCI_INPUT_DIGITIZER   0x1
#define PCI_INPUT_MOUSE       0x2
#define PCI_INPUT_SCANNER     0x3
#define PCI_INPUT_GAMEPORT    0x4
#define PCI_INPUT_OTHER       0x80

//Docking Station
//Class
#define PCI_CLASS_DOCKING     0xA
//Subclass
#define PCI_DOCKING_GENERIC   0x0
#define PCI_DOCKING_OTHER     0x80

//Processor
//Class
#define PCI_CLASS_PROCESSOR   0xB
//Subclass
#define PCI_PROCESSOR_386     0x0
#define PCI_PROCESSOR_486     0x1
#define PCI_PROCESSOR_PENTIUM 0x2
#define PCI_PROCESSOR_ALPHA   0x10
#define PCI_PROCESSOR_POWERPC 0x20
#define PCI_PROCESSOR_MIPS    0x30
#define PCI_PROCESSOR_COPROC  0x40
#define PCI_PROCESSOR_OTHER   0x80

//Serial Bus Controller
//Class
#define PCI_CLASS_SERIALBUS   0xC
//Subclass
#define PCI_SERIAL_FIREWIRE   0x0
#define PCI_SERIAL_ACCESS     0x1
#define PCI_SERIAL_SSA        0x2
#define PCI_SERIAL_USB        0x3
#define PCI_SERIAL_FIBRE      0x4
#define PCI_SERIAL_SMBUS      0x5
#define PCI_SERIAL_INFINIBAND 0x6
#define PCI_SERIAL_IPMI       0x7
#define PCI_SERIAL_SERCOS     0x8
#define PCI_SERIAL_CANBUS     0x9
#define PCI_SERIAL_OTHER      0x80

//Wireless Controller
//Class
#define PCI_CLASS_WIRELESS     0xD
//Subclass
#define PCI_WIRELESS_IRDA      0x0
#define PCI_WIRELESS_IR        0x1
#define PCI_WIRELESS_RF        0x10
#define PCI_WIRELESS_BLUETOOTH 0x11
#define PCI_WIRELESS_BROADBAND 0x12
#define PCI_WIRELESS_80211A    0x20
#define PCI_WIRELESS_80211B    0x21
#define PCI_WIRELESS_OTHER     0x80

//Intelligent Controller
//Class
#define PCI_CLASS_INTELLIGENT  0xE
//Subclass
#define PCI_INTELLIGENT_I2O    0x0

//Satellite Communication Controller
//Class
#define PCI_CLASS_SATELLITE    0xF
//Subclass
#define PCI_SATELLITE_TV       0x1
#define PCI_SATELLITE_AUDIO    0x2
#define PCI_SATELLITE_VOICE    0x3
#define PCI_SATELLITE_DATA     0x4

//Encryption Controller
//Class
#define PCI_CLASS_ENCRYPTION     0x10
//Subclass
#define PCI_CRYPTO_NETWORK       0x0
#define PCI_CRYPTO_ENTERTAINMENT 0x10
#define PCI_CRYPTO_OTHER         0x80

//Signal Processing Controller
//Class
#define PCI_CLASS_SIGNAL        0x11
//Subclass
#define PCI_SIGNAL_DPIO         0x0
#define PCI_SIGNAL_PERF_COUNTER 0x1
#define PCI_SIGNAL_COMM_SYNC    0x10
#define PCI_SIGNAL_MGMT_CARD    0x20
#define PCI_SIGNAL_OTHER        0x80

//Processing Accelerator
//Class
#define PCI_CLASS_ACCELERATOR   0x12

//Non-Essential Instrumentation
//Class
#define PCI_CLASS_INSTRUMENT    0x13

//Coprocessor
//Class
#define PCI_CLASS_COPROCESSOR   0x40

//Unassigned Class
//Class
#define PCI_CLASS_UNASSIGNED    0xFF

struct pci_bar_io {
    uint32_t base_address : 30;
    uint32_t reserved : 1;
    uint32_t always_one : 1;
} __attribute__ ((packed));

struct pci_bar_mem {
    uint32_t base_address : 28;
    uint32_t prefetchable : 1;
    uint32_t type : 2;
    uint32_t reserved : 1;
} __attribute__ ((packed));

//Prog if

struct pci_device_header {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
} __attribute__ ((packed));

struct pci_device_header_0 {
    struct pci_device_header header;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
    uint32_t cardbus_cis_pointer;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_address;
    uint8_t capabilities_pointer;
    uint8_t reserved0;
    uint16_t reserved1;
    uint32_t reserved2;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
} __attribute__ ((packed));

struct device_config {
    uint64_t base_address;
    uint16_t pci_sec_group;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
} __attribute__ ((packed));

void pci_set_irq(struct pci_device_header_0* pci, uint8_t irq);
uint64_t get_bar_size(void* addresslow, uint32_t base_address);
void* get_bar_address(struct pci_device_header_0 * devh, uint8_t index);
uint8_t get_bar_type(uint32_t value);
void enable_bus_mastering(struct pci_device_header* pci_device_header);
const char* get_vendor_name(uint16_t);
const char* get_device_name(uint16_t, uint16_t);
const char* get_device_class(uint8_t);
const char* get_subclass_name(uint8_t, uint8_t);
const char* get_prog_interface(uint8_t, uint8_t, uint8_t);
void trigger_pci_interrupt();
void subscribe_pci_interrupt(const char* id, struct pci_device_header * dev, void (*cb)(void*), void * data);
void init_pci();
#endif