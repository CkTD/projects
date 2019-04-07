#include "my_ext2.h"
#include "memprint.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/**********************************************************
 *               Function  Prototype                      *
 **********************************************************/
// common
void setbit(void *buffer, unsigned int pos);
void clearbit(void *buffer, unsigned int pos);
int testbit(void *buffer, unsigned int pos);
int find_first_unsetbit(void *buffer, unsigned int length);
// fs
int get_inode(int fd, int inodenum, struct myext2_inode *inode);                                          // DONE
int write_inode(int fd, int inodenum, void *buf);                                                         // DONE
int get_free_inode(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd);            // DONE
int free_inode(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd, int inodenum);  // DONE
int get_block(int fd, int blocknum, void *buf);                                                           // DONE
int write_block(int fd, int blocknum, void *buf);                                                         // DONE
int get_block_in_inode(int fd, int blocknum, const struct myext2_inode *inode, void *buf);                // DONE
int get_free_block(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd);            // DONE
int free_block(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd, int blocknum);  // DONE
int free_data_blocks_in_inode(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd); // TODO
int append_data_to_file(int fd, struct myext2_directory *sb, struct myext2_group_descriptor *gd,
                        struct myext2_inode *inode, int inodenum, void *buf, int size); // TODO
int sync(int fd, struct myext2_directory *sb, struct myext2_group_descriptor *gd);// TODO


// command
// DONE:
int mfs_mkfs(char *path);
int mfs_workwith(char *path, int *fd, int *hasfs, struct myext2_super_block *sb,
                 struct myext2_group_descriptor gd[16], struct myext2_inode *root,
                 struct myext2_inode *workingdir);
int mfs_ls(int fd, const struct myext2_inode *workingdir);
void mfs_pwd(int fd, char **abs_path);

// TODO:
int mfs_mkdir(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd,
              struct myext2_inode *workingdir, char *name, int userID);
int mfs_rmdir(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd,
              struct myext2_inode *workingdir, char *name, int userID);
int mfs_cd(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd,
              struct myext2_inode *workingdir, char *name, int userID);
int mfs_mkfile(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd,
               struct myext2_inode *workingdir, char *name, int size, int userID);
int mfs_rmfile(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd,
               struct myext2_inode *workingdir, char *name, int userID);
int mfs_readfile(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd,
                 struct myext2_inode *workingdir, char *name, int userID);
int mfs_chown(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd,
              struct myext2_inode *workingdir, char *name, int src_userID, int des_userID);
int mfs_touch(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd,
              struct myext2_inode *workingdir, char *name);

const char *useage =
    " mkfs fspath\n"
    " workwith fspath\n"
    " ls\n"
    " cd dirname\n"
    " mkfile size filename\n"
    " mkdir dirname\n"
    " rmfile filename\n"
    " rmdir dirname\n"
    " pwd\n";

/**********************************************************
 *                         Common                         *
 * ********************************************************/
void setbit(void *buffer, unsigned int pos)
{
    char *ptr = (char *)buffer;
    *(ptr + (pos / 8)) |= 1 << (7 - (pos % 8));
}

void clearbit(void *buffer, unsigned int pos)
{
    char *ptr = (char *)buffer;
    *(ptr + (pos / 8)) &= ~(1 << (7 - (pos % 8)));
}
int testbit(void *buffer, unsigned int pos)
{
    char *ptr = (char *)buffer;
    int res = *(ptr + (pos / 8)) & 1 << (7 - (pos % 8));
    if (res == 0)
        return 0;
    else
        return 1;
}
int find_first_unsetbit(void *buffer, unsigned int length)
{
    int pos;
    for (pos = 0; pos < length; pos++)
    {
        if (!testbit(buffer, pos))
            return pos;
    }
    return -1;
}

/**********************************************************
 *                           FS                           *
 * ********************************************************/
