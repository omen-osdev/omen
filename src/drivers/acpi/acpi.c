#include <acpi/acpi.h>
#include <omen/managers/boot/bootloaders/bootloader.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/hal/arch/x86/io.h> //TODO: Abstract!

const char acpi_signatures[][4] = {
    BERT_SIGNATURE,
    BOOT_SIGNATURE,
    BGRT_SIGNATURE,
    CPEP_SIGNATURE,
    CSRT_SIGNATURE,
    DBG2_SIGNATURE,
    DBGP_SIGNATURE,
    DSDT_SIGNATURE,
    DMAR_SIGNATURE,
    DRTM_SIGNATURE,
    ECDT_SIGNATURE,
    EINJ_SIGNATURE,
    ERST_SIGNATURE,
    ETDT_SIGNATURE,
    FACS_SIGNATURE,
    FADT_SIGNATURE,
    FPDT_SIGNATURE,
    GTDT_SIGNATURE,
    HEST_SIGNATURE,
    HPET_SIGNATURE,
    IBFT_SIGNATURE,
    IORT_SIGNATURE,
    IVRS_SIGNATURE,
    LPIT_SIGNATURE,
    MADT_SIGNATURE,
    MCFG_SIGNATURE,
    MCHI_SIGNATURE,
    MPST_SIGNATURE,
    MSCT_SIGNATURE,
    MSDM_SIGNATURE,
    NFIT_SIGNATURE,
    OEMx_SIGNATURE,
    PCCT_SIGNATURE,
    PMTT_SIGNATURE,
    PSDT_SIGNATURE,
    RASF_SIGNATURE,
    SBST_SIGNATURE,
    SLIC_SIGNATURE,
    SLIT_SIGNATURE,
    SPCR_SIGNATURE,
    SPMI_SIGNATURE,
    SRAT_SIGNATURE,
    SSDT_SIGNATURE,
    STAO_SIGNATURE,
    TCPA_SIGNATURE,
    TPM2_SIGNATURE,
    UEFI_SIGNATURE,
    WAET_SIGNATURE,
    WDAT_SIGNATURE,
    WDRT_SIGNATURE,
    WPBT_SIGNATURE,
    XENV_SIGNATURE,
    XSDT_SIGNATURE
};
uint8_t acpi_signature_count = sizeof(acpi_signatures) / 4;

uint8_t acpi_sdt_checksum(struct acpi_sdt_header* table_header)
{
    uint8_t sum = 0;
 
    for (uint32_t i = 0; i < table_header->length; i++)
    {
        sum += ((uint8_t *) table_header)[i];
    }
 
    return (sum == 0);
}

struct acpi_sdt_header* find_rsdt(struct rsdt* rsdt, const char* signature, uint64_t sig_len) {
    uint32_t entries = (rsdt->header.length - sizeof(struct acpi_sdt_header)) / sizeof(uint32_t);
    kprintf("find_rsdt Navigating %d entries searching for %s\n", entries, signature);

    for (uint32_t i = 0; i < entries; i++) {
        uint32_t pto = rsdt->pointer_other_sdt[i];
        struct acpi_sdt_header* header = (struct acpi_sdt_header*)(uint64_t)pto; //TODO: delete this inmediately
        kprintf("Entry %d => %x HEADER AT: %p SIG: %.4s\n", i, pto, header, header->signature);

        if (!memcmp(header->signature, signature, sig_len))
            return header;
    }

    kprintf("Could not find ACPI table with signature %.4s\n", signature);
    return 0;
}

struct acpi_sdt_header* find_xsdt(struct xsdt* xsdt, const char* signature, uint64_t sig_len) {
    uint32_t entries = (xsdt->header.length - sizeof(struct acpi_sdt_header)) / sizeof(uint64_t);
    kprintf("find_xsdt Navigating %d entries searching for %s\n", entries, signature);

    for (uint32_t i = 0; i < entries; i++) {
        uint32_t pto = xsdt->pointer_other_sdt[i];
        struct acpi_sdt_header* header = (struct acpi_sdt_header*)(uint64_t)pto; //TODO: delete this inmediately
        kprintf("Entry %d => %x HEADER AT: %p SIG: %.4s\n", i, pto, header, header->signature);

        if (!memcmp(header->signature, signature, sig_len))
            return header;
    }

