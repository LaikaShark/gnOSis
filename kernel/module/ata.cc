#include <system.h>

static int ata_present = 0;

// Wait for BSY to clear, return 0 on success, -1 on timeout/error
static int ata_poll()
{
    // 400ns delay: read alt status 4 times
    io::inb(ATA_ALT_STATUS);
    io::inb(ATA_ALT_STATUS);
    io::inb(ATA_ALT_STATUS);
    io::inb(ATA_ALT_STATUS);

    u8int status;
    for (int i = 0; i < 100000; i++)
    {
        status = io::inb(ATA_STATUS);
        if (!(status & ATA_SR_BSY))
        {
            if (status & ATA_SR_ERR) return -1;
            return 0;
        }
    }
    return -1; // timeout
}

static int ata_identify()
{
    // Select master drive
    io::outb(ATA_DRIVE_HEAD, 0xA0);

    // Zero out sector count and LBA registers
    io::outb(ATA_SECT_COUNT, 0);
    io::outb(ATA_LBA_LO, 0);
    io::outb(ATA_LBA_MID, 0);
    io::outb(ATA_LBA_HI, 0);

    // Send IDENTIFY
    io::outb(ATA_COMMAND, ATA_CMD_IDENTIFY);

    // Read status
    u8int status = io::inb(ATA_STATUS);
    if (status == 0) return -1; // no drive

    // Wait for BSY to clear
    if (ata_poll() != 0) return -1;

    // Check LBA_MID and LBA_HI for non-ATA
    if (io::inb(ATA_LBA_MID) != 0 || io::inb(ATA_LBA_HI) != 0)
        return -1; // not ATA

    // Wait for DRQ
    for (int i = 0; i < 100000; i++)
    {
        status = io::inb(ATA_STATUS);
        if (status & ATA_SR_ERR) return -1;
        if (status & ATA_SR_DRQ) break;
    }

    if (!(io::inb(ATA_STATUS) & ATA_SR_DRQ)) return -1;

    // Read 256 words of identification data (discard)
    for (int i = 0; i < 256; i++)
        io::inw(ATA_DATA);

    return 0;
}

void init_ata()
{
    printj("Initializing ATA......");
    if (ata_identify() == 0)
    {
        ata_present = 1;
        printj("DONE\n");
    }
    else
    {
        ata_present = 0;
        printj("NO DISK\n");
    }
}

int ata_read_sector(u32int lba, u8int* buf)
{
    if (!ata_present) return -1;

    // Select drive, LBA mode, bits 24-27 of LBA
    io::outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    io::outb(ATA_SECT_COUNT, 1);
    io::outb(ATA_LBA_LO, lba & 0xFF);
    io::outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    io::outb(ATA_LBA_HI, (lba >> 16) & 0xFF);
    io::outb(ATA_COMMAND, ATA_CMD_READ_PIO);

    if (ata_poll() != 0) return -1;

    // Wait for DRQ
    u8int status;
    for (int i = 0; i < 100000; i++)
    {
        status = io::inb(ATA_STATUS);
        if (status & ATA_SR_ERR) return -1;
        if (status & ATA_SR_DRQ) break;
    }
    if (!(io::inb(ATA_STATUS) & ATA_SR_DRQ)) return -1;

    // Read 256 words
    u16int* wbuf = (u16int*)buf;
    for (int i = 0; i < 256; i++)
        wbuf[i] = io::inw(ATA_DATA);

    return 0;
}

int ata_write_sector(u32int lba, const u8int* buf)
{
    if (!ata_present) return -1;

    // Select drive, LBA mode, bits 24-27 of LBA
    io::outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    io::outb(ATA_SECT_COUNT, 1);
    io::outb(ATA_LBA_LO, lba & 0xFF);
    io::outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    io::outb(ATA_LBA_HI, (lba >> 16) & 0xFF);
    io::outb(ATA_COMMAND, ATA_CMD_WRITE_PIO);

    if (ata_poll() != 0) return -1;

    // Wait for DRQ
    u8int status;
    for (int i = 0; i < 100000; i++)
    {
        status = io::inb(ATA_STATUS);
        if (status & ATA_SR_ERR) return -1;
        if (status & ATA_SR_DRQ) break;
    }
    if (!(io::inb(ATA_STATUS) & ATA_SR_DRQ)) return -1;

    // Write 256 words
    const u16int* wbuf = (const u16int*)buf;
    for (int i = 0; i < 256; i++)
        io::outw(ATA_DATA, wbuf[i]);

    // Flush cache
    ata_flush();

    return 0;
}

void ata_flush()
{
    if (!ata_present) return;
    io::outb(ATA_DRIVE_HEAD, 0xE0);
    io::outb(ATA_COMMAND, ATA_CMD_FLUSH);
    ata_poll();
}
