#include "pci.h"
#include <ahci/ahci.h>
#include <acpi/acpi.h>
#include <omen/managers/dev/devices.h>
#include <omen/libraries/std/stdint.h>
#include <omen/hal/arch/x86/io.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/std/string.h>
#include <omen/managers/mem/vmm.h>
#include <omen/libraries/allocators/heap_allocator.h>

//Ignore -waddress-of-packed-member
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

struct pci_device_header* global_device_header = {0};

struct pci_dev_list {
    char name[32];
    void * data;
    struct pci_device_header* device;
    void (*cb)(void*);
    struct pci_dev_list* next;
};

struct pci_dev_list* pci_dev_list_head = {0};

const char* pci_device_classes[] = {    
    "Unclassified",
    "Mass Storage Controller",
    "Network Controller",
    "Display Controller",
    "Multimedia Controller",
    "Memory Controller",
    "Bridge Device",
    "Simple Communication Controller",
    "Base System Peripheral",
    "Input Device Controller",
    "Docking Station", 
    "Processor",
    "Serial Bus Controller",
    "Wireless Controller",
    "Intelligent Controller",
    "Satellite Communication Controller",
    "Encryption Controller",
    "Signal Processing Controller",
    "Processing Accelerator",
    "Non Essential Instrumentation"
};

uint8_t has_interrupted(struct pci_device_header_0* device) {
    uint16_t status = ((struct pci_device_header*)device)->status;
    uint16_t command = ((struct pci_device_header*)device)->command;

    //If bit 3 of the status register == 1 and the bit 10 of the command register == 0 return 1
    return ((status & 0b1000) && !(command & 0b10000000000));
}

void PCIWrite32(uint32_t addr, uint32_t data) {
    addr &= ~(0b11);
    /* Write address */
    outl(PCI_CONFIG_ADDR, addr);
    /* Write data */
    outl(PCI_CONFIG_DATA, data);
}

uint32_t PCIRead32(uint32_t addr) {
    addr &= ~(0b11);
    /* Write address */
    outl(PCI_CONFIG_ADDR, addr);
    /* Read data */
    return inl(PCI_CONFIG_DATA);
}

uint16_t PCIRead16(uint32_t addr) {
    uint8_t offset = addr & 0xff;
    addr &= ~(0b11);
    return (uint16_t) ((PCIRead32(addr) >> ((offset & 0b10) * 0x10)) & 0xffff);
}

uint8_t PCIRead8(uint32_t addr) {
    uint8_t offset = addr & 0xff;
    addr &= ~(0b11);
    return (uint16_t) ((PCIRead16(addr) >> ((offset & 0b1) * 0x8)) & 0xff);
}

void PCIMemcpy8(void* dst, uint32_t src, uint64_t size){
    src &= ~(0b11);
    for(uint64_t i = 0; i < size; i += 0x1){
        *(uint8_t*)((uint64_t)dst + i) = PCIRead8(src + i);
    }
}

void PCIMemcpy16(void* dst, uint32_t src, uint64_t size){
    src &= ~(0b11);
    for(uint64_t i = 0; i < size; i += 0x2){
        *(uint16_t*)((uint64_t)dst + i) = PCIRead16(src + i);
    }
}

void PCIMemcpy32(void* dst, uint32_t src, uint64_t size){
    src &= ~(0b11);
    for(uint64_t i = 0; i < size; i += 0x4){
        *(uint32_t*)((uint64_t)dst + i) = PCIRead32(src + i);
    }
}


const char* get_vendor_name(uint16_t vendor_id) {
    switch (vendor_id) {
        case 0x8086:
            return "Intel";
        case 0x10de:
            return "nVidia";
        case 0x1022:
            return "AMD";
        case 0x1234:
            return "Bochs";
    }
    return itoa(vendor_id, 16);
}

const char* get_device_class(uint8_t class) {
    return pci_device_classes[class];
}


