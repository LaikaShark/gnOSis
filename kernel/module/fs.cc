#include <system.h>

static fs_superblock superblock;
static fs_dirent directory[FS_MAX_FILES];
static u8int bitmap[FS_SECTOR_SIZE];
static fs_openfile open_files[FS_MAX_OPEN];
static u8int sector_buf[FS_SECTOR_SIZE];
static int fs_ready = 0;

// Write directory sectors (1-2) to disk
static void flush_directory()
{
    ata_write_sector(FS_DIR_SECTOR_START, (const u8int*)&directory[0]);
    ata_write_sector(FS_DIR_SECTOR_START + 1, (const u8int*)&directory[16]);
}

// Write bitmap sector (3) to disk
static void flush_bitmap()
{
    ata_write_sector(FS_BITMAP_SECTOR, bitmap);
}

// Set a bit in the bitmap
static void bitmap_set(u32int sector)
{
    bitmap[sector / 8] |= (1 << (sector % 8));
}

// Clear a bit in the bitmap
static void bitmap_clear(u32int sector)
{
    bitmap[sector / 8] &= ~(1 << (sector % 8));
}

// Test a bit in the bitmap
static int bitmap_test(u32int sector)
{
    return bitmap[sector / 8] & (1 << (sector % 8));
}

// Find n consecutive free sectors starting from FS_DATA_START
// Returns start sector, or 0 on failure
static u32int bitmap_alloc(u32int n)
{
    u32int run_start = FS_DATA_START;
    u32int run_len = 0;

    for (u32int s = FS_DATA_START; s < FS_TOTAL_SECTORS; s++)
    {
        if (bitmap_test(s))
        {
            run_start = s + 1;
            run_len = 0;
        }
        else
        {
            run_len++;
            if (run_len >= n)
            {
                // Mark them used
                for (u32int i = 0; i < n; i++)
                    bitmap_set(run_start + i);
                return run_start;
            }
        }
    }
    return 0; // no space
}

// Look up a filename in the directory, return index or -1
static int dir_find(const char* name, int namelen)
{
    for (int i = 0; i < FS_MAX_FILES; i++)
    {
        if (!directory[i].flags) continue;
        // Compare names
        int match = 1;
        for (int j = 0; j < namelen; j++)
        {
            if (directory[i].name[j] != name[j])
            {
                match = 0;
                break;
            }
        }
        if (match && directory[i].name[namelen] == '\0')
            return i;
    }
    return -1;
}

// Check if a directory entry is currently open
static int is_open(int dir_index)
{
    for (int i = 0; i < FS_MAX_OPEN; i++)
    {
        if (open_files[i].in_use && open_files[i].dir_index == dir_index)
            return 1;
    }
    return 0;
}

void fs_init()
{
    // Clear open file table
    for (int i = 0; i < FS_MAX_OPEN; i++)
        open_files[i].in_use = 0;

    // Read superblock
    if (ata_read_sector(FS_SUPERBLOCK_SECTOR, (u8int*)&superblock) != 0)
    {
        printj("FS: Cannot read disk\n");
        fs_ready = 0;
        return;
    }

    if (superblock.magic != FS_MAGIC)
    {
        printj("No filesystem. Use FORMAT-DISK.\n");
        fs_ready = 0;
        return;
    }

    // Load directory
    ata_read_sector(FS_DIR_SECTOR_START, (u8int*)&directory[0]);
    ata_read_sector(FS_DIR_SECTOR_START + 1, (u8int*)&directory[16]);

    // Load bitmap
    ata_read_sector(FS_BITMAP_SECTOR, bitmap);

    fs_ready = 1;
    printj("Filesystem ready.\n");
}