// get inode by inode number
int get_inode(int fd, int inodenum, struct myext2_inode *inode)
{
    int retv;
    int group = inodenum / INODES_PER_GROUP;
    int offset = inodenum % INODES_PER_GROUP;
    int addr = group * BLOCK_SIZE * BLOCKS_PER_GROUP + 4 * BLOCK_SIZE + offset * sizeof(struct myext2_inode);
    retv = lseek(fd, addr, SEEK_SET);
    if (retv == -1)
    {
        fprintf(stderr, "Error: lseek %s\n", strerror(errno));
        return -1;
    }
    retv = read(fd, inode, sizeof(struct myext2_inode));
    if (retv == -1)
    {
        fprintf(stderr, "Error: read inode %d: %s\n", inodenum, strerror(errno));
        return -1;
    }
    return 0;
}
// write inode info to inode table for inode with #inodenum
int write_inode(int fd, int inodenum, void *buf)
{
    int retv;
    int group = inodenum / INODES_PER_GROUP;
    int offset = inodenum % INODES_PER_GROUP;
    int addr = (INODE_TABLE_OFFSET + group * BLOCKS_PER_GROUP) * BLOCK_SIZE +
               offset * sizeof(struct myext2_inode);
    retv = lseek(fd, addr, SEEK_SET);
    if (retv == -1)
    {
        fprintf(stderr, "Error: write_inode: %s", strerror(errno));
        return -1;
    }
    retv = write(fd, buf, sizeof(struct myext2_inode));
    if (retv == -1)
        return -1;
    else
        return 0;
}