const char* get_device_name(uint16_t vendor_id, uint16_t device_id) {
    switch (vendor_id) {
        case 0x8086: {
            switch (device_id) {
                case 0x29c0:
                    return "Express DRAM Controller";
                case 0x2918:
                    return "LPC Interface Controller";
                case 0x2922:
                    return "SATA Controller [AHCI]";
                case 0x2930:
                    return "SMBus Controller";
                case 0x10d3:
                    return "Gigabit Network Connection";
            }
            break;
        }
        case 0x1234: {
            switch (device_id) {
                case 0x1111:
                    return "Virtual VGA Controller";
            }
            break;
        }
    }
    
    return itoa(device_id, 16);
}

const char * mass_storage_controller_subclass_name(uint8_t subclass_code) {
    switch (subclass_code) {
        case 0x00:
            return "SCSI Bus Controller";
        case 0x01:
            return "IDE Controller";
        case 0x02:
            return "Floppy Disk Controller";
        case 0x03:
            return "IPI Bus Controller";
        case 0x04:
            return "RAID Controller";
        case 0x05:
            return "ATA Controller";
        case 0x06:
            return "Serial ATA";
        case 0x07:
            return "Serial Attached SCSI";
        case 0x08:
            return "Non-Volatile Memory Controller";
        case 0x80:
            return "Other";
    }

    return itoa(subclass_code, 16);
}

const char* serial_bus_controller_subclass_name(uint8_t subclass_code){
    switch (subclass_code){
        case 0x00:
            return "FireWire (IEEE 1394) Controller";
        case 0x01:
            return "ACCESS Bus";
        case 0x02:
            return "SSA";
        case 0x03:
            return "USB Controller";
        case 0x04:
            return "Fibre Channel";
        case 0x05:
            return "SMBus";
        case 0x06:
            return "Infiniband";
        case 0x07:
            return "IPMI Interface";
        case 0x08:
            return "SERCOS Interface (IEC 61491)";
        case 0x09:
            return "CANbus";
        case 0x80:
            return "SerialBusController - Other";
    }
    return itoa(subclass_code, 16);
}

const char* bridge_device_subclass_name(uint8_t subclass_code){
    switch (subclass_code){
        case 0x00:
            return "Host Bridge";
        case 0x01:
            return "ISA Bridge";
        case 0x02:
            return "EISA Bridge";
        case 0x03:
            return "MCA Bridge";
        case 0x04:
            return "PCI-to-PCI Bridge";
        case 0x05:
            return "PCMCIA Bridge";
        case 0x06:
            return "NuBus Bridge";
        case 0x07:
            return "CardBus Bridge";
        case 0x08:
            return "RACEway Bridge";
        case 0x09:
            return "PCI-to-PCI Bridge";
        case 0x0a:
            return "InfiniBand-to-PCI Host Bridge";
        case 0x80:
            return "Other";
    }

    return itoa(subclass_code, 16);
}

const char * get_subclass_name(uint8_t class_code, uint8_t subclass_code) {
    switch (class_code) {
        case 0x01:
            return mass_storage_controller_subclass_name(subclass_code);
        case 0x03: {
            switch (subclass_code) {
                case 0x00:
                    return "VGA Compatible Controller";
            }
            break;
        }
        case 0x06:
            return bridge_device_subclass_name(subclass_code);
        case 0x0c:
            return serial_bus_controller_subclass_name(subclass_code);
    }
    return itoa(subclass_code, 16);
}

