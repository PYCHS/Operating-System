#include <linux/fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "osfs.h"

/**
 * osfs_lookup - Find a file in a directory
 *
 * This is called when the VFS needs to find an inode for a given filename
 * (e.g., when you type "ls" or "open").
 */
// read data block, for iterate all the filename, osfs_iget and d_splice_alias return
static struct dentry *osfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
    struct osfs_sb_info *sb_info = dir->i_sb->s_fs_info;
    struct osfs_inode *parent_inode = dir->i_private;
    void *dir_data_block;
    struct osfs_dir_entry *dir_entries;
    int dir_entry_count;
    int i;
    struct inode *inode = NULL;

    pr_info("osfs_lookup: Looking up '%.*s' in inode %lu\n",
            (int)dentry->d_name.len, dentry->d_name.name, dir->i_ino);

    // Get pointer to the parent directory's data block
    // In our simple design, a directory only uses one block
    dir_data_block = sb_info->data_blocks + parent_inode->i_block * BLOCK_SIZE;

    // Treat the data block as an array of directory entries
    dir_entry_count = parent_inode->i_size / sizeof(struct osfs_dir_entry);
    dir_entries = (struct osfs_dir_entry *)dir_data_block;

    // Scan linear array to find the filename
    for (i = 0; i < dir_entry_count; i++) {
        if (strlen(dir_entries[i].filename) == dentry->d_name.len &&
            strncmp(dir_entries[i].filename, dentry->d_name.name, dentry->d_name.len) == 0) {
            
            // Found it! Retrieve the corresponding inode from disk (memory)
            inode = osfs_iget(dir->i_sb, dir_entries[i].inode_no);
            if (IS_ERR(inode)) {
                pr_err("osfs_lookup: Error getting inode %u\n", dir_entries[i].inode_no);
                return ERR_CAST(inode);
            }
            // Return the dentry linked to the inode we just found
            return d_splice_alias(inode, dentry);
        }
    }

    // File not found is NOT an error here; returning NULL tells VFS it's a negative dentry
    return NULL;
}

/**
 * osfs_iterate - Read directory contents (for 'ls')
 */
// dir_emit to show entries
static int osfs_iterate(struct file *filp, struct dir_context *ctx)
{
    struct inode *inode = file_inode(filp);
    struct osfs_sb_info *sb_info = inode->i_sb->s_fs_info;
    struct osfs_inode *osfs_inode = inode->i_private;
    void *dir_data_block;
    struct osfs_dir_entry *dir_entries;
    int dir_entry_count;
    int i;

    // Handle the standard "." and ".." entries first
    if (ctx->pos == 0) {
        if (!dir_emit_dots(filp, ctx))
            return 0;
    }

    // Map the directory's data block to memory
    dir_data_block = sb_info->data_blocks + osfs_inode->i_block * BLOCK_SIZE;
    dir_entry_count = osfs_inode->i_size / sizeof(struct osfs_dir_entry);
    dir_entries = (struct osfs_dir_entry *)dir_data_block;

    /* Adjust index because ctx->pos includes . and .. */
    i = ctx->pos - 2;

    // Loop through our stored entries and report them to userspace
    for (; i < dir_entry_count; i++) {
        struct osfs_dir_entry *entry = &dir_entries[i];
        unsigned int type = DT_UNKNOWN;

        // dir_emit is the VFS function to send data to 'ls'
        if (!dir_emit(ctx, entry->filename, strlen(entry->filename), entry->inode_no, type)) {
            pr_err("osfs_iterate: dir_emit failed for entry '%s'\n", entry->filename);
            return -EINVAL;
        }

        ctx->pos++;
    }

    return 0;
}

/**
 * osfs_new_inode - Allocate and initialize a new inode
 * * Used by osfs_create. This handles the low-level resource allocation:
 * checking bitmaps, grabbing a free inode, and setting up defaults.
 */
// allowcate spaces
struct inode *osfs_new_inode(const struct inode *dir, umode_t mode)
{
    struct super_block *sb = dir->i_sb;
    struct osfs_sb_info *sb_info = sb->s_fs_info;
    struct inode *inode;
    struct osfs_inode *osfs_inode;
    int ino, ret;

    /* Check if the mode is supported */
    if (!S_ISDIR(mode) && !S_ISREG(mode) && !S_ISLNK(mode)) {
        pr_err("File type not supported (only directory, regular file and symlink supported)\n");
        return ERR_PTR(-EINVAL);
    }

    /* Check resources in Superblock */
    if (sb_info->nr_free_inodes == 0 || sb_info->nr_free_blocks == 0)
        return ERR_PTR(-ENOSPC);

    /* 1. Allocate a free inode number from the bitmap */
    ino = osfs_get_free_inode(sb_info);
    if (ino < 0 || ino >= sb_info->inode_count)
        return ERR_PTR(-ENOSPC);

    /* 2. Allocate the VFS inode structure */
    inode = new_inode(sb);
    if (!inode)
        return ERR_PTR(-ENOMEM);

    /* 3. Initialize VFS inode (owner, permissions, timestamps) */
    inode_init_owner(&nop_mnt_idmap, inode, dir, mode);
    inode->i_ino = ino;
    inode->i_sb = sb;
    inode->i_blocks = 0;
    simple_inode_init_ts(inode);

