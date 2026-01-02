savedcmd_osfs.mod := printf '%s\n'   super.o inode.o file.o dir.o osfs_init.o | awk '!x[$$0]++ { print("./"$$0) }' > osfs.mod