const char* get_prog_interface(uint8_t class_code, uint8_t subclass_code, uint8_t prog_interface){
    switch (class_code){
        case 0x01: {
            switch (subclass_code){
                case 0x06: {
                    switch (prog_interface){
                        case 0x00:
                            return "Vendor Specific Interface";
                        case 0x01:
                            return "AHCI 1.0";
                        case 0x02:
                            return "Serial Storage Bus";
                    }
                    break;
                }
            }
            break;
        }
        case 0x03: {
            switch (subclass_code) {
                case 0x00: {
                    switch (prog_interface){
                        case 0x00:
                            return "VGA Controller";
                        case 0x01:
                            return "8514-Compatible Controller";
                    }
                    break;
                }
            }
            break;
        }
        case 0x0C: {
            switch (subclass_code){
                case 0x03: {
                    switch (prog_interface){
                        case 0x00:
                            return "UHCI Controller";
                        case 0x10:
                            return "OHCI Controller";
                        case 0x20:
                            return "EHCI (USB2) Controller";
                        case 0x30:
                            return "XHCI (USB3) Controller";
                        case 0x80:
                            return "Unspecified";
                        case 0xFE:
                            return "USB Device (Not a Host Controller)";
                    }
                    break;
                }
            }    
            break;
        }
    }
    return itoa(prog_interface, 16);
}

void trigger_pci_interrupt() {
    //Iterate pci_dev_list_head and check if any of the devices have an interrupt.
    //If so call cb
    struct pci_dev_list* current = pci_dev_list_head;
    while (current != 0) {
        struct pci_device_header* device = current->device;
        if (current->device != 0 && ((device->status & 0b1000) && !(device->command & 0b10000000000))) {
            if (current->cb != 0) current->cb(current->data);
        }
        if (current->next == 0) break;
        current = current->next;
    }
}

void subscribe_pci_interrupt(const char* id, struct pci_device_header * dev, void (*cb)(void*), void * data) {
    struct pci_dev_list* new_dev_list = kmalloc(sizeof(struct pci_dev_list));
    new_dev_list->device = dev;
    new_dev_list->cb = cb;
    new_dev_list->data = data;
    new_dev_list->next = pci_dev_list_head;
    strncpy(new_dev_list->name, id, strlen(id));
    pci_dev_list_head = new_dev_list;
}

void register_pci_device(struct pci_device_header* pci, uint32_t device_base_address) {
    printf("[PCI] Device detected: %x, %x, %x\n", pci->class_code, pci->subclass, pci->prog_if);
    switch (pci->class_code){
        case 0x01: {
            switch (pci->subclass){
                case 0x06: {
                    switch (pci->prog_if){
                        case 0x01:
                            init_ahci(((struct pci_device_header_0*)pci)->bar5);//TODO: Careful. This is a dynamic driver startup.
                            for (int i = 0; i < get_port_count(); i++) {
                                switch (get_port_type(i)) {
                                    case PORT_TYPE_SATA:
                                        device_create((void*)pci, 0x8, i);
                                        break;
                                    case PORT_TYPE_SATAPI:
                                        device_create((void*)pci, 0x9, i);
                                        break;
                                    case PORT_TYPE_SEMB:
                                        device_create((void*)pci, 0xa, i);
                                        break;
                                    case PORT_TYPE_PM:
                                        device_create((void*)pci, 0xb, i);
                                        break;
                                    case PORT_TYPE_NONE:
                                        device_create((void*)pci, 0xc, i);
                                        break;
                                }
                            }
                            break;
                        default:
                            printf("[PCI] Unknown AHCI interface: %x\n", pci->prog_if);
                    }
                    break;
                }
            }
            break;
        }
        case 0x2: {
            switch (pci->subclass) {
                case 0x0: {
                    switch (pci->prog_if) {
                        case 0x0: {
                            //struct e1000 * nic = e1000_init((struct pci_device_header_0*)pci, device_base_address);
                            //cb((void*)nic, 0x11, 0);
                        }
                    }
                }
            }
        }
    }
}

void pci_set_irq(struct pci_device_header_0* pci, uint8_t irq) {
    pci->interrupt_line = irq;
}