int fs_format()
{
    // Write superblock
    memset((char*)&superblock, 0, sizeof(superblock));
    superblock.magic = FS_MAGIC;
    superblock.version = FS_VERSION;
    superblock.total_sectors = FS_TOTAL_SECTORS;
    superblock.data_start = FS_DATA_START;
    if (ata_write_sector(FS_SUPERBLOCK_SECTOR, (const u8int*)&superblock) != 0)
        return -1;

    // Clear directory
    memset((char*)directory, 0, sizeof(directory));
    flush_directory();

    // Clear bitmap, mark sectors 0-3 as used
    memset((char*)bitmap, 0, FS_SECTOR_SIZE);
    bitmap_set(0); // superblock
    bitmap_set(1); // dir 1
    bitmap_set(2); // dir 2
    bitmap_set(3); // bitmap
    flush_bitmap();

    // Clear open file table
    for (int i = 0; i < FS_MAX_OPEN; i++)
        open_files[i].in_use = 0;

    fs_ready = 1;
    printj("Disk formatted.\n");
    return 0;
}

int fs_create(const char* name, int namelen)
{
    if (!fs_ready) return -1;
    if (namelen > FS_MAX_NAME) return -1;

    // Check if already exists
    if (dir_find(name, namelen) >= 0) return -1;

    // Find free slot
    int slot = -1;
    for (int i = 0; i < FS_MAX_FILES; i++)
    {
        if (!directory[i].flags)
        {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1; // directory full

    // Initialize entry
    memset((char*)&directory[slot], 0, sizeof(fs_dirent));
    directory[slot].flags = 1;
    for (int i = 0; i < namelen; i++)
        directory[slot].name[i] = name[i];
    directory[slot].name[namelen] = '\0';
    directory[slot].start_sector = 0;
    directory[slot].size_bytes = 0;
    directory[slot].sector_count = 0;

    flush_directory();
    return 0;
}

int fs_open(const char* name, int namelen, int mode)
{
    if (!fs_ready) return -1;

    int di = dir_find(name, namelen);
    if (di < 0) return -1; // not found

    // Find free open file slot
    int fd = -1;
    for (int i = 0; i < FS_MAX_OPEN; i++)
    {
        if (!open_files[i].in_use)
        {
            fd = i;
            break;
        }
    }
    if (fd < 0) return -1; // too many open files

    open_files[fd].in_use = 1;
    open_files[fd].dir_index = di;
    open_files[fd].position = 0;
    open_files[fd].mode = mode;
    open_files[fd].dirty = 0;

    return fd;
}

int fs_close(int fd)
{
    if (fd < 0 || fd >= FS_MAX_OPEN) return -1;
    if (!open_files[fd].in_use) return -1;

    if (open_files[fd].dirty)
    {
        flush_directory();
        flush_bitmap();
    }

    open_files[fd].in_use = 0;
    return 0;
}

int fs_read(int fd, u8int* buf, int count)
{
    if (fd < 0 || fd >= FS_MAX_OPEN) return -1;
    if (!open_files[fd].in_use) return -1;
    if (!(open_files[fd].mode & FS_MODE_READ)) return -1;

    fs_dirent* de = &directory[open_files[fd].dir_index];

    // Clamp to remaining bytes
    u32int remaining = de->size_bytes - open_files[fd].position;
    if ((u32int)count > remaining)
        count = remaining;
    if (count <= 0) return 0;

    int bytes_read = 0;
    while (bytes_read < count)
    {
        u32int pos = open_files[fd].position;
        u32int sector_offset = pos / FS_SECTOR_SIZE;
        u32int byte_offset = pos % FS_SECTOR_SIZE;
        u32int sector = de->start_sector + sector_offset;

        if (ata_read_sector(sector, sector_buf) != 0)
            return -1;

        u32int chunk = FS_SECTOR_SIZE - byte_offset;
        if ((int)chunk > count - bytes_read)
            chunk = count - bytes_read;

        memcpy(buf + bytes_read, sector_buf + byte_offset, chunk);
        bytes_read += chunk;
        open_files[fd].position += chunk;
    }

    return bytes_read;
}

int fs_write(int fd, const u8int* buf, int count)
{
    if (fd < 0 || fd >= FS_MAX_OPEN) return -1;
    if (!open_files[fd].in_use) return -1;
    if (!(open_files[fd].mode & FS_MODE_WRITE)) return -1;

    fs_dirent* de = &directory[open_files[fd].dir_index];

    // If file has no sectors allocated, allocate now
    if (de->sector_count == 0)
    {
        // Calculate how many sectors we need
        u32int needed = (open_files[fd].position + count + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE;
        if (needed == 0) needed = 1;
        // Allocate a bit extra for growth (up to double, max 32 extra)
        u32int alloc = needed;
        if (alloc < 8) alloc = 8;
        if (alloc > FS_TOTAL_SECTORS - FS_DATA_START)
            alloc = FS_TOTAL_SECTORS - FS_DATA_START;

        u32int start = bitmap_alloc(alloc);
        if (start == 0)
        {
            // Try exact amount
            alloc = needed;
            start = bitmap_alloc(alloc);
            if (start == 0)
            {
                printj("FS: No space\n");
                return -1;
            }
        }
        de->start_sector = start;
        de->sector_count = alloc;
        open_files[fd].dirty = 1;
    }

    // Check if write fits in allocated sectors
    u32int max_bytes = de->sector_count * FS_SECTOR_SIZE;
    if (open_files[fd].position + (u32int)count > max_bytes)
    {
        printj("FS: File full (cannot grow)\n");
        count = max_bytes - open_files[fd].position;
        if (count <= 0) return -1;
    }

    int bytes_written = 0;
    while (bytes_written < count)
    {
        u32int pos = open_files[fd].position;
        u32int sector_offset = pos / FS_SECTOR_SIZE;
        u32int byte_offset = pos % FS_SECTOR_SIZE;
        u32int sector = de->start_sector + sector_offset;

        u32int chunk = FS_SECTOR_SIZE - byte_offset;
        if ((int)chunk > count - bytes_written)
            chunk = count - bytes_written;

        // Read-modify-write for partial sector
        if (byte_offset != 0 || chunk < FS_SECTOR_SIZE)
        {
            if (ata_read_sector(sector, sector_buf) != 0)
            {
                // Sector might not have been written yet, zero it
                memset((char*)sector_buf, 0, FS_SECTOR_SIZE);
            }
        }

        memcpy(sector_buf + byte_offset, buf + bytes_written, chunk);

        if (ata_write_sector(sector, sector_buf) != 0)
            return -1;

        bytes_written += chunk;
        open_files[fd].position += chunk;
    }

    // Update file size if we wrote past the end
    if (open_files[fd].position > de->size_bytes)
    {
        de->size_bytes = open_files[fd].position;
        open_files[fd].dirty = 1;
    }

    // Flush metadata
    if (open_files[fd].dirty)
    {
        flush_directory();
        flush_bitmap();
        open_files[fd].dirty = 0;
    }

    return bytes_written;
}

int fs_delete(const char* name, int namelen)
{
    if (!fs_ready) return -1;

    int di = dir_find(name, namelen);
    if (di < 0) return -1;

    // Check not open
    if (is_open(di))
    {
        printj("FS: File is open\n");
        return -1;
    }

    // Free bitmap bits
    for (u32int i = 0; i < directory[di].sector_count; i++)
        bitmap_clear(directory[di].start_sector + i);

    // Clear directory entry
    memset((char*)&directory[di], 0, sizeof(fs_dirent));

    flush_directory();
    flush_bitmap();
    return 0;
}

u32int fs_filesize(int fd)
{
    if (fd < 0 || fd >= FS_MAX_OPEN) return 0;
    if (!open_files[fd].in_use) return 0;
    return directory[open_files[fd].dir_index].size_bytes;
}

void fs_list()
{
    if (!fs_ready)
    {
        printj("No filesystem.\n");
        return;
    }

    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++)
    {
        if (!directory[i].flags) continue;
        printj(directory[i].name);
        printj("  ");
        print_dec(directory[i].size_bytes);
        printj(" bytes\n");
        count++;
    }
    if (count == 0)
        printj("(empty)\n");
}

void fs_flush()
{
    if (!fs_ready) return;
    flush_directory();
    flush_bitmap();
}
