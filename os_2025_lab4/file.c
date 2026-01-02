#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include "osfs.h"

// Helper: Calculate how many blocks are needed to hold 'size' bytes
static inline u32 blocks_needed_for_size(loff_t size)
{
    if (size <= 0) return 0;
    return (u32)((size + BLOCK_SIZE - 1) / BLOCK_SIZE);
}

/**
 * osfs_read - Handle read operations on a file
 */
ssize_t osfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    struct osfs_inode *oi = osfs_get_osfs_inode(inode->i_sb, inode->i_ino);
    struct osfs_sb_info *sb_info = inode->i_sb->s_fs_info;
    loff_t pos = *ppos;
    size_t to_read, done = 0;

    if (!oi) return -EFAULT;
    
    // Check if we are trying to read past the end of the file
    if (pos >= oi->i_size) return 0;

    // specific read length: don't read past EOF
    to_read = min_t(size_t, len, (size_t)(oi->i_size - pos));

    while (done < to_read) {
        // Calculate which block index and offset within that block we are at
        u32 blk_off = (u32)(pos / BLOCK_SIZE);
        u32 in_blk  = (u32)(pos % BLOCK_SIZE);
        u32 blkno;
        size_t chunk;
        void *kaddr;

        if (blk_off >= oi->i_blocks)
            break;

        // Map logical block to physical block
        // Since we use extent-based allocation, this is just start_block + offset
        blkno = oi->i_block + blk_off;
        
        // Get the kernel virtual address for this data block
        kaddr = sb_info->data_blocks + (blkno * BLOCK_SIZE);

        // Determine how much we can copy in this iteration (limited by block boundary)
        chunk = min_t(size_t, to_read - done, (size_t)(BLOCK_SIZE - in_blk));

        // Copy data from kernel space to user space buffer
        if (copy_to_user(buf + done, kaddr + in_blk, chunk))
            return -EFAULT;

        done += chunk;
        pos  += chunk;
    }

    // Update file position and access time
    *ppos = pos;
    oi->__i_atime = current_time(inode);   
    return (ssize_t)done;
}

/**
 * osfs_write - Handle write operations (includes extent allocation logic)
 */
ssize_t osfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    struct osfs_inode *oi = osfs_get_osfs_inode(inode->i_sb, inode->i_ino);
    struct osfs_sb_info *sb_info = inode->i_sb->s_fs_info;

    loff_t pos = *ppos;
    loff_t end = pos + (loff_t)len;
    u32 need_blocks;
    size_t done = 0;

    if (!oi) return -EFAULT;
    if (len == 0) return 0;

    // Calculate total blocks needed to cover the new file size
    need_blocks = blocks_needed_for_size(end);

    // Case 1: First write to a new/empty file
    if (oi->i_blocks == 0) {
        u32 start;
        // Try to allocate a contiguous chunk (extent) of blocks
        int ret = osfs_alloc_extent(sb_info, need_blocks, &start);
        if (ret) return ret;
        
        oi->i_block  = start;
        oi->i_blocks = need_blocks;

    } 
    // Case 2: File is growing, need to extend it
    else if (need_blocks > oi->i_blocks) {
        u32 old = oi->i_blocks;
        u32 add = need_blocks - old;
        u32 tail = oi->i_block + old; // The block index immediately following current data
        u32 j;

        // Sanity check: ensure we don't go out of physical disk bounds
        if (tail + add > sb_info->block_count)
            return -ENOSPC;

        // Check if the NEXT blocks are free (Contiguous check)
        // We enforce strict continuity here for the extent-based bonus
        for (j = 0; j < add; j++)
            if (test_bit(tail + j, sb_info->block_bitmap))
                return -ENOSPC; // Blocks are taken, cannot extend contiguously

        // Mark them as used
        for (j = 0; j < add; j++)
            set_bit(tail + j, sb_info->block_bitmap);

        sb_info->nr_free_blocks -= add;
        oi->i_blocks = need_blocks;
    }

    // Write loop: Copy data from user buffer to kernel blocks
    while (done < len) {
        u32 blk_off = (u32)(pos / BLOCK_SIZE);
        u32 in_blk  = (u32)(pos % BLOCK_SIZE);
        u32 blkno;
        size_t chunk;
        void *kaddr;

        if (blk_off >= oi->i_blocks)
            break;

        blkno = oi->i_block + blk_off;
        kaddr = sb_info->data_blocks + (blkno * BLOCK_SIZE);

        chunk = min_t(size_t, len - done, (size_t)(BLOCK_SIZE - in_blk));

        // Copy data from user space to the mapped kernel address
        if (copy_from_user(kaddr + in_blk, buf + done, chunk))
            return -EFAULT;

        done += chunk;
        pos  += chunk;
    }

    // Update file pointer
    *ppos = pos;

    // Update file size if we wrote past the old EOF
    if (pos > oi->i_size)
        oi->i_size = pos;

    // Update modification timestamps
    oi->__i_mtime = oi->__i_ctime = current_time(inode);

    return (ssize_t)done;
}

const struct file_operations osfs_file_operations = {
    .open   = generic_file_open,
    .read   = osfs_read,
    .write  = osfs_write,
    .llseek = default_llseek,
};

const struct inode_operations osfs_file_inode_operations = {
};
