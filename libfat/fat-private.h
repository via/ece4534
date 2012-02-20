#ifndef FAT_PRIVATE_H
#define FAT_PRIVATE_H

#include <stdint.h>

#include "fat.h"

typedef struct fat_extBS_16
{
  uint8_t   bios_drive_num;
  uint8_t   reserved1;
  uint8_t   boot_signature;
  uint32_t  volume_id;
  uint8_t   volume_label[11];
  uint8_t   fat_type_label[8];
}__attribute__((packed)) fat_extBS_16_t;

typedef struct fat_BS
{
  uint8_t bootjmp[3];
  uint8_t oem_name[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sector_count;
  uint8_t table_count;
  uint16_t root_entry_count;
  uint16_t total_sectors_16;
  uint8_t media_type;
  uint16_t table_size_16;
  uint16_t sectors_per_track;
  uint16_t head_side_count;
  uint32_t hidden_sector_count;
  uint32_t total_sectors_32;

  fat_extBS_16_t ebs;

}__attribute__((packed)) fat_BS_t;

struct fat_file {
  struct block_ops *ops;
  fat_mode mode;
  uint32_t file_pointer;

};  

#endif

