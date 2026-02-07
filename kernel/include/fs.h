#ifndef FS_H_INCLUDED
#define FS_H_INCLUDED

#include <system.h>

#define FS_MAGIC        0x674E4653  // "gNFS"
#define FS_VERSION      1
#define FS_TOTAL_SECTORS 4096
#define FS_SECTOR_SIZE  512

// Layout: sector 0 = superblock, 1-16 = directory, 17 = bitmap, 18+ = data
#define FS_SUPERBLOCK_SECTOR 0
#define FS_DIR_SECTOR_START  1
#define FS_DIR_SECTOR_COUNT  16
#define FS_BITMAP_SECTOR     17
#define FS_DATA_START        18

#define FS_MAX_FILES     256
#define FS_MAX_NAME      18
#define FS_MAX_OPEN      4

// File access modes
#define FS_MODE_READ     1
#define FS_MODE_WRITE    2
#define FS_MODE_RW       3

struct fs_superblock {
    u32int magic;
    u32int version;
    u32int total_sectors;
    u32int data_start;
    u8int  reserved[496];
} __attribute__((packed));

struct fs_dirent {
    u8int  flags;          // 0=free, 1=used
    char   name[19];       // null-terminated, max 18 chars
    u32int start_sector;
    u32int size_bytes;
    u32int sector_count;
} __attribute__((packed));

struct fs_openfile {
    int    in_use;
    int    dir_index;
    u32int position;
    int    mode;
    int    dirty;
};

void fs_init();
int  fs_format();
int  fs_create(const char* name, int namelen);
int  fs_open(const char* name, int namelen, int mode);
int  fs_close(int fd);
int  fs_read(int fd, u8int* buf, int count);
int  fs_write(int fd, const u8int* buf, int count);
int  fs_delete(const char* name, int namelen);
u32int fs_filesize(int fd);
void fs_list();
void fs_flush();

#endif