void enumerate_function(uint64_t device_address, uint64_t bus, uint64_t device, uint64_t function) {

    uint64_t offset = function << 12;

    struct pci_device_header* pci_device_header = (struct pci_device_header*)to_identity_map(device_address + offset);
    global_device_header = pci_device_header;

    //if (pci_device_header->device_id == 0x0) return;
    if (pci_device_header->device_id == 0xFFFF) return;

    printf("[PCI] %s %s: %s %s / %s\n",
        get_device_class(pci_device_header->class_code),
        get_subclass_name(pci_device_header->class_code, pci_device_header->subclass),
        get_vendor_name(pci_device_header->vendor_id),
        get_device_name(pci_device_header->vendor_id, pci_device_header->device_id),
        get_prog_interface(pci_device_header->class_code, pci_device_header->subclass, pci_device_header->prog_if)
    );

    uint32_t device_base_address = ((1 << 31) | (bus << 16) | (device << 11) | (function << 8));
    printf("[PCI] Device base address: %x\n", device_base_address);
    register_pci_device(pci_device_header, device_base_address);
}

void enumerate_device(uint64_t bus_address, uint64_t device, uint64_t bus) {
    uint64_t offset = device << 15;

    uint64_t device_address = (uint64_t)to_identity_map(bus_address + offset);
    struct pci_device_header* pci_device_header = (struct pci_device_header*)device_address;
    //if (pci_device_header->device_id == 0x0) return;
    //if (pci_device_header->device_id == 0xFFFF) return;

    //printf("[PCI] enumerating device %x:%x\n", bus_address, device_address);

    if (pci_device_header->header_type & 0x80) {
        for (uint64_t function = 0; function < 8; function++) {
            enumerate_function(bus_address + offset, bus, device, function);
        }
    } else {
        enumerate_function(bus_address + offset, bus, device, 0);
    }
}

void enumerate_bus(uint64_t base_address, uint64_t bus) {
    uint64_t offset = bus << 20;

    //uint64_t bus_address = (uint64_t)to_identity_map(base_address + offset);
    //struct pci_device_header* pci_device_header = (struct pci_device_header*)bus_address;

    //if (pci_device_header->device_id == 0x0) {printf("ig0:%ld|", bus); return;}
    //if (pci_device_header->device_id == 0xFFFF) {printf("igf:%ld|", bus); return;}

    //printf("\n[PCI] Enumerating bus %x\n", bus);
    for (uint64_t device = 0; device < 32; device++) {
        enumerate_device(base_address + offset, device, bus);
    }
}

//Shoutout to kot!
//https://github.com/kot-org/Kot/blob/f14dabe3771fa5d633ab5523c888f3bf428f89b8/Sources/Modules/Drivers/bus/pci/Src/core/main.cpp#L152

void* get_bar_address(struct pci_device_header_0 * devh, uint8_t index) {
    if (index > 5) return 0x0;
    uint32_t *bar_pointer = &(devh->bar0);
    uint32_t bar_value = bar_pointer[index];
    uint32_t bar_value2 = bar_pointer[index + 1];
    switch (get_bar_type(bar_value)){
        case PCI_BAR_TYPE_IO:
            return (void*)(uint64_t)(bar_value & 0xFFFFFFFC);
        case PCI_BAR_TYPE_32:
            return (void*)(uint64_t)(bar_value & 0xFFFFFFF0);
        case PCI_BAR_TYPE_64: {
            if (index == 5) return 0x0;
            return (void*)(uint64_t)((bar_value & 0xFFFFFFF0) | ((uint64_t)(bar_value2 & 0xFFFFFFFF) << 32));
        }
        default:
            break;
    }

    return 0x0;
}

uint8_t get_bar_type(uint32_t value){
    if(value & 0b1){ /* i/o */
        return PCI_BAR_TYPE_IO;
    }else{
        if(!(value & 0b110)){ /* 32bits */
            return PCI_BAR_TYPE_32;
        }else if((value & 0b110) == 0b110){ /* 64bits */
            return PCI_BAR_TYPE_64;
        }
    }
    return PCI_BAR_TYPE_NULL;
}

