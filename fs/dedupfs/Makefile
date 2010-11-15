#
# Makefile for the linux dedupfs-filesystem routines.
#

obj-$(CONFIG_DEDUPFS) += dedupfs.o

dedupfs-y := 	balloc.o bitmap.o dir.o file.o fsync.o ialloc.o inode.o \
	   	ioctl.o namei.o super.o symlink.o hash.o resize.o ext3_jbd.o

dedupfs-$(CONFIG_DEDUPFS_XATTR)	 	+= xattr.o xattr_user.o xattr_trusted.o
dedupfs-$(CONFIG_DEDUPFS_POSIX_ACL) 	+= acl.o
dedupfs-$(CONFIG_DEDUPFS_SECURITY)	+= xattr_security.o