    kprintf("Could not find ACPI table with signature %.4s\n", signature);
    return 0;
}

void* init_acpi_vt(void* rsdp_address, const char* target) {
    char signature[9];
    char oemid[7];

    //kprintf("RSDP revision 2\n");

    struct rsdp2_descriptor* rsdp = (struct rsdp2_descriptor*)rsdp_address;

    if (memcmp(rsdp->first_part.signature, "RSD PTR ", 8))
        panic("RSDP signature mismatch");

    strncpy(oemid, rsdp->first_part.oem_id, 6);
    strncpy(signature, rsdp->first_part.signature, 8);

    /*   
    kprintf("RSDPv2 at: 0x%llx Sig: %.4s Check: %d OEM: %s Rev: %d RsdtAddr: 0x%llx Len: 0x%llx XsdtAddr: 0x%llx ExtCheck: %d Res: %s\n",
        rsdp, signature, rsdp->first_part.checksum, oemid, rsdp->first_part.revision, rsdp->first_part.rsdt_address, rsdp->length,
        rsdp->xsdt_address, rsdp->extended_checksum, rsdp->reserved
    );
    */
    struct xsdt* xsdt = (struct xsdt*)(uint64_t)(rsdp->xsdt_address);
    struct acpi_sdt_header* result = find_xsdt(xsdt, target, 4);
    if (result == 0) {
        return 0;
    }

    if (!acpi_sdt_checksum(result))
        panic("XSDT checksum mismatch");

    return (void*)result;
}

void* init_acpi_vz(void* rsdp_address, const char * target) {
    char signature[9];
    char oemid[7];

    //kprintf("RSDP revision 0\n");

    struct rsdp_descriptor* rsdp = (struct rsdp_descriptor*)rsdp_address;

    if (memcmp(rsdp->signature, "RSD PTR ", 8))
        panic("RSDP signature mismatch");

    strncpy(oemid, rsdp->oem_id, 6);
    strncpy(signature, rsdp->signature, 8);

    /*   
    kprintf("RSDP at: 0x%llx Sig: %.4s Check: %d OEM: %s Rev: %d RsdtAddr: 0x%llx\n",
        rsdp, signature, rsdp->checksum, oemid, rsdp->revision, rsdp->rsdt_address
    );
    */

    struct rsdt* rsdt = (struct rsdt*)(uint64_t)(rsdp->rsdt_address);
    struct acpi_sdt_header* result = find_rsdt(rsdt, target, 4);

    if (!acpi_sdt_checksum(result))
        panic("RSDT checksum mismatch");

    return (void*)result;
}

struct mcfg_header* get_acpi_mcfg() {
    char signature[9];
    char oemid[7];
    char oemtableid[9];

    void* rsdp_address = (void*)get_rsdp_address();
    struct rsdp_descriptor* prev_rsdp = (struct rsdp_descriptor*)rsdp_address;
    struct mcfg_header* mcfg_header = 0x0;

    if (prev_rsdp->revision == 0) {
        mcfg_header = (struct mcfg_header*)init_acpi_vz(rsdp_address, "MCFG");
    } else if (prev_rsdp->revision == 2) {
        mcfg_header = (struct mcfg_header*)init_acpi_vt(rsdp_address, "MCFG");
    } else {
        panic("RSDP revision not supported");
    }   

    strncpy(signature, mcfg_header->header.signature, 4);
    strncpy(oemtableid, mcfg_header->header.oem_table_id, 8);
    strncpy(oemid, mcfg_header->header.oem_id, 6);

    kprintf("MCFG at: 0x%llx Sig: %.4s Len: 0x%llx Rev: %d Check: %d OEM: %s OTI: %s OR: %d CID: %d CREV: %d\n",
        mcfg_header, signature, mcfg_header->header.length, mcfg_header->header.revision, mcfg_header->header.checksum, oemid, oemtableid,
        mcfg_header->header.oem_revision, mcfg_header->header.creator_id, mcfg_header->header.creator_revision
    );

    return mcfg_header;
}

struct madt_header* get_acpi_madt() {
    char signature[9];
    char oemid[7];
    char oemtableid[9];

    void* rsdp_address = (void*)get_rsdp_address();
    struct rsdp_descriptor* prev_rsdp = (struct rsdp_descriptor*)rsdp_address;
    struct madt_header* madt_header = 0x0;

