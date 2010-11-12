/*
 * linux/include/linux/dedupfs_jbd.h
 *
 * Written by Stephen C. Tweedie <sct@redhat.com>, 1999
 *
 * Copyright 1998--1999 Red Hat corp --- All Rights Reserved
 *
 * This file is part of the Linux kernel and is made available under
 * the terms of the GNU General Public License, version 2, or at your
 * option, any later version, incorporated herein by reference.
 *
 * Ext3-specific journaling extensions.
 */

#ifndef _LINUX_DEDUPFS_JBD_H
#define _LINUX_DEDUPFS_JBD_H

#include <linux/fs.h>
#include <linux/jbd.h>
#include "dedupfs.h"

#define DEDUPFS_JOURNAL(inode)	(DEDUPFS_SB((inode)->i_sb)->s_journal)

/* Define the number of blocks we need to account to a transaction to
 * modify one block of data.
 *
 * We may have to touch one inode, one bitmap buffer, up to three
 * indirection blocks, the group and superblock summaries, and the data
 * block to complete the transaction.  */

#define DEDUPFS_SINGLEDATA_TRANS_BLOCKS	8U

/* Extended attribute operations touch at most two data buffers,
 * two bitmap buffers, and two group summaries, in addition to the inode
 * and the superblock, which are already accounted for. */

#define DEDUPFS_XATTR_TRANS_BLOCKS		6U

/* Define the minimum size for a transaction which modifies data.  This
 * needs to take into account the fact that we may end up modifying two
 * quota files too (one for the group, one for the user quota).  The
 * superblock only gets updated once, of course, so don't bother
 * counting that again for the quota updates. */

#define DEDUPFS_DATA_TRANS_BLOCKS(sb)	(DEDUPFS_SINGLEDATA_TRANS_BLOCKS + \
					 DEDUPFS_XATTR_TRANS_BLOCKS - 2 + \
					 DEDUPFS_MAXQUOTAS_TRANS_BLOCKS(sb))

/* Delete operations potentially hit one directory's namespace plus an
 * entire inode, plus arbitrary amounts of bitmap/indirection data.  Be
 * generous.  We can grow the delete transaction later if necessary. */

#define DEDUPFS_DELETE_TRANS_BLOCKS(sb)   (DEDUPFS_MAXQUOTAS_TRANS_BLOCKS(sb) + 64)

/* Define an arbitrary limit for the amount of data we will anticipate
 * writing to any given transaction.  For unbounded transactions such as
 * write(2) and truncate(2) we can write more than this, but we always
 * start off at the maximum transaction size and grow the transaction
 * optimistically as we go. */

#define DEDUPFS_MAX_TRANS_DATA		64U

/* We break up a large truncate or write transaction once the handle's
 * buffer credits gets this low, we need either to extend the
 * transaction or to start a new one.  Reserve enough space here for
 * inode, bitmap, superblock, group and indirection updates for at least
 * one block, plus two quota updates.  Quota allocations are not
 * needed. */

#define DEDUPFS_RESERVE_TRANS_BLOCKS	12U

#define DEDUPFS_INDEX_EXTRA_TRANS_BLOCKS	8

#ifdef CONFIG_QUOTA
/* Amount of blocks needed for quota update - we know that the structure was
 * allocated so we need to update only inode+data */
#define DEDUPFS_QUOTA_TRANS_BLOCKS(sb) (test_opt(sb, QUOTA) ? 2 : 0)
/* Amount of blocks needed for quota insert/delete - we do some block writes
 * but inode, sb and group updates are done only once */
#define DEDUPFS_QUOTA_INIT_BLOCKS(sb) (test_opt(sb, QUOTA) ? (DQUOT_INIT_ALLOC*\
		(DEDUPFS_SINGLEDATA_TRANS_BLOCKS-3)+3+DQUOT_INIT_REWRITE) : 0)
#define DEDUPFS_QUOTA_DEL_BLOCKS(sb) (test_opt(sb, QUOTA) ? (DQUOT_DEL_ALLOC*\
		(DEDUPFS_SINGLEDATA_TRANS_BLOCKS-3)+3+DQUOT_DEL_REWRITE) : 0)
#else
#define DEDUPFS_QUOTA_TRANS_BLOCKS(sb) 0
#define DEDUPFS_QUOTA_INIT_BLOCKS(sb) 0
#define DEDUPFS_QUOTA_DEL_BLOCKS(sb) 0
#endif
#define DEDUPFS_MAXQUOTAS_TRANS_BLOCKS(sb) (MAXQUOTAS*DEDUPFS_QUOTA_TRANS_BLOCKS(sb))
#define DEDUPFS_MAXQUOTAS_INIT_BLOCKS(sb) (MAXQUOTAS*DEDUPFS_QUOTA_INIT_BLOCKS(sb))
#define DEDUPFS_MAXQUOTAS_DEL_BLOCKS(sb) (MAXQUOTAS*DEDUPFS_QUOTA_DEL_BLOCKS(sb))

int
dedupfs_mark_iloc_dirty(handle_t *handle,
		     struct inode *inode,
		     struct dedupfs_iloc *iloc);