// get a free inode, return the inodenumber, or on Erro -1
//        set the inode bitmap and DEC the free inode count in superblock and group descriptor.
// NOTE:
// This function write the corresponding inodebitmap to fs(DISK).
// This function will change the superblock and group descripror in MEMORY, BUT the real data in file system is left UNCHANGED.
// After this function called, make sure to save these two block cached in memory to disk.
int get_free_inode(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd)
{
    int i;
    int j;
    int retv;
    int offset;
    char buffer[BLOCK_SIZE];
    if (sb->s_free_inodes_count == 0)
    {
        fprintf(stderr, "No More Enough I node!\n");
        return -1;
    }

    for (i = 0; i < GROUPS_COUNT; i++)
    {
        if (gd[i].g_free_inode_count != 0)
        {
            retv = get_block(fd, i * BLOCKS_PER_GROUP + gd[i].g_inode_bitmap, buffer);
            if (retv == -1)
                return -1;
            offset = find_first_unsetbit(buffer, sb->s_inodes_per_group);
            setbit(buffer, offset);
            retv = write_block(fd, i * BLOCKS_PER_GROUP + gd[i].g_inode_bitmap, buffer);
            if (retv == -1)
                return -1;
            gd[i].g_free_inode_count -= 1;
            sb->s_free_inodes_count -= 1;
            return i * sb->s_inodes_per_group + offset;
        }
    }
}
// free a inode
//           clear the inode bitmap and INC the free inode count in superblock and group descriptor.
//           But not free the blocks pointed to by this indoe.
// NOTE:
// This function write the corresponding inodebitmap to fs.
// This function will change the superblock and group descripror in memory, The real data in file system is left unchanged.
// After this function called, make sure to save these two block cached in memory to disk.
int free_inode(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd, int inodenum)
{
    int retv;
    char buffer[BLOCK_SIZE];
    struct myext2_inode inode;
    retv = get_inode(fd, inodenum, &inode);
    if (retv == -1)
        return -1;

    // claer this inode' inode bitmap
    int group = inodenum / INODES_PER_GROUP;
    int offset = inodenum % INODES_PER_GROUP;
    int inode_bitmap = group * BLOCKS_PER_GROUP + gd[group].g_inode_bitmap;
    retv = get_block(fd, inode_bitmap, buffer);
    if (retv == -1)
        return -1;
    clearbit(buffer, offset);
    retv = write_block(fd, inode_bitmap, buffer);
    if (retv == -1)
        return -1;
    sb->s_inodes_count += 1;
    gd[group].g_free_inode_count += 1;
    return 0;
}
// get block by block number
int get_block(int fd, int blocknum, void *buf)
{
    int addr, retv;

    if (blocknum >= BLOCKS_COUNT)
    {
        fprintf(stderr, "get block INVALID BLOCKNUM!\n");
        return -1;
    }
    addr = blocknum * BLOCK_SIZE;

    retv = lseek(fd, addr, SEEK_SET);
    if (retv == -1)
    {
        fprintf(stderr, "Error: get block lseek %s\n", strerror(errno));
        return -1;
    }
    retv = read(fd, buf, BLOCK_SIZE);
    if (retv == -1)
    {
        fprintf(stderr, "Error: get block read %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
// write buf to block with #blocknum
int write_block(int fd, int blocknum, void *buf)
{
    int retv;
    int addr = blocknum * BLOCK_SIZE;
    retv = lseek(fd, addr, SEEK_SET);
    if (retv == -1)
        return -1;
    retv = write(fd, buf, BLOCK_SIZE);
    if (retv == -1)
        return -1;
    else
        return 0;
}
// get specific block with #blocknum in a inode
int get_block_in_inode(int fd, int blocknum,
                       const struct myext2_inode *inode, void *buf)
{
    int retv;
    int i, j;
    uint32_t indirect, doubly_indirect, triply_indirect;
    char *buffer[BLOCK_SIZE];

    if (blocknum >= ceil((double)inode->i_size / BLOCK_SIZE) || blocknum < 0)
    {
        fprintf(stderr, "get block in inode INVALID BLOCKNUM!\n");
        return -1;
    }
    else if (blocknum < 12) // direct
    {
        return get_block(fd, inode->i_data_blocks[blocknum], buf);
    }
    else if (blocknum < (12 + 256)) // indirect
    {
        retv = get_block(fd, inode->i_data_blocks[12], buffer);
        if (retv == -1)
            return -1;
        memcpy(&indirect, buffer + (blocknum - 12) * 4, 4);
        retv = get_block(fd, indirect, buf);
        if (retv == -1)
            return -1;
        else
            return 0;
    }
    else if (blocknum < (12 + 256 + 256 * 256)) // doubly indirect
    {
        retv = get_block(fd, inode->i_data_blocks[13], buffer);
        if (retv == -1)
            return -1;
        memcpy(&indirect, buffer + ((blocknum - 12 - 256) / 256) * 4, 4);
        retv = get_block(fd, indirect, buffer);
        if (retv == -1)
            return -1;
        memcpy(&doubly_indirect, buffer + ((blocknum - 12 - 256) % 256) * 4, 4);
        retv = get_block(fd, doubly_indirect, buf);
        if (retv == -1)
            return -1;
        else
            return 0;
    }
    else // triply indirect
    {
        retv = get_block(fd, inode->i_data_blocks[14], buffer);
        if (retv == -1)
            return -1;
        memcpy(&indirect, buffer + ((blocknum - 12 - 256 - 256 * 256) / (256 * 256)) * 4, 4);
        retv = get_block(fd, indirect, buffer);
        if (retv == -1)
            return -1;
        memcpy(&doubly_indirect, buffer + ((blocknum - 12 - 256 - 256 * 256) % (256 * 256)) * 4, 4);
        retv = get_block(fd, doubly_indirect, buffer);
        if (retv == -1)
            return -1;
        memcpy(&triply_indirect, buffer + ((blocknum - 12 - 256 - 256 * 256) % 256) * 4, 4);
        retv = get_block(fd, triply_indirect, buf);
        if (retv == -1)
            return -1;
        else
            return 0;
    }
}
// get an unused block, return the block number, or on Error -1;
int get_free_block(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd)
{
    int i;
    int retv;
    int offset;
    char buffer[BLOCK_SIZE];

    if (sb->s_free_blocks_count == 0)
    {
        fprintf(stderr, "No more free block\n");
        return -1;
    }
    for (i = 0; i < GROUPS_COUNT; i++)
    {
        if (gd[i].g_free_blocks_count != 0)
        {
            retv = get_block(fd, i * sb->s_blocks_per_group + gd[i].g_block_bitmap, buffer);
            if (retv == -1)
                return -1;
            offset = find_first_unsetbit(buffer, BLOCK_SIZE * 8);
            setbit(buffer, offset);
            retv = write_block(fd, i * sb->s_blocks_per_group + gd[i].g_block_bitmap, buffer);
            if (retv == -1)
                return -1;
            gd[i].g_free_blocks_count -= 1;
            sb->s_free_blocks_count -= 1;
            return i * BLOCKS_PER_GROUP + offset;
        }
    }
}
int free_block(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd, int blocknum)
{
    int retv;
    char buffer[BLOCK_SIZE];

    int group = blocknum / BLOCKS_PER_GROUP;
    int offset = blocknum % BLOCKS_PER_GROUP;
    int block_bitmap = group * BLOCKS_PER_GROUP + gd[group].g_block_bitmap;

    retv = get_block(fd, block_bitmap, buffer);
    if (retv == -1)
        return -1;
    clearbit(buffer, offset);
    retv = write_block(fd, block_bitmap, buffer);
    if (retv == -1)
        return -1;
    gd[group].g_free_blocks_count += 1;
    sb->s_free_blocks_count += 1;
    return 0;
}

int append_data_to_file(int fd, struct myext2_directory *sb,
                        struct myext2_group_descriptor *gd,
                        struct myext2_inode *inode,
                        int inodenum, void *buf, int size)
{
    int retv;
    if (inode == NULL)
    {
        inode = malloc(sizeof(struct myext2_inode));
        if (inode == NULL)
        {
            printf("append data to file malloc\n");
            return -1;
        }
        retv = get_inode(fd, inodenum, inode);
        if (retv == -1)
            return -1;
    }

    int filesize = inode->i_size;
    int blocks_count = ceil((double)size / BLOCK_SIZE);

    // Manage indirect pointer is sooooooooooooooooooooooo horrible ...
    //
    //
    //
    //
    //
    //
    //
    //
    //
}
/**********************************************************
 *                         Command                        *
 **********************************************************/
#define MKFS 0
#define WORKWITH 1
#define LS 2
#define CD 3
#define MKFILE 4
#define MKDIR 5
#define RMFILE 6
#define RMDIR 7
#define HELP 8
#define QUIT 9
#define PWD 10

int checkCmd(char *cmd, char *arg1, char *arg2, int *cmdtype)
{
    //      printf("%s,%s,%s\n",cmd,arg1,arg2);
    if (!strcmp(cmd, "mkfs"))
    {
        *cmdtype = MKFS;
        if (arg1)
            return 0;
    }
    else if (!strcmp(cmd, "workwith"))
    {
        *cmdtype = WORKWITH;
        if (arg1)
            return 0;
    }
    else if (!strcmp(cmd, "ls"))
    {
        *cmdtype = LS;
        return 0;
    }
    else if (!strcmp(cmd, "cd"))
    {
        *cmdtype = CD;
        if (arg1)
            return 0;
    }
    else if (!strcmp(cmd, "mkfile"))
    {
        *cmdtype = MKFILE;
        if (arg1 && arg2)
            return 0;
    }
    else if (!strcmp(cmd, "mkdir"))
    {
        *cmdtype = MKDIR;
        if (arg1)
            return 0;
    }
    else if (!strcmp(cmd, "rmfile"))
    {
        *cmdtype = RMFILE;
        if (arg1)
            return 0;
    }
    else if (!strcmp(cmd, "rmdir"))
    {
        *cmdtype = RMDIR;
        if (arg1)
            return 0;
    }
    else if (!strcmp(cmd, "pwd"))
    {
        *cmdtype = PWD;
        return 0;
    }
    else if (!strcmp(cmd, "help"))
    {
        *cmdtype = HELP;
        return 0;
    }
    else if (!strcmp(cmd, "quit"))
    {
        *cmdtype = QUIT;
        return 0;
    }
    return -1;
}

int mfs_mkfs(char *path)
{
    int fd, retv;
    int i, j;

    struct myext2_super_block sb;
    struct myext2_group_descriptor gd;
    struct myext2_inode inode;
    struct myext2_directory directory;

    char sb_buffer[BLOCK_SIZE];
    char gd_buffer[BLOCK_SIZE];
    char db_bitmap_buffer[BLOCK_SIZE];
    char buffer[BLOCK_SIZE];

    fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        fprintf(stderr, "Error: open file %s\n", strerror(errno));
        return -1;
    }

    /* super block */
    sb.s_block_size = BLOCK_SIZE;
    sb.s_blocks_count = BLOCKS_COUNT;
    sb.s_inodes_count = INODES_COUNT;
    sb.s_groups_count = GROUPS_COUNT;
    sb.s_blocks_per_group = BLOCKS_PER_GROUP;
    sb.s_inodes_per_group = INODES_PER_GROUP;
    // root directory use on block and one inode ...
    sb.s_free_blocks_count = sb.s_blocks_count - GROUPS_COUNT * BLOCKS_COUNT_USED_BY_EXT2_PER_GROUP - 1;
    sb.s_free_inodes_count = sb.s_inodes_count - 1;
    memset(sb_buffer, 0, BLOCK_SIZE);
    memcpy(sb_buffer, &sb, sizeof(sb));

    /* group descripter */
    memset(gd_buffer, 0, BLOCK_SIZE);
    gd.g_block_bitmap = 2;
    gd.g_inode_bitmap = 3;
    gd.g_inode_table = 4;
    gd.g_free_blocks_count = sb.s_blocks_per_group - BLOCKS_COUNT_USED_BY_EXT2_PER_GROUP - 1;
    gd.g_free_inode_count = sb.s_inodes_per_group - 1;
    memcpy(gd_buffer, &gd, sizeof(gd));
    gd.g_free_blocks_count = sb.s_blocks_per_group - BLOCKS_COUNT_USED_BY_EXT2_PER_GROUP;
    gd.g_free_inode_count = sb.s_inodes_per_group;
    for (i = 1; i < 16; i++)
        memcpy(gd_buffer + i * sizeof(gd), &gd, sizeof(gd));

    /* data block bitmap */
    memset(db_bitmap_buffer, 0, BLOCK_SIZE);
    for (i = 0; i < BLOCKS_COUNT_USED_BY_EXT2_PER_GROUP + 1; i++)
        setbit(db_bitmap_buffer, i);

    //  write first block group
    printf("Writing block group 0\n");
    // super block
    printf("Writing super block...\n");
    retv = write(fd, sb_buffer, sizeof(sb_buffer));
    if (retv != BLOCK_SIZE)
    {
        fprintf(stderr, "Error: write super block %s\n", strerror(errno));
        close(fd);
        unlink(path);
        return -1;
    }
    // group descriptor
    printf("Writing group descriptor...\n");
    retv = write(fd, gd_buffer, sizeof(gd_buffer));
    if (retv != BLOCK_SIZE)
    {
        fprintf(stderr, "Error: write super block %s\n", strerror(errno));
        close(fd);
        unlink(path);
        return -1;
    }
    // data block bitmap
    printf("Writing data block bitmap...\n");
    setbit(db_bitmap_buffer, BLOCKS_COUNT_USED_BY_EXT2_PER_GROUP);
    retv = write(fd, db_bitmap_buffer, sizeof(db_bitmap_buffer));
    if (retv != BLOCK_SIZE)
    {
        fprintf(stderr, "Error: write datablock bitmap %s\n", strerror(errno));
        close(fd);
        unlink(path);
        return -1;
    }
    // inode bitmap
    printf("Writing inode bitmap...\n");
    memset(buffer, 0, BLOCK_SIZE);
    setbit(buffer, 0);
    retv = write(fd, buffer, sizeof(buffer));
    if (retv != BLOCK_SIZE)
    {
        fprintf(stderr, "Error: write inode bitmap %s\n", strerror(errno));
        close(fd);
        unlink(path);
        return -1;
    }
    // inode table
    printf("Writing inode table...\n");
    memset(buffer, 0, BLOCK_SIZE);
    inode.i_num = 0;
    inode.i_access_time = time(NULL);
    inode.i_change_time = time(NULL);
    inode.i_modify_time = time(NULL);
    inode.i_uid = 0;
    inode.i_size = DIRECTORY_ENTRY_SIZE * 2;
    inode.i_mode = FILE_DIRECTORY;
    inode.i_data_blocks[0] = BLOCKS_COUNT_USED_BY_EXT2_PER_GROUP;
    memcpy(buffer, &inode, sizeof(inode));
    retv = write(fd, buffer, sizeof(buffer));
    if (retv != BLOCK_SIZE)
    {
        fprintf(stderr, "Error: write inode table %s\n", strerror(errno));
        close(fd);
        unlink(path);
        return -1;
    }
    memset(buffer, 0, BLOCK_SIZE);
    for (i = 1; i < 64; i++)
    {
        retv = write(fd, buffer, sizeof(buffer));
        if (retv != BLOCK_SIZE)
        {
            {
                fprintf(stderr, "Error: write inode table %s\n", strerror(errno));
                close(fd);
                unlink(path);
                return -1;
            }
        }
    }
    // datablock for root directory
    printf("Writing root directory's data block...\n");
    memset(buffer, 0, BLOCK_SIZE);
    directory.d_inode = 0;
    strcpy(directory.d_name, ".");
    memcpy(buffer, &directory, sizeof(directory));
    strcpy(directory.d_name, "..");
    memcpy(buffer + sizeof(directory), &directory, sizeof(directory));
    retv = write(fd, buffer, sizeof(buffer));
    if (retv != BLOCK_SIZE)
    {
        fprintf(stderr, "Error: write directory entry for root data block:%s\n", strerror(errno));
        close(fd);
        unlink(path);
        return -1;
    }
    // other datablock
    printf("Writing block group 1-15 ...\n");
    memset(buffer, 0, BLOCK_SIZE);
    for (i = 0; i < BLOCKS_PER_GROUP - BLOCKS_COUNT_USED_BY_EXT2_PER_GROUP - 1; i++)
    {
        retv = write(fd, buffer, sizeof(buffer));
        if (retv != BLOCK_SIZE)
        {
            fprintf(stderr, "Error: write data block:%s\n", strerror(errno));
            close(fd);
            unlink(path);
            return -1;
        }
    }

    // block group 1 - 15
    memset(buffer, 0, BLOCK_SIZE);
    clearbit(db_bitmap_buffer, BLOCKS_COUNT_USED_BY_EXT2_PER_GROUP);
    for (i = 1; i < GROUPS_COUNT; i++)
    {
        // super block
        retv = write(fd, sb_buffer, sizeof(sb_buffer));
        if (retv != BLOCK_SIZE)
        {
            fprintf(stderr, "Error: write super block %s\n", strerror(errno));
            close(fd);
            unlink(path);
            return -1;
        }
        // group descriptor
        retv = write(fd, gd_buffer, sizeof(gd_buffer));
        if (retv != BLOCK_SIZE)
        {
            fprintf(stderr, "Error: write group descriptor %s\n", strerror(errno));
            close(fd);
            unlink(path);
            return -1;
        }
        // datablock bitmap
        retv = write(fd, db_bitmap_buffer, sizeof(db_bitmap_buffer));
        if (retv != BLOCK_SIZE)
        {
            fprintf(stderr, "Error: write datablock bitmap %s\n", strerror(errno));
            close(fd);
            unlink(path);
            return -1;
        }
        // 0 for other
        for (j = 0; j < BLOCKS_PER_GROUP - 3; j++)
        {
            retv = write(fd, buffer, sizeof(gd_buffer));
            if (retv != BLOCK_SIZE)
            {
                fprintf(stderr, "Error: write zero %s\n", strerror(errno));
                close(fd);
                unlink(path);
                return -1;
            }
        }
    }

    printf("Make file system success, the file is on ./%s\n", path);
    printf("Type \"workwith filepath\" to load the file system\n");
    return 0;
}

int mfs_workwith(char *path, int *fd, int *hasfs, struct myext2_super_block *sb,
                 struct myext2_group_descriptor gd[16], struct myext2_inode *root,
                 struct myext2_inode *workingdir)
{
    int retv;
    char buffer[BLOCK_SIZE];

    *fd = open(path, O_RDWR);
    if (*fd == -1)
    {
        fprintf(stderr, "Error: open file: %s\n", strerror(errno));
        *hasfs = 0;
        return -1;
    }

    retv = read(*fd, sb, BLOCK_SIZE);
    if (retv != BLOCK_SIZE)
    {
        fprintf(stderr, "Error: Read super block %s\n", strerror(errno));
        close(*fd);
        *hasfs = 0;
        return -1;
    }

    retv = read(*fd, buffer, BLOCK_SIZE);
    if (retv != BLOCK_SIZE)
    {
        fprintf(stderr, "Error: Read group descriptor's block:%s\n", strerror(errno));
        close(*fd);
        *hasfs = 0;
        return -1;
    }
    memcpy(gd, buffer, sizeof(struct myext2_group_descriptor) * GROUPS_COUNT);

    retv = get_inode(*fd, 0, root);
    if (retv == -1)
    {
        close(*fd);
        *hasfs = 0;
        return -1;
    }
    memcpy(workingdir, root, sizeof(struct myext2_inode));
    *hasfs = 1;
    return 0;
}

int mfs_ls(int fd, const struct myext2_inode *workingdir)
{
    int i;
    int retv;
    char *buffer;
    struct myext2_directory dir;
    struct myext2_inode inode;

    int entry_count = ceil((double)workingdir->i_size / DIRECTORY_ENTRY_SIZE);
    int block_count = ceil((double)workingdir->i_size / BLOCK_SIZE);
    buffer = malloc(block_count * BLOCK_SIZE);
    for (i = 0; i < block_count; i++)
    {
        retv = get_block_in_inode(fd, i, workingdir, buffer + i * BLOCK_SIZE);
        if (retv == -1)
            return -1;
    }
    for (i = 0; i < entry_count; i++)
    {
        memcpy(&dir, buffer + i * sizeof(struct myext2_directory), sizeof(struct myext2_directory));
        retv = get_inode(fd, dir.d_inode, &inode);
        if (retv == -1)
            continue;
        time_t t = inode.i_modify_time;
        printf("Name: [%s]  Inode:[%d]  Type:[%s] Size:[%d] Owner:[%d] MTIME:%s",
               dir.d_name, dir.d_inode, typeoffile(inode.i_mode),
               inode.i_size, inode.i_uid, ctime(&t));
    }
}

void mfs_pwd(int fd, char **abs_path)
{
    int i = 0;
    char *cur = abs_path[i];
    if (!cur)
        printf("/");
    else
    {
        do
        {
            printf("/%s", cur);
            i += 1;
            cur = abs_path[i];
        } while (cur);
    }
    printf("\n");
}

int mfs_mkdir(int fd, struct myext2_super_block *sb, struct myext2_group_descriptor *gd,
              struct myext2_inode *workingdir, char *name, int userID)
{
    int retv;
    int inode_num;
    int block_num;
    char buffer[BLOCK_SIZE];
    struct myext2_directory directory;
    struct myext2_inode inode;

    memset(&inode, 0, sizeof(inode));
    inode_num = get_free_inode(fd, sb, gd);
    if (inode_num == -1)
        return -1;
    block_num = get_free_block(fd, sb, gd);
    if (block_num == -1)
        goto err_free_inode;
    inode.i_access_time = time(NULL);
    inode.i_change_time = time(NULL);
    inode.i_modify_time = time(NULL);
    inode.i_uid = userID;
    inode.i_mode = FILE_DIRECTORY;
    inode.i_size = DIRECTORY_ENTRY_SIZE * 2;
    inode.i_data_blocks[0] = block_num;
    retv = write_inode(fd, inode_num, &inode);
    if (retv == -1)
        goto err_free_block_and_inode;

    memset(buffer, 0, BLOCK_SIZE);
    memset(&directory, 0, sizeof(struct myext2_directory));
    directory.d_inode = inode_num;
    strcpy(directory.d_name, ".");
    memcpy(buffer, &directory, sizeof(struct myext2_directory));
    directory.d_inode = workingdir->i_num;
    strcpy(directory.d_name, "..");
    memcpy(buffer, &directory + sizeof(struct myext2_directory), sizeof(struct myext2_directory));
    retv = write_block(fd, block_num, buffer);
    if (retv == -1)
        goto err_free_block_and_inode;

    // add a directory entry in workingdir's data
    //
    //  stuck.........append_data_to_file is horrible
    //
    //
    //
    //
    //

    return 0;

err_free_block_and_inode:
    free_block(fd, sb, gd, block_num);
err_free_inode:
    free_inode(fd, sb, gd, inode_num);
    return -1;
}
/**********************************************************
 *                          Main                          *
 * ********************************************************/
int main()
{
    int userID;

    int fd;
    int hasfs;
    struct myext2_super_block sb;
    struct myext2_group_descriptor gd[GROUPS_COUNT];
    struct myext2_inode root, workingdir;

    char abs_path[128][128];
    int path_level;

    char cmd[4096];
    char *arg1, *arg2;
    int cmdtype, retv;

    memset(abs_path, 0, sizeof(abs_path));
    path_level = 0;
    printf("Input you User ID: (Integer between 0 and 65535)\n");
    fgets(cmd, 4096, stdin);
    userID = atoi(cmd);
    printf("Welcome, your ID is %d (0 for super user)\n", userID);
    printf("Use workwith to load a file system or mkfs to creat a new file system\n");

    printf(">>> ");
    fflush(stdout);
    while (fgets(cmd, 4096, stdin) != NULL)
    {
        strtok(cmd, " \t\n");
        arg1 = strtok(NULL, " \t\n");
        arg2 = strtok(NULL, " \t\n");

        if (checkCmd(cmd, arg1, arg2, &cmdtype) != 0)
        {
            fprintf(stderr, "Bad command!\n>>> ");
            continue;
        }

        if (!hasfs && cmdtype != MKFS && cmdtype != WORKWITH && cmdtype != QUIT && cmdtype != HELP)
        {
            fprintf(stderr, "No file system work with.\n");
            fprintf(stderr, "Use workwith to load a file system or mkfs to creat a new file system\n>>> ");
            continue;
        }

        switch (cmdtype)
        {
        case HELP:
            printf("%s", useage);
            break;
        case MKFS:
            mfs_mkfs(arg1);
            break;
        case WORKWITH:
            mfs_workwith(arg1, &fd, &hasfs, &sb, &gd, &root, &workingdir);
            break;
        case LS:
            mfs_ls(fd, &root);
            break;
        case PWD:
            mfs_pwd(fd, abs_path);
            break;
        case QUIT:
            exit(0);
            break;
        }

        printf(">>> ");
        fflush(stdout);
    }
}