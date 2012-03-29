#ifndef FAT_H
#define FAT_H

typedef enum {
  FAT_SUCCESS,
  FAT_INVALID,
  FAT_NOSPACE,
  FAT_IOERROR,
} fat_error;

typedef enum {
  FAT_READ,
  FAT_WRITE,
} fat_mode;

struct block_ops {
  unsigned int (*sector_read)(const unsigned int lba, void *buf);
  unsigned int (*sector_write)(const unsigned int lba, void *buf);
};
  
struct fat_context;
struct fat_file;

struct fat_context *fat_disk_open(struct block_ops *);
struct fat_file *fat_file_open(struct fat_context *, const char *, fat_mode);

unsigned int fat_write(struct fat_file *, const void *buf, unsigned int count);
unsigned int fat_read(struct fat_file *, void *buf, unsigned int count);

fat_error fat_sync(struct fat_file *);

#endif

