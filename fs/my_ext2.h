// 1024 block size
// 16   block group
// 8K   block per group

// For every block group
//
//            +-------------------------------------------------------------------------------------------------------+
//   block#   |       0      |          1          |          2         |        3       |   4  -  67  |  68 -  8191  |
//            +--------------+---------------------+--------------------+----------------+-------------+--------------+
//   Content  |  superblock  |  group descriptors  |  datablock bitmap  |  inode bitmap  | inode table |  data block  |
//            +--------------+---------------------+--------------------+----------------+-------------+--------------+
//
// root directory has inode  0#

#define SUPERBLOCK_OFFSET 0
#define GROUP_DESCRIPTORS_OFFSET  1
#define DATABLOCK_BITMAP_OFFSET  2
#define INODE_BITMAP_OFFSET  3
#define INODE_TABLE_OFFSET  4
#define DATA_OFFSET 68



#include <stdint.h>

#define BLOCK_SIZE 1024
#define BLOCKS_COUNT 131072
#define INODES_COUNT 8192
#define GROUPS_COUNT 16
#define BLOCKS_PER_GROUP 8192
#define INODES_PER_GROUP 512

struct myext2_super_block
{
    uint32_t s_block_size;        // 块大小  1K
    uint32_t s_blocks_count;      // 块数量  128K
    uint32_t s_inodes_count;      // inode 数量 128K
    uint32_t s_groups_count;      // 组数量  16
    uint32_t s_blocks_per_group;  // 每一组中的块数 8K
    uint32_t s_inodes_per_group;  // 每一组中的inode数 512
    uint32_t s_free_blocks_count; // 总可用的块数
    uint32_t s_free_inodes_count; // 总可用的inode数
};

/* 16字节 每项 */
/* 所有的组描述项 需要 16*16=256字节 1个块*/
struct myext2_group_descriptor
{
    uint32_t g_block_bitmap;      // 这组的block bitmap所在块
    uint32_t g_inode_bitmap;      // 这组的inode bitmap所在块
    uint32_t g_inode_table;       // 这组的inode table所在块
    uint16_t g_free_blocks_count; // 这组的可用的块数
    uint16_t g_free_inode_count;  // 这组的可用的inode数
};


#define FILE_REGULAR 0x01
#define FILE_DIRECTORY 0x02

static inline char* typeoffile(uint16_t mode)
{
    static char * regular = "REGULAR";
    static char * directory = "DIRECTORY";
    static char * unknow = "UNKNOW";
    if (mode & FILE_REGULAR)
        return regular;
    else if (mode & FILE_DIRECTORY)
        return directory;
    else
        return unknow;
}


/* 每个 inode 128 */
/* 1块可存放 8个inode */
/* 一组需要 512/8= 64 块 来存储inode */
struct myext2_inode
{
    uint32_t i_num;
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_access_time;
    uint32_t i_modify_time;
    uint32_t i_change_time;
    uint32_t i_size;

    uint32_t i_data_blocks[15];
    // Direct 12              12288       12K
    // Indirect 1             262144      256K
    // Double Indirect 1      67108864    64M
    // Triple Indirect 1      17179869184 16G
    //                        17247252480         (最大文件长度)
    uint32_t i_unused[11];
};
#define __DIRECT_SIZE (12 * 1024)
#define __INDIRECT_SIZE (256 * 1024)
#define __DOUBLE_INDIRECT_SIZE (256 * 256 * 1024)
#define __TRIPLE_INDIRECT_SIZE (256 * 256 * 256 * 1024)
#define MAX_FILE_LENGTH (__DIRECT_SIZE + __INDIRECT_SIZE + __DOUBLE_INDIRECT_SIZE + __TRIPLE_INDIRECT_SIZE)

#define DIRECTORY_ENTRY_SIZE 128
/* 128字节每个目录项 */
/* 最大文件名长度 96 */
struct myext2_directory
{
    uint32_t d_inode;
    char d_name[124];
};

#define BLOCKS_COUNT_USED_BY_EXT2_PER_GROUP (1 + 1 + 1 + 1 + 64)