    /* Set operation handlers based on file type */
    if (S_ISDIR(mode)) {
        inode->i_op = &osfs_dir_inode_operations;
        inode->i_fop = &osfs_dir_operations;
        set_nlink(inode, 2); /* . and .. */
        inode->i_size = 0;
    } else if (S_ISREG(mode)) {
        inode->i_op = &osfs_file_inode_operations;
        inode->i_fop = &osfs_file_operations;
        set_nlink(inode, 1);
        inode->i_size = 0;
    } else if (S_ISLNK(mode)) {
        // inode->i_op = &osfs_symlink_inode_operations;
        set_nlink(inode, 1);
        inode->i_size = 0;
    }

    /* 4. Initialize our custom on-disk inode (osfs_inode) */
    osfs_inode = osfs_get_osfs_inode(sb, ino);
    if (!osfs_inode) {
        pr_err("osfs_new_inode: Failed to get osfs_inode for inode %d\n", ino);
        iput(inode);
        return ERR_PTR(-EIO);
    }
    memset(osfs_inode, 0, sizeof(*osfs_inode));

    osfs_inode->i_ino = ino;
    osfs_inode->i_mode = inode->i_mode;
    osfs_inode->i_uid = i_uid_read(inode);
    osfs_inode->i_gid = i_gid_read(inode);
    osfs_inode->i_size = inode->i_size;
    osfs_inode->i_blocks = 1; // Start with 1 block allocated
    osfs_inode->__i_atime = osfs_inode->__i_mtime = osfs_inode->__i_ctime = current_time(inode);
    
    // Link VFS inode private data to our internal inode
    inode->i_private = osfs_inode;

    /* 5. Pre-allocate the first data block for this file */
    ret = osfs_alloc_data_block(sb_info, &osfs_inode->i_block);
    if (ret) {
        pr_err("osfs_new_inode: Failed to allocate data block\n");
        iput(inode);
        return ERR_PTR(ret);
    }

    /* Update global counters */
    sb_info->nr_free_inodes--;

    /* Mark inode as dirty so VFS knows it needs syncing */
    mark_inode_dirty(inode);

    return inode;
}

/**
 * osfs_add_dir_entry - Add a new file to the parent directory
 */
// add entries into dirs
static int osfs_add_dir_entry(struct inode *dir, uint32_t inode_no, const char *name, size_t name_len)
{
    struct osfs_sb_info *sb_info = dir->i_sb->s_fs_info;
    struct osfs_inode *parent_inode = dir->i_private;
    void *dir_data_block;
    struct osfs_dir_entry *dir_entries;
    int dir_entry_count;
    int i;

    // Access parent directory's data
    dir_data_block = sb_info->data_blocks + parent_inode->i_block * BLOCK_SIZE;

    // Calculate current entry count based on file size
    dir_entry_count = parent_inode->i_size / sizeof(struct osfs_dir_entry);
    
    // Hard limit check (since we only support 1 block per dir)
    if (dir_entry_count >= MAX_DIR_ENTRIES) {
        pr_err("osfs_add_dir_entry: Parent directory is full\n");
        return -ENOSPC;
    }

    dir_entries = (struct osfs_dir_entry *)dir_data_block;

    // Check for duplicates before adding
    for (i = 0; i < dir_entry_count; i++) {
        if (strlen(dir_entries[i].filename) == name_len &&
            strncmp(dir_entries[i].filename, name, name_len) == 0) {
            pr_warn("osfs_add_dir_entry: File '%.*s' already exists\n", (int)name_len, name);
            return -EEXIST;
        }
    }

    // Append the new entry at the end
    strncpy(dir_entries[dir_entry_count].filename, name, name_len);
    dir_entries[dir_entry_count].filename[name_len] = '\0';
    dir_entries[dir_entry_count].inode_no = inode_no;

    // Increase parent directory size
    parent_inode->i_size += sizeof(struct osfs_dir_entry);

    return 0;
}


/**
 * osfs_create - Entry point for creating files (e.g. 'touch')
 */
// 1.create Inode(osfs_add_dir_entry), 2.add filename into parent dir, 3.link dentry and new inode(d_instantiate)
static int osfs_create(struct mnt_idmap *idmap,
                       struct inode *dir,
                       struct dentry *dentry,
                       umode_t mode,
                       bool excl)
{
    struct inode *inode;
    struct osfs_sb_info *sb_info = dir->i_sb->s_fs_info;
    struct osfs_inode *parent_inode = dir->i_private;
    struct osfs_dir_entry *dir_entries;
    int ret;

    /* Sanity check on filename length */
    if (dentry->d_name.len >= MAX_FILENAME_LEN)
        return -ENAMETOOLONG;

    /* 1. Create and init the new inode */
    inode = osfs_new_inode(dir, mode);
    if (IS_ERR(inode))
        return PTR_ERR(inode);

    /* 2. Register the new file in the parent directory */
    ret = osfs_add_dir_entry(dir,
                             inode->i_ino,
                             dentry->d_name.name,
                             dentry->d_name.len);
    if (ret) {
        // Clean up if we failed to add the entry
        iput(inode);
        return ret;
    }

    /* 3. Link the dentry (VFS name cache) to the inode */
    d_instantiate(dentry, inode);

    /* Update parent timestamps since we modified it */
    inode_set_mtime_to_ts(dir, current_time(dir));
    inode_set_ctime_to_ts(dir, current_time(dir));

    /* Mark dirty */
    mark_inode_dirty(dir);
    mark_inode_dirty(inode);

    return 0;
}


const struct inode_operations osfs_dir_inode_operations = {
    .lookup = osfs_lookup,
    .create = osfs_create,
    // Add other operations as needed
};

const struct file_operations osfs_dir_operations = {
    .iterate_shared = osfs_iterate,
    .llseek = generic_file_llseek,
    // Add other operations as needed
};
