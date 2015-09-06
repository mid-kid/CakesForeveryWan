#ifndef __headers_h__
#define __headers_h__

#include <stdint.h>

#define FIRM_TYPE_ARM9          0
#define FIRM_TYPE_ARM11         1

#define MEDIA_UNITS             0x200

#define NCCH_MAGIC              (0x4843434E)
#define NCSD_MAGIC              (0x4453434E)
#define FIRM_MAGIC              (0x4D524946)
#define ARM9BIN_MAGIC           (0x47704770)

typedef struct firm_section_h {
    uint32_t offset;
    void* address;
    uint32_t size;
    uint32_t type;                // Firmware Type ('0'=ARM9/'1'=ARM11)
    uint8_t hash[0x20];         // SHA-256 Hash of Firmware Section
} firm_section_h;               // 0x30

typedef struct firm_h {
    uint32_t magic;             // FIRM
    uint32_t reserved1;
    void* a11Entry;             // ARM11 entry
    void* a9Entry;                // ARM9 entry
    uint8_t reserved2[0x30];
    firm_section_h section[4];
    uint8_t sig[0x100];
} firm_h;

// http://3dbrew.org/wiki/NCCH/Extended_Header
typedef struct code_set_info {
    uint32_t address;
    uint32_t phy_region_size;   // Physical region size (in page-multiples)
    uint32_t size;              // size in bytes
} code_set_info;                // 0x0C

typedef struct system_info {
    uint32_t saveDataSize[2];
    uint32_t jumpID[2];
    uint8_t reserved[0x30];
} system_info;                  // 0x40

typedef struct system_control_info {
    uint8_t appTitle[8];
    char reserved1[5];
    char flag;
    char remasterVersion[2];
    code_set_info textCodeSet;
    uint32_t stackSize;
    code_set_info roCodeSet;
    char reserved2[4];
    code_set_info dataCodeSet;
    uint32_t bssSize;
    char depends[0x180];        // Dependency Module (Program ID) List 48*8
    system_info systemInfo;
} system_control_info;          // 0x200

typedef struct ncch_ex_h {
    system_control_info sci;
    uint8_t aci[0x200];
    uint8_t accessDescSig[0x100];
    uint8_t ncchPubKey[0x100];
    uint8_t aciLim[0x200];
} ncch_ex_h;                    // 0x800

// http://3dbrew.org/wiki/NCCH
typedef struct ncch_h {
    uint8_t sig[0x100];         // RSA-2048 signature of the NCCH header, using SHA-256.
    uint32_t magic;             // NCCH
    uint32_t contentSize;       // Content size, in media units (1 media unit = 0x200 bytes)
    uint8_t partitionID[8];
    uint8_t makerCode[2];
    uint16_t version;
    uint8_t reserved1[4];
    uint8_t programID[8];
    uint8_t reserved2[0x10];
    uint8_t logoHash[0x20];     // Logo Region SHA-256 hash. (For applications built with SDK 5+) (Supported from firmware: 5.0.0-11)
    uint8_t productCode[0x10];
    uint8_t exHeaderHash[0x20]; // Extended header SHA-256 hash (SHA256 of 2x Alignment Size, beginning at 0x0 of ExHeader)
    uint32_t exHeaderSize;      // Extended header size
    uint32_t reserved3;
    uint8_t flags[8];
    uint32_t plainOffset;       // media unit
    uint32_t plainSize;         // media unit
    uint32_t logoOffset;        // media unit
    uint32_t logoSize;          // media unit
    uint32_t exeFSOffset;       // media unit
    uint32_t exeFSSize;         // media unit
    uint32_t exeFSHashSize;     // media unit ExeFS hash region size
    uint32_t reserved4;
    uint32_t romFSOffset;       // media unit
    uint32_t romFSSize;         // media unit
    uint32_t romFSHashSize;     // media unit RomFS hash region size
    uint32_t reserved5;
    uint8_t exeFSHash[0x20];    // ExeFS superblock SHA-256 hash - (SHA-256 hash, starting at 0x0 of the ExeFS over the number of media units specified in the ExeFS hash region size)
    uint8_t romFSHash[0x20];    // RomFS superblock SHA-256 hash - (SHA-256 hash, starting at 0x0 of the RomFS over the number of media units specified in the RomFS hash region size)
} ncch_h;                       // 0x200