/*
 * On success, We end up with an outstanding reference count against
 * iloc->bh.  This _must_ be cleaned up later.
 */

int dedupfs_reserve_inode_write(handle_t *handle, struct inode *inode,
			struct dedupfs_iloc *iloc);

int dedupfs_mark_inode_dirty(handle_t *handle, struct inode *inode);

/*
 * Wrapper functions with which dedupfs calls into JBD.  The intent here is
 * to allow these to be turned into appropriate stubs so dedupfs can control
 * ext2 filesystems, so ext2+dedupfs systems only nee one fs.  This work hasn't
 * been done yet.
 */

static inline void dedupfs_journal_release_buffer(handle_t *handle,
						struct buffer_head *bh)
{
	journal_release_buffer(handle, bh);
}

void dedupfs_journal_abort_handle(const char *caller, const char *err_fn,
		struct buffer_head *bh, handle_t *handle, int err);

int __dedupfs_journal_get_undo_access(const char *where, handle_t *handle,
				struct buffer_head *bh);

int __dedupfs_journal_get_write_access(const char *where, handle_t *handle,
				struct buffer_head *bh);

int __dedupfs_journal_forget(const char *where, handle_t *handle,
				struct buffer_head *bh);

int __dedupfs_journal_revoke(const char *where, handle_t *handle,
				unsigned long blocknr, struct buffer_head *bh);

int __dedupfs_journal_get_create_access(const char *where,
				handle_t *handle, struct buffer_head *bh);

int __dedupfs_journal_dirty_metadata(const char *where,
				handle_t *handle, struct buffer_head *bh);

#define dedupfs_journal_get_undo_access(handle, bh) \
	__dedupfs_journal_get_undo_access(__func__, (handle), (bh))
#define dedupfs_journal_get_write_access(handle, bh) \
	__dedupfs_journal_get_write_access(__func__, (handle), (bh))
#define dedupfs_journal_revoke(handle, blocknr, bh) \
	__dedupfs_journal_revoke(__func__, (handle), (blocknr), (bh))
#define dedupfs_journal_get_create_access(handle, bh) \
	__dedupfs_journal_get_create_access(__func__, (handle), (bh))
#define dedupfs_journal_dirty_metadata(handle, bh) \
	__dedupfs_journal_dirty_metadata(__func__, (handle), (bh))
#define dedupfs_journal_forget(handle, bh) \
	__dedupfs_journal_forget(__func__, (handle), (bh))

int dedupfs_journal_dirty_data(handle_t *handle, struct buffer_head *bh);

handle_t *dedupfs_journal_start_sb(struct super_block *sb, int nblocks);
int __dedupfs_journal_stop(const char *where, handle_t *handle);

static inline handle_t *dedupfs_journal_start(struct inode *inode, int nblocks)
{
	return dedupfs_journal_start_sb(inode->i_sb, nblocks);
}

#define dedupfs_journal_stop(handle) \
	__dedupfs_journal_stop(__func__, (handle))

static inline handle_t *dedupfs_journal_current_handle(void)
{
	return journal_current_handle();
}

static inline int dedupfs_journal_extend(handle_t *handle, int nblocks)
{
	return journal_extend(handle, nblocks);
}

static inline int dedupfs_journal_restart(handle_t *handle, int nblocks)
{
	return journal_restart(handle, nblocks);
}

static inline int dedupfs_journal_blocks_per_page(struct inode *inode)
{
	return journal_blocks_per_page(inode);
}

static inline int dedupfs_journal_force_commit(journal_t *journal)
{
	return journal_force_commit(journal);
}

/* super.c */
int dedupfs_force_commit(struct super_block *sb);

static inline int dedupfs_should_journal_data(struct inode *inode)
{
	if (!S_ISREG(inode->i_mode))
		return 1;
	if (test_opt(inode->i_sb, DATA_FLAGS) == DEDUPFS_MOUNT_JOURNAL_DATA)
		return 1;
	if (DEDUPFS_I(inode)->i_flags & DEDUPFS_JOURNAL_DATA_FL)
		return 1;
	return 0;
}

static inline int dedupfs_should_order_data(struct inode *inode)
{
	if (!S_ISREG(inode->i_mode))
		return 0;
	if (DEDUPFS_I(inode)->i_flags & DEDUPFS_JOURNAL_DATA_FL)
		return 0;
	if (test_opt(inode->i_sb, DATA_FLAGS) == DEDUPFS_MOUNT_ORDERED_DATA)
		return 1;
	return 0;
}

static inline int dedupfs_should_writeback_data(struct inode *inode)
{
	if (!S_ISREG(inode->i_mode))
		return 0;
	if (DEDUPFS_I(inode)->i_flags & DEDUPFS_JOURNAL_DATA_FL)
		return 0;
	if (test_opt(inode->i_sb, DATA_FLAGS) == DEDUPFS_MOUNT_WRITEBACK_DATA)
		return 1;
	return 0;
}

#endif	/* _LINUX_DEDUPFS_JBD_H */
