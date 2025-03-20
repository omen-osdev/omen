#ifndef _DISK_DRIVER_H
#define _DISK_DRIVER_H
#include <omen/libraries/std/stdint.h>

#define IOCTL_ATAPI_IDENTIFY   0x1
#define IOCTL_GENERIC_STATUS   0x2
#define IOCTL_INIT             0x3
#define IOCTL_CTRL_SYNC        0x4
#define IOCTL_CTRL_TRIM        0x5
#define IOCTL_GET_SECTOR_SIZE  0x6
#define IOCTL_GET_SECTOR_COUNT 0x7
#define IOCTL_GET_BLOCK_SIZE   0x8

#define DISK_ATA_DD_NAME    "ATA DRIVER\0"
#define DISK_ATAPI_DD_NAME  "ATAPI DRIVER\0"

//https://forum.osdev.org/viewtopic.php?f=1&t=30118
struct sata_ident_test
{
    uint16_t   config;      /* lots of obsolete bit flags */
    uint16_t   cyls;      /* obsolete */
    uint16_t   reserved2;   /* special config */
    uint16_t   heads;      /* "physical" heads */
    uint16_t   track_bytes;   /* unformatted bytes per track */
    uint16_t   sector_bytes;   /* unformatted bytes per sector */
    uint16_t   sectors;   /* "physical" sectors per track */
    uint16_t   vendor0;   /* vendor unique */
    uint16_t   vendor1;   /* vendor unique */
    uint16_t   vendor2;   /* vendor unique */
    uint8_t   serial_no[20];   /* 0 = not_specified */
    uint16_t   buf_type;
    uint16_t   buf_size;   /* 512 byte increments; 0 = not_specified */
    uint16_t   ecc_bytes;   /* for r/w long cmds; 0 = not_specified */
    uint8_t   fw_rev[8];   /* 0 = not_specified */
    uint8_t   model[40];   /* 0 = not_specified */
    uint16_t  multi_count; /* Multiple Count */
    uint16_t   dword_io;   /* 0=not_implemented; 1=implemented */
    uint16_t   capability1;   /* vendor unique */
    uint16_t   capability2;   /* bits 0:DMA 1:LBA 2:IORDYsw 3:IORDYsup word: 50 */
    uint8_t   vendor5;   /* vendor unique */
    uint8_t   tPIO;      /* 0=slow, 1=medium, 2=fast */
    uint8_t   vendor6;   /* vendor unique */
    uint8_t   tDMA;      /* 0=slow, 1=medium, 2=fast */
    uint16_t   field_valid;   /* bits 0:cur_ok 1:eide_ok */
    uint16_t   cur_cyls;   /* logical cylinders */
    uint16_t   cur_heads;   /* logical heads word 55*/
    uint16_t   cur_sectors;   /* logical sectors per track */
    uint16_t   cur_capacity0;   /* logical total sectors on drive */
    uint16_t   cur_capacity1;   /*  (2 words, misaligned int)     */
    uint8_t   multsect;   /* current multiple sector count */
    uint8_t   multsect_valid;   /* when (bit0==1) multsect is ok */
    uint32_t   lba_capacity;   /* total number of sectors */
    uint16_t   dma_1word;   /* single-word dma info */
    uint16_t   dma_mword;   /* multiple-word dma info */
    uint16_t  eide_pio_modes; /* bits 0:mode3 1:mode4 */
    uint16_t  eide_dma_min;   /* min mword dma cycle time (ns) */
    uint16_t  eide_dma_time;   /* recommended mword dma cycle time (ns) */
    uint16_t  eide_pio;       /* min cycle time (ns), no IORDY  */
    uint16_t  eide_pio_iordy; /* min cycle time (ns), with IORDY */
    uint16_t   words69_70[2];   /* reserved words 69-70 */
    uint16_t   words71_74[4];   /* reserved words 71-74 */
    uint16_t  queue_depth;   /*  */
    uint16_t  sata_capability;   /*  SATA Capabilities word 76*/
    uint16_t  sata_additional;   /*  Additional Capabilities */
    uint16_t  sata_supported;   /* SATA Features supported  */
    uint16_t  features_enabled;   /* SATA features enabled */
    uint16_t  major_rev_num;   /*  Major rev number word 80 */
    uint16_t  minor_rev_num;   /*  */
    uint16_t  command_set_1;   /* bits 0:Smart 1:Security 2:Removable 3:PM */
    uint16_t  command_set_2;   /* bits 14:Smart Enabled 13:0 zero */
    uint16_t  cfsse;      /* command set-feature supported extensions */
    uint16_t  cfs_enable_1;   /* command set-feature enabled */
    uint16_t  cfs_enable_2;   /* command set-feature enabled */
    uint16_t  csf_default;   /* command set-feature default */
    uint16_t  dma_ultra;   /*  */
    uint16_t   word89;      /* reserved (word 89) */
    uint16_t   word90;      /* reserved (word 90) */
    uint16_t   CurAPMvalues;   /* current APM values */
    uint16_t   word92;         /* reserved (word 92) */
    uint16_t   comreset;      /* should be cleared to 0 */
    uint16_t  accoustic;      /*  accoustic management */
    uint16_t  min_req_sz;      /* Stream minimum required size */
    uint16_t  transfer_time_dma;   /* Streaming Transfer Time-DMA */
    uint16_t  access_latency;      /* Streaming access latency-DMA & PIO WORD 97*/
    uint32_t    perf_granularity;   /* Streaming performance granularity */
    uint32_t   total_usr_sectors[2]; /* Total number of user addressable sectors */
    uint16_t   transfer_time_pio;    /* Streaming Transfer time PIO */
    uint16_t   reserved105;       /* Word 105 */
    uint16_t   sector_sz;          /* Puysical Sector size / Logical sector size */
    uint16_t   inter_seek_delay;   /* In microseconds */
    uint16_t   words108_116[9];    /*  */
    uint32_t   words_per_sector;    /* words per logical sectors */
    uint16_t   supported_settings; /* continued from words 82-84 */
    uint16_t   command_set_3;       /* continued from words 85-87 */
    uint16_t  words121_126[6];   /* reserved words 121-126 */
    uint16_t   word127;         /* reserved (word 127) */
    uint16_t   security_status;   /* device lock function
                 * 15:9   reserved
                 * 8   security level 1:max 0:high
                 * 7:6   reserved
                 * 5   enhanced erase
                 * 4   expire
                 * 3   frozen
                 * 2   locked
                 * 1   en/disabled
                 * 0   capability
                 */
    uint16_t  csfo;      /* current set features options
                 * 15:4   reserved
                 * 3   auto reassign
                 * 2   reverting
                 * 1   read-look-ahead
                 * 0   write cache
                 */
    uint16_t   words130_155[26];/* reserved vendor words 130-155 */
    uint16_t   word156;
    uint16_t   words157_159[3];/* reserved vendor words 157-159 */
    uint16_t   cfa; /* CFA Power mode 1 */
    uint16_t   words161_175[15]; /* Reserved */
    uint8_t   media_serial[60]; /* words 176-205 Current Media serial number */
    uint16_t   sct_cmd_transport; /* SCT Command Transport */
    uint16_t   words207_208[2]; /* reserved */
    uint16_t   block_align; /* Alignement of logical blocks in larger physical blocks */
    uint32_t   WRV_sec_count; /* Write-Read-Verify sector count mode 3 only */
    uint32_t      verf_sec_count; /* Verify Sector count mode 2 only */
    uint16_t   nv_cache_capability; /* NV Cache capabilities */
    uint16_t   nv_cache_sz; /* NV Cache size in logical blocks */
    uint16_t   nv_cache_sz2; /* NV Cache size in logical blocks */
    uint16_t   rotation_rate; /* Nominal media rotation rate */
    uint16_t   reserved218; /*  */
    uint16_t   nv_cache_options; /* NV Cache options */
    uint16_t   words220_221[2]; /* reserved */
    uint16_t   transport_major_rev; /*  */
    uint16_t   transport_minor_rev; /*  */
    uint16_t   words224_233[10]; /* Reserved */
    uint16_t   min_dwnload_blocks; /* Minimum number of 512byte units per DOWNLOAD MICROCODE
                                command for mode 03h */
    uint16_t   max_dwnload_blocks; /* Maximum number of 512byte units per DOWNLOAD MICROCODE
                                command for mode 03h */
    uint16_t   words236_254[19];   /* Reserved */
    uint16_t   integrity;          /* Cheksum, Signature */
} __attribute__((packed));