#define NCCH_FLAG_NOCRYPTO      0x4
#define NCCH_FLAG_7XCRYPTO      0x1

typedef struct cxi_h {
    ncch_h ncch;
    ncch_ex_h exheader;
} cxi_h;

typedef struct exefs_file_h {
    uint8_t fname[0x8];
    uint32_t offset;            // offset starts after exefs_h
    uint32_t size;
} exefs_file_h;                 // 0x10

typedef struct exefs_file_hash {
    uint8_t hash[0x20];
} exefs_file_hash;

typedef struct exefs_h {
    exefs_file_h fileHeaders[10];// File headers (10 headers maximum, 16 bytes each)
    uint8_t reserved[0x20];
    exefs_file_hash fileHashes[10];// File hashes (10 hashes maximum, 32 bytes each, one for each header), SHA256 over entire file content
} exefs_h;                      // 0x200

typedef struct ncsd_partition_table {
    uint32_t offset;
    uint32_t size;
} ncsd_partition_table;

// http://3dbrew.org/wiki/NCSD
typedef struct ncsd_h {
    uint8_t sig[0x100];         // RSA-2048 signature of the NCSD header, using SHA-256.
    uint32_t magic;             // NCSD
    uint32_t size;              // Size of the NCSD image, in media units (1 media unit = 0x200 bytes)
    uint8_t mediaID[8];
    uint8_t fsType[8];          // Partitions FS type (0=None, 1=Normal, 3=FIRM, 4=AGB_FIRM save)
    uint8_t cryptType[8];       // Partitions crypt type
    ncsd_partition_table ptable[8];// Offset & Length partition table, in media units
    uint8_t spec[0xA0];
} ncsd_h;

#define NCSD_PARTITION_EXE          0
#define NCSD_PARTITION_MANUAL       1
#define NCSD_PARTITION_DLP          2
#define NCSD_PARTITION_N3DSUPDATE   6
#define NCSD_PARTITION_O3DSUPDATE   7

typedef struct arm9bin_h {
    uint8_t keyx[0x10];
    uint8_t keyy[0x10];
    uint8_t ctr[0x10];
    char size[8];
    uint8_t pad[8];
    uint8_t ctl_block[0x10];
    uint8_t unk[0x10];
    uint8_t slot0x16keyX[0x10];
} arm9bin_h;

// http://3dbrew.org/wiki/Ticket
typedef struct ticket_h
{
    uint8_t sigIssuer[0x40];
    uint8_t eccPubKey[0x3C];
    uint8_t version;
    uint8_t caCrlVersion;
    uint8_t signerCrlVersion;
    uint8_t titleKey[0x10];
    uint8_t reserved;
    uint8_t ticketID[8];
    uint8_t consoleID[4];
    uint8_t titleID[8];
    uint8_t reserved2[2];
    uint16_t ticketTitleVersion;
    uint8_t reserved3[8];
    uint8_t licenseType;
    uint8_t ticketCommonKeyYIndex;    //Ticket common keyY index, usually 0x1 for retail system titles.
    uint8_t reserved4[0x2A];
    uint8_t unk[4];                    // eShop Account ID?
    uint8_t reserved5;
    uint8_t audit;
    uint8_t reserved6[0x42];
    uint8_t limits[0x40];
    uint8_t contentIndex[0xAC];
} __attribute__((__packed__)) ticket_h; // 0x210

#define SIG_TYPE_RSA4096_SHA1        0x010000
#define SIG_TYPE_RSA2048_SHA1        0x010001
#define SIG_TYPE_ECDSA_SHA1          0x010002
#define SIG_TYPE_RSA4096_SHA256      0x010003
#define SIG_TYPE_RSA2048_SHA256      0x010004
#define SIG_TYPE_ECDSA_SHA256        0x010005

#endif /*__headers_h__*/
