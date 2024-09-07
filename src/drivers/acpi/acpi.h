#ifndef _ACPI_H
#define _ACPI_H

//https://wiki.osdev.org/FADT
#include <omen/libraries/std/stdint.h>

#define RSDP_SIGNATURE "RSD PTR "
#define RSDP2_SIGNATURE "RSDP PTR "

#define BERT_SIGNATURE "BERT"
#define BOOT_SIGNATURE "BOOT"
#define BGRT_SIGNATURE "BGRT"
#define CPEP_SIGNATURE "CPEP"
#define CSRT_SIGNATURE "CSRT"
#define DBG2_SIGNATURE "DBG2"
#define DBGP_SIGNATURE "DBGP"
#define DSDT_SIGNATURE "DSDT"
#define DMAR_SIGNATURE "DMAR"
#define DRTM_SIGNATURE "DRTM"
#define ECDT_SIGNATURE "ECDT"
#define EINJ_SIGNATURE "EINJ"
#define ERST_SIGNATURE "ERST"
#define ETDT_SIGNATURE "ETDT"
#define FACS_SIGNATURE "FACS"
#define FADT_SIGNATURE "FADT"
#define FPDT_SIGNATURE "FPDT"
#define GTDT_SIGNATURE "GTDT"
#define HEST_SIGNATURE "HEST"
#define HPET_SIGNATURE "HPET"
#define IBFT_SIGNATURE "IBFT"
#define IORT_SIGNATURE "IORT"
#define IVRS_SIGNATURE "IVRS"
#define LPIT_SIGNATURE "LPIT"
#define MADT_SIGNATURE "MADT"
#define MCFG_SIGNATURE "MCFG"
#define MCHI_SIGNATURE "MCHI"
#define MPST_SIGNATURE "MPST"
#define MSCT_SIGNATURE "MSCT"
#define MSDM_SIGNATURE "MSDM"
#define NFIT_SIGNATURE "NFIT"
#define OEMx_SIGNATURE "OEMx"
#define PCCT_SIGNATURE "PCCT"
#define PMTT_SIGNATURE "PMTT"
#define PSDT_SIGNATURE "PSDT"
#define RASF_SIGNATURE "RASF"
#define RSDT_SIGNATURE "RSDT"
#define SBST_SIGNATURE "SBST"
#define SLIC_SIGNATURE "SLIC"
#define SLIT_SIGNATURE "SLIT"
#define SPCR_SIGNATURE "SPCR"
#define SPMI_SIGNATURE "SPMI"
#define SRAT_SIGNATURE "SRAT"
#define SSDT_SIGNATURE "SSDT"
#define STAO_SIGNATURE "STAO"
#define TCPA_SIGNATURE "TCPA"
#define TPM2_SIGNATURE "TPM2"
#define UEFI_SIGNATURE "UEFI"
#define WAET_SIGNATURE "WAET"
#define WDAT_SIGNATURE "WDAT"
#define WDRT_SIGNATURE "WDRT"
#define WPBT_SIGNATURE "WPBT"
#define XENV_SIGNATURE "XENV"
#define XSDT_SIGNATURE "XSDT"

#define ADDRESS_SPACE_SYSTEM_MEMORY                     0
#define ADDRESS_SPACE_SYSTEM_IO                         1
#define ADDRESS_SPACE_PCI_CONFIGURATION_SPACE           2
#define ADDRESS_SPACE_EMBEDDED_CONTROLLER               3
#define ADDRESS_SPACE_SMBUS                             4
#define ADDRESS_SPACE_CMOS                              5
#define ADDRESS_SPACE_PCI_BAR_TARGET                    6
#define ADDRESS_SPACE_IPMI                              7
#define ADDRESS_SPACE_GENERAL_PURPOSE_IO                8
#define ADDRESS_SPACE_GENERIC_SERIAL_BUS                9
#define ADDRESS_SPACE_PLATFORM_COMMUNICATION_CHANNEL    10

#define ACCESS_SIZE_UNDEFINED                           0
#define ACCESS_SIZE_BYTE                                1
#define ACCESS_SIZE_WORD                                2
#define ACCESS_SIZE_DWORD                               3
#define ACCESS_SIZE_QWORD                               4

#define PM_PROFILE_UNSPECIFIED                          0
#define PM_PROFILE_DESKTOP                              1
#define PM_PROFILE_MOBILE                               2
#define PM_PROFILE_WORKSTATION                          3
#define PM_PROFILE_ENTERPRISE_SERVER                    4
#define PM_PROFILE_SOHO_SERVER                          5
#define PM_PROFILE_APPLIANCE_PC                         6
#define PM_PROFILE_PERFORMANCE_SERVER                   7

struct rsdp_descriptor {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__ ((packed));

struct rsdp2_descriptor {
    struct rsdp_descriptor first_part;

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

struct acpi_sdt_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__ ((packed));

struct rsdt {
    struct acpi_sdt_header header;
    uint32_t pointer_other_sdt[];
} __attribute__ ((packed)); 

struct xsdt {
  struct acpi_sdt_header header;
  uint64_t pointer_other_sdt[];
} __attribute__ ((packed));

struct mcfg_header {
    struct acpi_sdt_header header;
    uint64_t reserved;
} __attribute__ ((packed));

struct madt_header {
    struct acpi_sdt_header header;
    uint32_t local_apic_address;
    uint32_t flags; //1 for dual 8259 PIC, 0 for single
} __attribute__ ((packed));

struct generic_address_structure {
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
} __attribute__ ((packed));

struct fadt_header {
    struct acpi_sdt_header header;
    uint32_t firmware_control;
    uint32_t dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t reserved;

    uint8_t preferred_pm_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_request;
    uint8_t pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t cstate_control;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t boot_architecture_flags;

    uint8_t reserved2;
    uint32_t flags;

    // 12 byte structure; see below for details
    struct generic_address_structure reset_reg;

    uint8_t reset_value;
    uint8_t reserved3[3];

    // 64bit pointers - Available on ACPI 2.0+
    uint64_t x_firmware_control;
    uint64_t x_dsdt;

    struct generic_address_structure x_pm1a_event_block;
    struct generic_address_structure x_pm1b_event_block;
    struct generic_address_structure x_pm1a_control_block;
    struct generic_address_structure x_pm1b_control_block;
    struct generic_address_structure x_pm2_control_block;
    struct generic_address_structure x_pm_timer_block;
    struct generic_address_structure x_gpe0_block;
    struct generic_address_structure x_gpe1_block;
} __attribute__ ((packed));

struct mcfg_header* get_acpi_mcfg();
struct madt_header* get_acpi_madt();
struct fadt_header* get_acpi_fadt();
void init_acpi();
void enumerate_tables();
#endif