//https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ata/ns-ata-_identify_device_data
struct sata_ident {
    uint16_t GeneralConfiguration_Reserved1 : 1;
    uint16_t GeneralConfiguration_Retired3 : 1;
    uint16_t GeneralConfiguration_ResponseIncomplete : 1;
    uint16_t GeneralConfiguration_Retired2 : 3;
    uint16_t GeneralConfiguration_FixedDevice : 1;
    uint16_t GeneralConfiguration_RemovableMedia : 1;
    uint16_t GeneralConfiguration_Retired1 : 7;
    uint16_t GeneralConfiguration_DeviceType : 1;
    uint16_t NumCylinders;
    uint16_t SpecificConfiguration;
    uint16_t NumHeads;
    uint16_t Retired1[2];
    uint16_t NumSectorsPerTrack;
    uint16_t VendorUnique1[3];
    uint8_t  SerialNumber[20];
    uint16_t Retired2[2];
    uint16_t Obsolete1;
    uint8_t  FirmwareRevision[8];
    uint8_t  ModelNumber[40];
    uint8_t  MaximumBlockTransfer;
    uint8_t  VendorUnique2;
    uint16_t TrustedComputing_FeatureSupported : 1;
    uint16_t TrustedComputing_Reserved : 15;
    uint8_t  Capabilities_CurrentLongPhysicalSectorAlignment : 2;
    uint8_t  Capabilities_ReservedByte49 : 6;
    uint8_t  Capabilities_DmaSupported : 1;
    uint8_t  Capabilities_LbaSupported : 1;
    uint8_t  Capabilities_IordyDisable : 1;
    uint8_t  Capabilities_IordySupported : 1;
    uint8_t  Capabilities_Reserved1 : 1;
    uint8_t  Capabilities_StandybyTimerSupport : 1;
    uint8_t  Capabilities_Reserved2 : 2;
    uint16_t Capabilities_ReservedWord50;
    uint16_t ObsoleteWords51[2];
    uint16_t TranslationFieldsValid : 3;
    uint16_t Reserved3 : 5;
    uint16_t FreeFallControlSensitivity : 8;
    uint16_t NumberOfCurrentCylinders;
    uint16_t NumberOfCurrentHeads;
    uint16_t CurrentSectorsPerTrack;
    uint64_t  CurrentSectorCapacity;
    uint8_t  CurrentMultiSectorSetting;
    uint8_t  MultiSectorSettingValid : 1;
    uint8_t  ReservedByte59 : 3;
    uint8_t  SanitizeFeatureSupported : 1;
    uint8_t  CryptoScrambleExtCommandSupported : 1;
    uint8_t  OverwriteExtCommandSupported : 1;
    uint8_t  BlockEraseExtCommandSupported : 1;
    uint64_t  UserAddressableSectors;
    uint16_t ObsoleteWord62;
    uint16_t MultiWordDMASupport : 8;
    uint16_t MultiWordDMAActive : 8;
    uint16_t AdvancedPIOModes : 8;
    uint16_t ReservedByte64 : 8;
    uint16_t MinimumMWXferCycleTime;
    uint16_t RecommendedMWXferCycleTime;
    uint16_t MinimumPIOCycleTime;
    uint16_t MinimumPIOCycleTimeIORDY;
    uint16_t AdditionalSupported_ZonedCapabilities : 2;
    uint16_t AdditionalSupported_NonVolatileWriteCache : 1;
    uint16_t AdditionalSupported_ExtendedUserAddressableSectorsSupported : 1;
    uint16_t AdditionalSupported_DeviceEncryptsAllUserData : 1;
    uint16_t AdditionalSupported_ReadZeroAfterTrimSupported : 1;
    uint16_t AdditionalSupported_Optional28BitCommandsSupported : 1;
    uint16_t AdditionalSupported_IEEE1667 : 1;
    uint16_t AdditionalSupported_DownloadMicrocodeDmaSupported : 1;
    uint16_t AdditionalSupported_SetMaxSetPasswordUnlockDmaSupported : 1;
    uint16_t AdditionalSupported_WriteBufferDmaSupported : 1;
    uint16_t AdditionalSupported_ReadBufferDmaSupported : 1;
    uint16_t AdditionalSupported_DeviceConfigIdentifySetDmaSupported : 1;
    uint16_t AdditionalSupported_LPSAERCSupported : 1;
    uint16_t AdditionalSupported_DeterministicReadAfterTrimSupported : 1;
    uint16_t AdditionalSupported_CFastSpecSupported : 1;
    uint16_t ReservedWords70[5];
    uint16_t QueueDepth : 5;
    uint16_t ReservedWord75 : 11;
    uint16_t SerialAtaCapabilities_Reserved0 : 1;
    uint16_t SerialAtaCapabilities_SataGen1 : 1;
    uint16_t SerialAtaCapabilities_SataGen2 : 1;
    uint16_t SerialAtaCapabilities_SataGen3 : 1;
    uint16_t SerialAtaCapabilities_Reserved1 : 4;
    uint16_t SerialAtaCapabilities_NCQ : 1;
    uint16_t SerialAtaCapabilities_HIPM : 1;
    uint16_t SerialAtaCapabilities_PhyEvents : 1;
    uint16_t SerialAtaCapabilities_NcqUnload : 1;
    uint16_t SerialAtaCapabilities_NcqPriority : 1;
    uint16_t SerialAtaCapabilities_HostAutoPS : 1;
    uint16_t SerialAtaCapabilities_DeviceAutoPS : 1;
    uint16_t SerialAtaCapabilities_ReadLogDMA : 1;
    uint16_t SerialAtaCapabilities_Reserved2 : 1;
    uint16_t SerialAtaCapabilities_CurrentSpeed : 3;
    uint16_t SerialAtaCapabilities_NcqStreaming : 1;
    uint16_t SerialAtaCapabilities_NcqQueueMgmt : 1;
    uint16_t SerialAtaCapabilities_NcqReceiveSend : 1;
    uint16_t SerialAtaCapabilities_DEVSLPtoReducedPwrState : 1;
    uint16_t SerialAtaCapabilities_Reserved3 : 8;
    uint16_t SerialAtaFeaturesSupported_Reserved0 : 1;
    uint16_t SerialAtaFeaturesSupported_NonZeroOffsets : 1;
    uint16_t SerialAtaFeaturesSupported_DmaSetupAutoActivate : 1;
    uint16_t SerialAtaFeaturesSupported_DIPM : 1;
    uint16_t SerialAtaFeaturesSupported_InOrderData : 1;
    uint16_t SerialAtaFeaturesSupported_HardwareFeatureControl : 1;
    uint16_t SerialAtaFeaturesSupported_SoftwareSettingsPreservation : 1;
    uint16_t SerialAtaFeaturesSupported_NCQAutosense : 1;
    uint16_t SerialAtaFeaturesSupported_DEVSLP : 1;
    uint16_t SerialAtaFeaturesSupported_HybridInformation : 1;
    uint16_t SerialAtaFeaturesSupported_Reserved1 : 6;
    uint16_t SerialAtaFeaturesEnabled_Reserved0 : 1;
    uint16_t SerialAtaFeaturesEnabled_NonZeroOffsets : 1;
    uint16_t SerialAtaFeaturesEnabled_DmaSetupAutoActivate : 1;
    uint16_t SerialAtaFeaturesEnabled_DIPM : 1;
    uint16_t SerialAtaFeaturesEnabled_InOrderData : 1;
    uint16_t SerialAtaFeaturesEnabled_HardwareFeatureControl : 1;
    uint16_t SerialAtaFeaturesEnabled_SoftwareSettingsPreservation : 1;
    uint16_t SerialAtaFeaturesEnabled_DeviceAutoPS : 1;
    uint16_t SerialAtaFeaturesEnabled_DEVSLP : 1;
    uint16_t SerialAtaFeaturesEnabled_HybridInformation : 1;
    uint16_t SerialAtaFeaturesEnabled_Reserved1 : 6;
    uint16_t MajorRevision;
    uint16_t MinorRevision;
    uint16_t CommandSetSupport_SmartCommands : 1;
    uint16_t CommandSetSupport_SecurityMode : 1;
    uint16_t CommandSetSupport_RemovableMediaFeature : 1;
    uint16_t CommandSetSupport_PowerManagement : 1;
    uint16_t CommandSetSupport_Reserved1 : 1;
    uint16_t CommandSetSupport_WriteCache : 1;
    uint16_t CommandSetSupport_LookAhead : 1;
    uint16_t CommandSetSupport_ReleaseInterrupt : 1;
    uint16_t CommandSetSupport_ServiceInterrupt : 1;
    uint16_t CommandSetSupport_DeviceReset : 1;
    uint16_t CommandSetSupport_HostProtectedArea : 1;
    uint16_t CommandSetSupport_Obsolete1 : 1;
    uint16_t CommandSetSupport_WriteBuffer : 1;
    uint16_t CommandSetSupport_ReadBuffer : 1;
    uint16_t CommandSetSupport_Nop : 1;
    uint16_t CommandSetSupport_Obsolete2 : 1;
    uint16_t CommandSetSupport_DownloadMicrocode : 1;
    uint16_t CommandSetSupport_DmaQueued : 1;
    uint16_t CommandSetSupport_Cfa : 1;
    uint16_t CommandSetSupport_AdvancedPm : 1;
    uint16_t CommandSetSupport_Msn : 1;
    uint16_t CommandSetSupport_PowerUpInStandby : 1;
    uint16_t CommandSetSupport_ManualPowerUp : 1;
    uint16_t CommandSetSupport_Reserved2 : 1;
    uint16_t CommandSetSupport_SetMax : 1;
    uint16_t CommandSetSupport_Acoustics : 1;
    uint16_t CommandSetSupport_BigLba : 1;
    uint16_t CommandSetSupport_DeviceConfigOverlay : 1;
    uint16_t CommandSetSupport_FlushCache : 1;
    uint16_t CommandSetSupport_FlushCacheExt : 1;
    uint16_t CommandSetSupport_WordValid83 : 2;
    uint16_t CommandSetSupport_SmartErrorLog : 1;
    uint16_t CommandSetSupport_SmartSelfTest : 1;
    uint16_t CommandSetSupport_MediaSerialNumber : 1;
    uint16_t CommandSetSupport_MediaCardPassThrough : 1;
    uint16_t CommandSetSupport_StreamingFeature : 1;
    uint16_t CommandSetSupport_GpLogging : 1;
    uint16_t CommandSetSupport_WriteFua : 1;
    uint16_t CommandSetSupport_WriteQueuedFua : 1;
    uint16_t CommandSetSupport_WWN64Bit : 1;
    uint16_t CommandSetSupport_URGReadStream : 1;
    uint16_t CommandSetSupport_URGWriteStream : 1;
    uint16_t CommandSetSupport_ReservedForTechReport : 2;
    uint16_t CommandSetSupport_IdleWithUnloadFeature : 1;
    uint16_t CommandSetSupport_WordValid : 2;
    uint16_t CommandSetActive_SmartCommands : 1;
    uint16_t CommandSetActive_SecurityMode : 1;
    uint16_t CommandSetActive_RemovableMediaFeature : 1;
    uint16_t CommandSetActive_PowerManagement : 1;
    uint16_t CommandSetActive_Reserved1 : 1;
    uint16_t CommandSetActive_WriteCache : 1;
    uint16_t CommandSetActive_LookAhead : 1;
    uint16_t CommandSetActive_ReleaseInterrupt : 1;
    uint16_t CommandSetActive_ServiceInterrupt : 1;
    uint16_t CommandSetActive_DeviceReset : 1;
    uint16_t CommandSetActive_HostProtectedArea : 1;
    uint16_t CommandSetActive_Obsolete1 : 1;
    uint16_t CommandSetActive_WriteBuffer : 1;
    uint16_t CommandSetActive_ReadBuffer : 1;
    uint16_t CommandSetActive_Nop : 1;
    uint16_t CommandSetActive_Obsolete2 : 1;
    uint16_t CommandSetActive_DownloadMicrocode : 1;
    uint16_t CommandSetActive_DmaQueued : 1;
    uint16_t CommandSetActive_Cfa : 1;
    uint16_t CommandSetActive_AdvancedPm : 1;
    uint16_t CommandSetActive_Msn : 1;
    uint16_t CommandSetActive_PowerUpInStandby : 1;
    uint16_t CommandSetActive_ManualPowerUp : 1;
    uint16_t CommandSetActive_Reserved2 : 1;
    uint16_t CommandSetActive_SetMax : 1;
    uint16_t CommandSetActive_Acoustics : 1;
    uint16_t CommandSetActive_BigLba : 1;
    uint16_t CommandSetActive_DeviceConfigOverlay : 1;
    uint16_t CommandSetActive_FlushCache : 1;
    uint16_t CommandSetActive_FlushCacheExt : 1;
    uint16_t CommandSetActive_Resrved3 : 1;
    uint16_t CommandSetActive_Words119_120Valid : 1;
    uint16_t CommandSetActive_SmartErrorLog : 1;
    uint16_t CommandSetActive_SmartSelfTest : 1;
    uint16_t CommandSetActive_MediaSerialNumber : 1;
    uint16_t CommandSetActive_MediaCardPassThrough : 1;
    uint16_t CommandSetActive_StreamingFeature : 1;
    uint16_t CommandSetActive_GpLogging : 1;
    uint16_t CommandSetActive_WriteFua : 1;
    uint16_t CommandSetActive_WriteQueuedFua : 1;
    uint16_t CommandSetActive_WWN64Bit : 1;
    uint16_t CommandSetActive_URGReadStream : 1;
    uint16_t CommandSetActive_URGWriteStream : 1;
    uint16_t CommandSetActive_ReservedForTechReport : 2;
    uint16_t CommandSetActive_IdleWithUnloadFeature : 1;
    uint16_t CommandSetActive_Reserved4 : 2;
    uint16_t UltraDMASupport : 8;
    uint16_t UltraDMAActive : 8;
    uint16_t NormalSecurityEraseUnit_TimeRequired : 15;
    uint16_t NormalSecurityEraseUnit_ExtendedTimeReported : 1;
    uint16_t EnhancedSecurityEraseUnit_TimeRequired : 15;
    uint16_t EnhancedSecurityEraseUnit_ExtendedTimeReported : 1;
    uint16_t CurrentAPMLevel : 8;
    uint16_t ReservedWord91 : 8;
    uint16_t MasterPasswordID;
    uint16_t HardwareResetResult;
    uint16_t CurrentAcousticValue : 8;
    uint16_t RecommendedAcousticValue : 8;
    uint16_t StreamMinRequestSize;
    uint16_t StreamingTransferTimeDMA;
    uint16_t StreamingAccessLatencyDMAPIO;
    uint64_t  StreamingPerfGranularity;
    uint64_t  Max48BitLBA[2];
    uint16_t StreamingTransferTime;
    uint16_t DsmCap;
    uint16_t PhysicalLogicalSectorSize_LogicalSectorsPerPhysicalSector : 4;
    uint16_t PhysicalLogicalSectorSize_Reserved0 : 8;
    uint16_t PhysicalLogicalSectorSize_LogicalSectorLongerThan256Words : 1;
    uint16_t PhysicalLogicalSectorSize_MultipleLogicalSectorsPerPhysicalSector : 1;
    uint16_t PhysicalLogicalSectorSize_Reserved1 : 2;
    uint16_t InterSeekDelay;
    uint16_t WorldWideName[4];
    uint16_t ReservedForWorldWideName128[4];
    uint16_t ReservedForTlcTechnicalReport;
    uint16_t WordsPerLogicalSector[2];
    uint16_t CommandSetSupportExt_ReservedForDrqTechnicalReport : 1;
    uint16_t CommandSetSupportExt_WriteReadVerify : 1;
    uint16_t CommandSetSupportExt_WriteUncorrectableExt : 1;
    uint16_t CommandSetSupportExt_ReadWriteLogDmaExt : 1;
    uint16_t CommandSetSupportExt_DownloadMicrocodeMode3 : 1;
    uint16_t CommandSetSupportExt_FreefallControl : 1;
    uint16_t CommandSetSupportExt_SenseDataReporting : 1;
    uint16_t CommandSetSupportExt_ExtendedPowerConditions : 1;
    uint16_t CommandSetSupportExt_Reserved0 : 6;
    uint16_t CommandSetSupportExt_WordValid : 2;
    uint16_t CommandSetActiveExt_ReservedForDrqTechnicalReport : 1;
    uint16_t CommandSetActiveExt_WriteReadVerify : 1;
    uint16_t CommandSetActiveExt_WriteUncorrectableExt : 1;
    uint16_t CommandSetActiveExt_ReadWriteLogDmaExt : 1;
    uint16_t CommandSetActiveExt_DownloadMicrocodeMode3 : 1;
    uint16_t CommandSetActiveExt_FreefallControl : 1;
    uint16_t CommandSetActiveExt_SenseDataReporting : 1;
    uint16_t CommandSetActiveExt_ExtendedPowerConditions : 1;
    uint16_t CommandSetActiveExt_Reserved0 : 6;
    uint16_t CommandSetActiveExt_Reserved1 : 2;
    uint16_t ReservedForExpandedSupportandActive[6];
    uint16_t MsnSupport : 2;
    uint16_t ReservedWord127 : 14;
    uint16_t SecurityStatus_SecuritySupported : 1;
    uint16_t SecurityStatus_SecurityEnabled : 1;
    uint16_t SecurityStatus_SecurityLocked : 1;
    uint16_t SecurityStatus_SecurityFrozen : 1;
    uint16_t SecurityStatus_SecurityCountExpired : 1;
    uint16_t SecurityStatus_EnhancedSecurityEraseSupported : 1;
    uint16_t SecurityStatus_Reserved0 : 2;
    uint16_t SecurityStatus_SecurityLevel : 1;
    uint16_t SecurityStatus_Reserved1 : 7;
    uint16_t ReservedWord129[31];
    uint16_t CfaPowerMode1_MaximumCurrentInMA : 12;
    uint16_t CfaPowerMode1_CfaPowerMode1Disabled : 1;
    uint16_t CfaPowerMode1_CfaPowerMode1Required : 1;
    uint16_t CfaPowerMode1_Reserved0 : 1;
    uint16_t CfaPowerMode1_Word160Supported : 1;
    uint16_t ReservedForCfaWord161[7];
    uint16_t NominalFormFactor : 4;
    uint16_t ReservedWord168 : 12;
    uint16_t DataSetManagementFeature_SupportsTrim : 1;
    uint16_t DataSetManagementFeature_Reserved0 : 15;
    uint16_t AdditionalProductID[4];
    uint16_t ReservedForCfaWord174[2];
    uint16_t CurrentMediaSerialNumber[30];
    uint16_t SCTCommandTransport_Supported : 1;
    uint16_t SCTCommandTransport_Reserved0 : 1;
    uint16_t SCTCommandTransport_WriteSameSuported : 1;
    uint16_t SCTCommandTransport_ErrorRecoveryControlSupported : 1;
    uint16_t SCTCommandTransport_FeatureControlSuported : 1;
    uint16_t SCTCommandTransport_DataTablesSuported : 1;
    uint16_t SCTCommandTransport_Reserved1 : 6;
    uint16_t SCTCommandTransport_VendorSpecific : 4;
    uint16_t ReservedWord207[2];
    uint16_t BlockAlignment_AlignmentOfLogicalWithinPhysical : 14;
    uint16_t BlockAlignment_Word209Supported : 1;
    uint16_t BlockAlignment_Reserved0 : 1;
    uint16_t WriteReadVerifySectorCountMode3Only[2];
    uint16_t WriteReadVerifySectorCountMode2Only[2];
    uint16_t NVCacheCapabilities_NVCachePowerModeEnabled : 1;
    uint16_t NVCacheCapabilities_Reserved0 : 3;
    uint16_t NVCacheCapabilities_NVCacheFeatureSetEnabled : 1;
    uint16_t NVCacheCapabilities_Reserved1 : 3;
    uint16_t NVCacheCapabilities_NVCachePowerModeVersion : 4;
    uint16_t NVCacheCapabilities_NVCacheFeatureSetVersion : 4;
    uint16_t NVCacheSizeLSW;
    uint16_t NVCacheSizeMSW;
    uint16_t NominalMediaRotationRate;
    uint16_t ReservedWord218;
    uint8_t NVCacheOptions_NVCacheEstimatedTimeToSpinUpInSeconds;
    uint8_t NVCacheOptions_Reserved;
    uint16_t WriteReadVerifySectorCountMode : 8;
    uint16_t ReservedWord220 : 8;
    uint16_t ReservedWord221;
    uint16_t TransportMajorVersion_MajorVersion : 12;
    uint16_t TransportMajorVersion_TransportType : 4;
    uint16_t TransportMinorVersion;
    uint16_t ReservedWord224[6];
    uint64_t  ExtendedNumberOfUserAddressableSectors[2];
    uint16_t MinBlocksPerDownloadMicrocodeMode03;
    uint16_t MaxBlocksPerDownloadMicrocodeMode03;
    uint16_t ReservedWord236[19];
    uint16_t Signature : 8;
    uint16_t CheckSum : 8;
} __attribute__((packed));

void init_drive(void);
void exit_drive(void);

#endif