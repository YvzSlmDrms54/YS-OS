/* drivers/ata.c - ATA disk access in PIO mode.
 *
 * PIO means the CPU moves every byte itself, through a port. It is slow
 * compared to DMA, but it needs no setup and no interrupt handling, so
 * it is where every hobby OS starts.
 *
 * We only handle the primary channel, master drive, with 28-bit LBA.
 * That covers 128 GiB, which is rather more than Fish will ever use.
 */

#include "ata.h"
#include "io.h"

#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE       0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7
#define ATA_CONTROL     0x3F6

/* Status register bits. */
#define STATUS_ERR      0x01
#define STATUS_DRQ      0x08    /* ready to transfer a block */
#define STATUS_DF       0x20    /* drive fault */
#define STATUS_RDY      0x40
#define STATUS_BSY      0x80

#define CMD_READ        0x20
#define CMD_WRITE       0x30
#define CMD_FLUSH       0xE7
#define CMD_IDENTIFY    0xEC

static int present;

int ata_present(void) { return present; }

/* Reading the status port takes about 100 ns, so four reads is the
 * traditional way to wait the 400 ns the spec asks for after a command. */
static void delay_400ns(void)
{
    inb(ATA_CONTROL);
    inb(ATA_CONTROL);
    inb(ATA_CONTROL);
    inb(ATA_CONTROL);
}

/* Waits for the drive to stop being busy and become ready to transfer.
 * Returns 0 on success, -1 if the drive reported an error, -2 on timeout. */
static int wait_ready(void)
{
    /* A bounded loop, not while(1). A missing or broken drive would
     * otherwise hang the whole system here forever. */
    for (uint32_t i = 0; i < 1000000; i++) {
        uint8_t status = inb(ATA_STATUS);

        if (status & STATUS_BSY) continue;
        if (status & (STATUS_ERR | STATUS_DF)) return -1;
        if (status & STATUS_DRQ) return 0;
    }
    return -2;
}

static int wait_not_busy(void)
{
    for (uint32_t i = 0; i < 1000000; i++) {
        if (!(inb(ATA_STATUS) & STATUS_BSY)) return 0;
    }
    return -2;
}

/* Loads the sector address into the drive's registers. */
static void select_sector(uint32_t lba, uint8_t count)
{
    /* 0xE0 = master drive, LBA mode. The low nibble carries the top
     * four bits of the 28-bit address. */
    outb(ATA_DRIVE,    (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
}

int ata_init(void)
{
    uint8_t status;

    present = 0;

    outb(ATA_DRIVE, 0xA0);           /* select the master drive */
    delay_400ns();

    status = inb(ATA_STATUS);
    if (status == 0xFF || status == 0x00) return 0;   /* nothing on the bus */

    /* Clear the address registers, then ask the drive to identify itself. */
    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA_LOW,  0);
    outb(ATA_LBA_MID,  0);
    outb(ATA_LBA_HIGH, 0);
    outb(ATA_COMMAND, CMD_IDENTIFY);
    delay_400ns();

    if (inb(ATA_STATUS) == 0) return 0;               /* no drive */
    if (wait_ready() != 0) return 0;

    /* IDENTIFY returns one sector of information about the drive. We do
     * not use it yet, but it has to be read or the drive stays stuck. */
    for (int i = 0; i < 256; i++) (void)inw(ATA_DATA);

    present = 1;
    return 1;
}

int ata_read(uint32_t lba, uint8_t count, void *buffer)
{
    uint16_t *out = (uint16_t *)buffer;

    if (!present) return -1;
    if (count == 0) return 0;

    if (wait_not_busy() != 0) return -2;

    select_sector(lba, count);
    outb(ATA_COMMAND, CMD_READ);

    for (uint8_t sector = 0; sector < count; sector++) {
        int result = wait_ready();
        if (result != 0) return result;

        /* One sector is 512 bytes, moved 2 bytes at a time. */
        for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++) {
            *out++ = inw(ATA_DATA);
        }
        delay_400ns();
    }

    return 0;
}

int ata_write(uint32_t lba, uint8_t count, const void *buffer)
{
    const uint16_t *in = (const uint16_t *)buffer;

    if (!present) return -1;
    if (count == 0) return 0;

    if (wait_not_busy() != 0) return -2;

    select_sector(lba, count);
    outb(ATA_COMMAND, CMD_WRITE);

    for (uint8_t sector = 0; sector < count; sector++) {
        int result = wait_ready();
        if (result != 0) return result;

        for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++) {
            outw(ATA_DATA, *in++);
        }
        delay_400ns();
    }

    /* The drive may still be holding the data in its own cache. Tell it
     * to commit, or a reset could lose everything we just wrote. */
    outb(ATA_COMMAND, CMD_FLUSH);
    wait_not_busy();

    return 0;
}