    if (prev_rsdp->revision == 0) {
        madt_header = (struct madt_header*)init_acpi_vz(rsdp_address, "APIC");
    } else if (prev_rsdp->revision == 2) {
        madt_header = (struct madt_header*)init_acpi_vt(rsdp_address, "APIC");
    } else {
        panic("RSDP revision not supported");
    }   

    strncpy(signature, madt_header->header.signature, 4);
    strncpy(oemtableid, madt_header->header.oem_table_id, 8);
    strncpy(oemid, madt_header->header.oem_id, 6);

    kprintf("MADT at: 0x%llx Sig: %.4s Len: 0x%llx Rev: %d Check: %d OEM: %s OTI: %s OR: %d CID: %d CREV: %d\n",
        madt_header, signature, madt_header->header.length, madt_header->header.revision, madt_header->header.checksum, oemid, oemtableid,
        madt_header->header.oem_revision, madt_header->header.creator_id, madt_header->header.creator_revision
    );

    return madt_header;
}

struct fadt_header* get_acpi_fadt() {
    char signature[9];
    char oemid[7];
    char oemtableid[9];

    void* rsdp_address = (void*)get_rsdp_address();
    struct rsdp_descriptor* prev_rsdp = (struct rsdp_descriptor*)rsdp_address;
    struct fadt_header* fadt_header = 0x0;

    if (prev_rsdp->revision == 0) {
        fadt_header = (struct fadt_header*)init_acpi_vz(rsdp_address, "FACP");
    } else if (prev_rsdp->revision == 2) {
        fadt_header = (struct fadt_header*)init_acpi_vt(rsdp_address, "FACP");
    } else {
        panic("RSDP revision not supported");
    }   

    strncpy(signature, fadt_header->header.signature, 4);
    strncpy(oemtableid, fadt_header->header.oem_table_id, 8);
    strncpy(oemid, fadt_header->header.oem_id, 6);

    kprintf("FADT at: 0x%llx Sig: %.4s Len: 0x%llx Rev: %d Check: %d OEM: %s OTI: %s OR: %d CID: %d CREV: %d\n",
        fadt_header, signature, fadt_header->header.length, fadt_header->header.revision, fadt_header->header.checksum, oemid, oemtableid,
        fadt_header->header.oem_revision, fadt_header->header.creator_id, fadt_header->header.creator_revision
    );

    return fadt_header;
}

void enumerate_tables() {
    void* rsdp_address = (void*)get_rsdp_address();
    struct rsdp_descriptor* prev_rsdp = (struct rsdp_descriptor*)rsdp_address;
    struct acpi_sdt_header * table;
    if (prev_rsdp->revision == 0) {
        for (uint32_t i = 0; i < acpi_signature_count; i++) {
            table  = init_acpi_vz(rsdp_address, acpi_signatures[i]);
            if (table) {
                kprintf("Table %d vz => %s\n", i, table->signature);
            }
        }
    } else if (prev_rsdp->revision == 2) {
        for (uint32_t i = 0; i < acpi_signature_count; i++) {
            table = init_acpi_vt(rsdp_address, acpi_signatures[i]);
            if (table) {
                kprintf("Table %d vt => %s\n", i, table->signature);
            }
        }
    } else {
        panic("RSDP revision not supported");
    }
}

uint8_t is_enabled(struct fadt_header* fadt_header) {
    //Check if ACPI is enabled
    if (fadt_header->smi_command_port != 0) return 0;
    if (fadt_header->acpi_enable != 0 || fadt_header->acpi_disable != 0) return 0;
    if (!(fadt_header->pm1a_event_block & 1)) return 0;

    return 1;
}

void init_acpi() {
    struct fadt_header* fadt_header = get_acpi_fadt();
    if (is_enabled(fadt_header)) {kprintf("ACPI is enabled\n"); return;}
    kprintf("ACPI is not enabled, trying to enable...\n");
    if (inw(fadt_header->pm1a_control_block) & 1) {kprintf("ACPI is enabled\n"); return;}
    outb(fadt_header->smi_command_port, fadt_header->acpi_enable);
    kprintf("Waiting 3 seconds for ACPI to enable...\n");
    while (((inw(fadt_header->pm1a_control_block) & 1) == 0));
}