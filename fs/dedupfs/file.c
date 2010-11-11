/*
 *  linux/fs/dedupfs/file.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/file.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  dedupfs fs regular file handling primitives
 *
 *  64-bit file support on 64-bit platforms by Jakub Jelinek
 * 	(jj@sunsite.ms.mff.cuni.cz)
 */

#include <linux/time.h>
#include <linux/pagemap.h>
#include <linux/quotaops.h>
#include "dedupfs.h"
#include "xattr.h"
#include "acl.h"

/*
 * Called when filp is released. This happens when all file descriptors
 * for a single struct file are closed. Note that different open() calls
 * for the same file yield different struct file structures.
 */
static int dedupfs_release_file (struct inode * inode, struct file * filp)
{
	if (filp->f_mode & FMODE_WRITE) {
		mutex_lock(&DEDUPFS_I(inode)->truncate_mutex);
		dedupfs_discard_reservation(inode);
		mutex_unlock(&DEDUPFS_I(inode)->truncate_mutex);
	}
	return 0;
}

int dedupfs_fsync(struct file *file, int datasync)
{
	int ret;
	struct super_block *sb = file->f_mapping->host->i_sb;
	struct address_space *mapping = sb->s_bdev->bd_inode->i_mapping;

	ret = generic_file_fsync(file, datasync);
	if (ret == -EIO || test_and_clear_bit(AS_EIO, &mapping->flags)) {
		/* We don't really know where the IO error happened... */
		dedupfs_error(sb, __func__,
			   "detected IO error when writing metadata buffers");
		ret = -EIO;
	}
	return ret;
}

/*
 * We have mostly NULL's here: the current defaults are ok for
 * the dedupfs filesystem.
 */
const struct file_operations dedupfs_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.write		= do_sync_write,
	.aio_read	= generic_file_aio_read,
	.aio_write	= generic_file_aio_write,
	.unlocked_ioctl = dedupfs_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= dedupfs_compat_ioctl,
#endif
	.mmap		= generic_file_mmap,
	.open		= dquot_file_open,
	.release	= dedupfs_release_file,
	.fsync		= dedupfs_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= generic_file_splice_write,
};

#ifdef CONFIG_DEDUPFS_XIP
const struct file_operations dedupfs_xip_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= xip_file_read,
	.write		= xip_file_write,
	.unlocked_ioctl = dedupfs_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= dedupfs_compat_ioctl,
#endif
	.mmap		= xip_file_mmap,
	.open		= dquot_file_open,
	.release	= dedupfs_release_file,
	.fsync		= dedupfs_fsync,
};
#endif

const struct inode_operations dedupfs_file_inode_operations = {
#ifdef CONFIG_DEDUPFS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= dedupfs_listxattr,
	.removexattr	= generic_removexattr,
#endif
	.setattr	= dedupfs_setattr,
	.check_acl	= dedupfs_check_acl,
	.fiemap		= dedupfs_fiemap,
};