void ReceiveConfigurationSpacePCI(uint32_t address, void * buffer){
    PCIMemcpy32(buffer, address, PCI_CONFIGURATION_SPACE_SIZE);
}

void SendConfigurationSpacePCI(uint32_t address, void * buffer){
    uint64_t ubuffer = (uint64_t)buffer;

    ubuffer &= ~(0b11);

    for(uint64_t i = 0; i < PCI_CONFIGURATION_SPACE_SIZE; i += 4) {
        PCIWrite32(address, *(uint32_t*)ubuffer);
        ubuffer += 4;
        address += 4;
    }
}

uint64_t get_bar_size(void* addresslow, uint32_t base_address){ //Address low is bar0.
    uint32_t BARValueLow = *(uint32_t*)addresslow;
    uint8_t Type = get_bar_type(BARValueLow);

    uint8_t * confspace = kmalloc(PCI_CONFIGURATION_SPACE_SIZE);
    ReceiveConfigurationSpacePCI(base_address, confspace);

    if(Type != PCI_BAR_TYPE_NULL){
        /* Write into bar low */
        *(uint32_t*)addresslow = 0xFFFFFFFF;
        SendConfigurationSpacePCI(base_address, confspace);

        /* Read bar low */
        ReceiveConfigurationSpacePCI(base_address, confspace);
        uint32_t SizeLow = *(uint32_t*)addresslow;

        if(Type == PCI_BAR_TYPE_IO){
            SizeLow &= 0xFFFFFFFC;
        }else{
            SizeLow &= 0xFFFFFFF0;
        }


        uint32_t SizeHigh = 0xFFFFFFFF;

        if(Type == PCI_BAR_TYPE_64){
            void* addresshigh = (void*)((uint64_t)addresslow + 0x4);

            uint32_t BARValueHigh = *(uint32_t*)addresshigh;
            /* Write into bar high */
            *(uint32_t*)addresshigh = 0xFFFFFFFF;
            SendConfigurationSpacePCI(base_address, confspace);

            /* Read bar high */
            ReceiveConfigurationSpacePCI(base_address, confspace);
            SizeHigh = *(uint32_t*)addresshigh;

            /* Restore value */
            *(uint32_t*)addresshigh = BARValueHigh;   
            SendConfigurationSpacePCI(base_address, confspace);
        }

        /* Restore value */
        *(uint32_t*)addresslow = BARValueLow;
        SendConfigurationSpacePCI(base_address, confspace);

        uint64_t Size = ((uint64_t)(SizeHigh & 0xFFFFFFFF) << 32) | (SizeLow & 0xFFFFFFFF);
        Size = ~Size + 1;

        kfree(confspace);
        return Size;
    }

    return 0;
}

void enable_bus_mastering(struct pci_device_header* pci_device_header) {
    uint16_t command = pci_device_header->command;
    command |= 0x4;
    pci_device_header->command = command;
}

void init_pci() {
    struct mcfg_header * mcfg = (struct mcfg_header*)get_acpi_mcfg();
    uint64_t entries = ((mcfg->header.length) - sizeof(struct mcfg_header)) / sizeof(struct device_config);

    printf("[PCI] Registering pci devices... %ld entries\n", entries);
    for (uint64_t i = 0; i < entries; i++) {
        struct device_config * new_device_config = (struct device_config*)((uint64_t)mcfg + sizeof(struct mcfg_header) + (sizeof(struct device_config) * i));	
        printf("[PCI] Base address as registering: %x\n", new_device_config->base_address);
        printf("[PCI] Enumerating buses %x:%x\n", new_device_config->start_bus, new_device_config->end_bus);
        for (uint64_t bus = new_device_config->start_bus; bus < new_device_config->end_bus; bus++) {
            enumerate_bus(new_device_config->base_address, bus);
        }
    }

    //unmask_interrupt(PCIA_IRQ);
}