/*
 * Copyright (c) 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * disk/ahci/registers.h
 * Driver for the Advanced Host Controller Interface.
 */

#ifndef SORTIX_DISK_AHCI_REGISTERS_H
#define SORTIX_DISK_AHCI_REGISTERS_H

#include <endian.h>
#include <stdint.h>

namespace Sortix {
namespace AHCI {

#define AHCI_COMMAND_HEADER_COUNT 32

#if 1 /* ATA STUFF */
#define ATA_STATUS_ERR  (1 << 0) /* Error. */
#define ATA_STATUS_DRQ  (1 << 3) /* Data Request. */
#define ATA_STATUS_DF   (1 << 5) /* Device Fault. */
#define ATA_STATUS_DRDY (1 << 6) /* Device Ready. */
#define ATA_STATUS_BSY  (1 << 7) /* Busy. */

/** ATA Commands. */
#define ATA_CMD_READ_DMA                0xC8 /**< READ DMA. */
#define ATA_CMD_READ_DMA_EXT            0x25 /**< READ DMA EXT. */
#define ATA_CMD_READ_SECTORS            0x20 /**< READ SECTORS. */
#define ATA_CMD_READ_SECTORS_EXT        0x24 /**< READ SECTORS EXT. */
#define ATA_CMD_WRITE_DMA               0xCA /**< WRITE DMA. */
#define ATA_CMD_WRITE_DMA_EXT           0x35 /**< WRITE DMA EXT. */
#define ATA_CMD_WRITE_SECTORS           0x30 /**< WRITE SECTORS. */
#define ATA_CMD_WRITE_SECTORS_EXT       0x34 /**< WRITE SECTORS EXT. */
#define ATA_CMD_PACKET                  0xA0 /**< PACKET. */
#define ATA_CMD_IDENTIFY_PACKET         0xA1 /**< IDENTIFY PACKET DEVICE. */
#define ATA_CMD_FLUSH_CACHE             0xE7 /**< FLUSH CACHE. */
#define ATA_CMD_FLUSH_CACHE_EXT         0xEA /**< FLUSH CACHE EXT. */
#define ATA_CMD_IDENTIFY                0xEC /**< IDENTIFY DEVICE. */
#endif

struct fis
{
	little_uint8_t dsfis[0x1C];     // DMA Setup FIS.
	little_uint8_t reserved1[0x04];
	little_uint8_t psfis[0x14];     // PIO Setup FIS.
	little_uint8_t reserved2[0x0C];
	little_uint8_t rfis[0x14];      // D2H Register FIS.
	little_uint8_t reserved3[0x04];
	little_uint8_t sdbfis[0x08];    // Set Device Bits FIS.
	little_uint8_t ufis[0x40];      // Unknown FIS.
	little_uint8_t reserved4[0x60];
};

/* TODO: Less blatantly copied from Kiwi: */
struct command_header
{
	/** DW0 - Description Information. */
	little_uint16_t dw0l;
	little_uint16_t prdtl;
	/** DW1 - Command Status. */
	little_uint32_t prdbc;  /**< Physical Region Descriptor Byte Count. */
	/** DW2 - Command Table Base Address.
	* @note Bits 0-6 are reserved, must be 0. */
	little_uint32_t ctba; /**< Command Table Descriptor Base Address. */
	/** DW3 - Command Table Base Address Upper. */
	little_uint32_t ctbau; /**< Command Table Descriptor Base Address Upper 32-bits. */
	/** DW4-7 - Reserved. */
	little_uint32_t reserved2[4];
};

static const uint32_t COMMAND_HEADER_DW0_WRITE = 1 << 6;

/* TODO: Less blatantly copied from Kiwi: */
struct prd
{
	/** DW0 - Data Base Address. */
	little_uint32_t dba;
	/** DW1 - Data Base Address Upper. */
	little_uint32_t dbau;
	/** DW2 - Reserved */
	little_uint32_t reserved1;
	/** DW3 - Description Information. */
	/* dw3 bits 0-21: data byte count */
	/* dw3 bits 22-30: reserved */
	/* dw3 bits 31-31: interrupt on completion */
	little_uint32_t dw3;
};

/* TODO: Less blatantly copied from Kiwi: */
struct command_table
{
	/** Command FIS - Host to Device. */
	struct
	{
		little_uint8_t type;                   /**< FIS Type (0x27). */
		/*little_*/uint8_t pm_port : 4;        /**< Port Multiplier Port. */
		/*little_*/uint8_t reserved1 : 3;      /**< Reserved. */
		/*little_*/uint8_t c_bit : 1;          /**< C bit. */
		little_uint8_t command;                /**< ATA Command. */
		little_uint8_t features_0_7;           /**< Features (bits 0-7). */
		little_uint8_t lba_0_7;                /**< LBA (bits 0-7). */
		little_uint8_t lba_8_15;               /**< LBA (bits 8-15). */
		little_uint8_t lba_16_23;              /**< LBA (bits 16-23). */
		little_uint8_t device;                 /**< Device (bits 24-27 for LBA28). */
		little_uint8_t lba_24_31;              /**< LBA48 (bits 24-31). */
		little_uint8_t lba_32_39;              /**< LBA48 (bits 32-39). */
		little_uint8_t lba_40_47;              /**< LBA48 (bits 40-47). */
		little_uint8_t features_8_15;          /**< Features (bits 8-15). */
		little_uint8_t count_0_7;              /**< Sector Count (bits 0-7). */
		little_uint8_t count_8_15;             /**< Sector Count (bits 8-15). */
		little_uint8_t icc;                    /**< Isochronous Command Completion. */
		little_uint8_t control;                /**< Device Control. */
		little_uint32_t reserved2;             /**< Reserved. */
		little_uint8_t padding[0x2C];          /**< Padding to 64 bytes. */
	} cfis;
	little_uint8_t acmd[0x10];                     /**< ATAPI Command (12 or 16 bytes). */
	little_uint8_t reserved[0x30];                 /**< Reserved. */
};

struct port_regs
{
	little_uint32_t pxclb;
	little_uint32_t pxclbu;
	little_uint32_t pxfb;
	little_uint32_t pxfbu;
	little_uint32_t pxis;
	little_uint32_t pxie;
	little_uint32_t pxcmd;
	little_uint32_t reserved0;
	little_uint32_t pxtfd;
	little_uint32_t pxsig;
	little_uint32_t pxssts;
	little_uint32_t pxsctl;
	little_uint32_t pxserr;
	little_uint32_t pxsact;
	little_uint32_t pxci;
	little_uint32_t pxsntf;
	little_uint32_t pxfbs;
	little_uint32_t preserved1[15];
};

/** Bits in the Port x Interrupt Enable register. */
#define PXIE_DHRE (1 << 0)          /**< Device to Host Register Enable. */
#define PXIE_PSE  (1 << 1)          /**< PIO Setup FIS Enable. */
#define PXIE_DSE  (1 << 2)          /**< DMA Setup FIS Enable. */
#define PXIE_SDBE (1 << 3)          /**< Set Device Bits Enable. */
#define PXIE_UFE  (1 << 4)          /**< Unknown FIS Enable. */
#define PXIE_DPE  (1 << 5)          /**< Descriptor Processed Enable. */
#define PXIE_PCE  (1 << 6)          /**< Port Connect Change Enable. */
#define PXIE_DMPE (1 << 7)          /**< Device Mechanical Presence Enable. */
#define PXIE_PRCE (1 << 22)         /**< PhyRdy Change Enable. */
#define PXIE_IPME (1 << 23)         /**< Incorrect Port Multiplier Enable. */
#define PXIE_OFE  (1 << 24)         /**< Overflow Enable. */
#define PXIE_INFE (1 << 26)         /**< Interface Non-Fatal Error Enable. */
#define PXIE_IFE  (1 << 27)         /**< Interface Fatal Error Enable. */
#define PXIE_HBDE (1 << 28)         /**< Host Bus Data Error Enable. */
#define PXIE_HBFE (1 << 29)         /**< Host Bus Fatal Error Enable. */
#define PXIE_TFEE (1 << 30)         /**< Task File Error Enable. */
#define PXIE_CPDE (1 << 31)         /**< Cold Port Detect Enable. */

#define PORT_INTR_ERROR    \
        (PXIE_UFE | PXIE_PCE | PXIE_PRCE | PXIE_IPME | \
         PXIE_OFE | PXIE_INFE | PXIE_IFE | PXIE_HBDE | \
         PXIE_HBFE | PXIE_TFEE)

static const uint32_t PXCMD_ST =    1 << 0; /* 0x00000001 */
static const uint32_t PXCMD_SUD =   1 << 1; /* 0x00000002 */
static const uint32_t PXCMD_POD =   1 << 2; /* 0x00000004 */
static const uint32_t PXCMD_CLO =   1 << 3; /* 0x00000008 */
static const uint32_t PXCMD_FRE =   1 << 4; /* 0x00000010 */
static inline uint32_t PXCMD_CSS(uint32_t val) { return (val >> 8) % 32; }
static const uint32_t PXCMD_MPSS =  1 << 13; /* 0x00002000 */
static const uint32_t PXCMD_FR =    1 << 14; /* 0x00004000 */
static const uint32_t PXCMD_CR =    1 << 15; /* 0x00008000 */
static const uint32_t PXCMD_CPS =   1 << 16; /* 0x00010000 */
static const uint32_t PXCMD_PMA =   1 << 17; /* 0x00020000 */
static const uint32_t PXCMD_HPCP =  1 << 18; /* 0x00040000 */
static const uint32_t PXCMD_MPSP =  1 << 19; /* 0x00080000 */
static const uint32_t PXCMD_CPD =   1 << 20; /* 0x00100000 */
static const uint32_t PXCMD_ESP =   1 << 21; /* 0x00200000 */
static const uint32_t PXCMD_FBSCP = 1 << 22; /* 0x00400000 */
static const uint32_t PXCMD_APSTE = 1 << 23; /* 0x00800000 */
static const uint32_t PXCMD_ATAPI = 1 << 24; /* 0x01000000 */
static const uint32_t PXCMD_DLAE =  1 << 25; /* 0x02000000 */
static const uint32_t PXCMD_ALPE =  1 << 26; /* 0x04000000 */
static const uint32_t PXCMD_ASP =   1 << 27; /* 0x08000000 */
static inline uint32_t PXCMD_ICC(uint32_t val) { return (val >> 28) % 16; }

struct hba_regs
{
	little_uint32_t cap;
	little_uint32_t ghc;
	little_uint32_t is;
	little_uint32_t pi;
	little_uint32_t vs;
	little_uint32_t ccc_ctl;
	little_uint32_t ccc_ports;
	little_uint32_t em_loc;
	little_uint32_t em_ctl;
	little_uint32_t cap2;
	little_uint32_t bohc;
	little_uint32_t reserved0[53];
	struct port_regs ports[32];
};

static_assert(sizeof(struct hba_regs) == 0x1100,
              "sizeof(struct hba_regs) == 0x1100");

static const uint32_t CAP_S64A = 1U << 31;
static const uint32_t CAP_SNCQ = 1U << 30;
static const uint32_t CAP_SSNTF = 1U << 29;
static const uint32_t CAP_SMPS = 1U << 28;
static const uint32_t CAP_SSS = 1U << 27;
static const uint32_t CAP_SALP = 1U << 26;
static const uint32_t CAP_SAL = 1U << 25;
static const uint32_t CAP_SCLO = 1U << 24;
static inline uint32_t CAP_ISS(uint32_t val) { return (val >> 20) % 16; }
static const uint32_t CAP_SAM = 1U << 18;
static const uint32_t CAP_SPM = 1U << 17;
static const uint32_t CAP_FBSS = 1U << 16;
static const uint32_t CAP_PMD = 1U << 15;
static const uint32_t CAP_SSC = 1U << 14;
static const uint32_t CAP_PSC = 1U << 13;
static inline uint32_t CAP_NCS(uint32_t val) { return (val >> 8) % 32 + 1; }
static const uint32_t CAP_CCCS = 1U << 7;
static const uint32_t CAP_EMS = 1U << 6;
static const uint32_t CAP_SXS = 1U << 5;
static inline uint32_t CAP_NP(uint32_t val) { return (val >> 0) % 32 + 1; }

static const uint32_t GHC_AE = 1U << 31;
static const uint32_t GHC_MRSM = 1U << 2;
static const uint32_t GHC_IE = 1U << 1;
static const uint32_t GHC_HR = 1U << 0;

} // namespace AHCI
} // namespace Sortix

#endif
