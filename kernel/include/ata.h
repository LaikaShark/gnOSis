#ifndef ATA_H_INCLUDED
#define ATA_H_INCLUDED

#include <system.h>

// Primary ATA controller ports
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECT_COUNT  0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE_HEAD   0x1F6
#define ATA_COMMAND     0x1F7
#define ATA_STATUS      0x1F7
#define ATA_ALT_STATUS  0x3F6

// Status register bits
#define ATA_SR_BSY      0x80
#define ATA_SR_DRDY     0x40
#define ATA_SR_DRQ      0x08
#define ATA_SR_ERR      0x01

// Commands
#define ATA_CMD_IDENTIFY    0xEC
#define ATA_CMD_READ_PIO    0x20
#define ATA_CMD_WRITE_PIO   0x30
#define ATA_CMD_FLUSH       0xE7

void init_ata();
int ata_read_sector(u32int lba, u8int* buf);
int ata_write_sector(u32int lba, const u8int* buf);
void ata_flush();

#endif
