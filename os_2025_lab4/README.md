# Memory-Based Virtual File System (Lab 4)

An implementation of a simple in-memory file system (`osfs`) for the Linux Kernel. This project explores the Virtual File System (VFS) architecture, inode management, and custom file allocation strategies within the Linux kernel.

## Features

This project implements a functional file system with the following capabilities:

### 1. File System Registration
* **VFS Integration**: Registers the `osfs` file system type with the kernel using `register_filesystem`.
* **Superblock Management**: Implements `osfs_fill_super` to initialize filesystem metadata, bitmaps, and the root inode.

### 2. Basic Operations (Requirements)
* **File Creation**: Implements `osfs_create` and `osfs_new_inode` to allocate inodes and link dentry objects.
* **File Writing**: Implements `osfs_write` using `copy_from_user` to transfer data from user space to kernel memory blocks.
* **Directory Management**: Supports adding directory entries via `osfs_add_dir_entry`.

### 3. Advanced Allocation Strategy (Bonus)
* **Extent-Based Allocation**: Modified the standard single-block pointer to an **Extent-based** structure.
    * Supports files larger than a single block (4KB) by allocating multiple contiguous blocks.
    * **Structure**: Uses `struct extent` to track `start_block`, `block_count`, and `file_offset`.
    * **Logic**: The write function intelligently handles data spanning across multiple extents.

---

## Project Structure

```text
.
├── dir.c           # Directory operations (Create, Lookup, Link)
├── file.c          # File operations (Read, Write with Extent logic)
├── inode.c         # Inode management & Data block allocation
├── super.c         # Superblock operations & Mount logic
├── osfs.h          # Data structures (superblock, inode, extent definitions)
├── osfs_init.c     # Module initialization & registration
└── Makefile        # Build script for Kernel Module
```
## Compilation & Usage
### Prerequisites
* GCC Compiler
* Make tool
* Linux Headers (for Kernel Module compilation)
* Install: sudo apt install linux-headers-$(uname -r)

### 1. Compile and Install
Compile the kernel module and insert it into the kernel:

```Bash
# 1. Compile the module
make

# 2. Insert the module
sudo insmod osfs.ko

# 3. Verify installation
sudo dmesg | tail
# Output: osfs: Successfully registered
```
### 2. Mount the File System
Create a mount point and mount the filesystem:

```Bash
mkdir mnt
sudo mount -t osfs none mnt/
```
### 3. Clean up
Always unmount and remove the module after testing:

```Bash
sudo umount mnt
sudo rmmod osfs
make clean
```
## Testing Examples
### Basic Requirement: Create & Write
Creating a file and writing a short string (fitting in one block):

```Bash
cd mnt
sudo touch test1.txt
sudo bash -c "echo 'I LOVE OSLAB' > test1.txt"
cat test1.txt
# Output: I LOVE OSLAB
```
### Bonus Requirement: Extent-based Allocation
Writing a file larger than 4KB to demonstrate multi-block allocation support:

```Bash
# Write 10KB of data (spanning multiple blocks)
sudo dd if=/dev/zero of=largefile bs=1024 count=10

# Verify file size
ls -lh largefile
# Output: -rw-r--r-- 1 root root 10K ... largefile
```
* Note: Without the Extent-based implementation, the file size would be limited to a single block (4KB).