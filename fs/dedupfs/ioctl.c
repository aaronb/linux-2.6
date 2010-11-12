/*
 * linux/fs/dedupfs/ioctl.c
 *
 * Copyright (C) 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 */

#include <linux/fs.h>
#include <linux/jbd.h>
#include <linux/capability.h>
#include "dedupfs.h"
#include "dedupfs_jbd.h"
#include <linux/mount.h>
#include <linux/time.h>
#include <linux/compat.h>
#include <asm/uaccess.h>

long dedupfs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode = filp->f_dentry->d_inode;
	struct dedupfs_inode_info *ei = DEDUPFS_I(inode);
	unsigned int flags;
	unsigned short rsv_window_size;

	dedupfs_debug ("cmd = %u, arg = %lu\n", cmd, arg);

	switch (cmd) {
	case DEDUPFS_IOC_GETFLAGS:
		dedupfs_get_inode_flags(ei);
		flags = ei->i_flags & DEDUPFS_FL_USER_VISIBLE;
		return put_user(flags, (int __user *) arg);
	case DEDUPFS_IOC_SETFLAGS: {
		handle_t *handle = NULL;
		int err;
		struct dedupfs_iloc iloc;
		unsigned int oldflags;
		unsigned int jflag;

		if (!is_owner_or_cap(inode))
			return -EACCES;

		if (get_user(flags, (int __user *) arg))
			return -EFAULT;

		err = mnt_want_write(filp->f_path.mnt);
		if (err)
			return err;

		flags = dedupfs_mask_flags(inode->i_mode, flags);

		mutex_lock(&inode->i_mutex);

		/* Is it quota file? Do not allow user to mess with it */
		err = -EPERM;
		if (IS_NOQUOTA(inode))
			goto flags_out;

		oldflags = ei->i_flags;

		/* The JOURNAL_DATA flag is modifiable only by root */
		jflag = flags & DEDUPFS_JOURNAL_DATA_FL;

		/*
		 * The IMMUTABLE and APPEND_ONLY flags can only be changed by
		 * the relevant capability.
		 *
		 * This test looks nicer. Thanks to Pauline Middelink
		 */
		if ((flags ^ oldflags) & (DEDUPFS_APPEND_FL | DEDUPFS_IMMUTABLE_FL)) {
			if (!capable(CAP_LINUX_IMMUTABLE))
				goto flags_out;
		}

		/*
		 * The JOURNAL_DATA flag can only be changed by
		 * the relevant capability.
		 */
		if ((jflag ^ oldflags) & (DEDUPFS_JOURNAL_DATA_FL)) {
			if (!capable(CAP_SYS_RESOURCE))
				goto flags_out;
		}

		handle = dedupfs_journal_start(inode, 1);
		if (IS_ERR(handle)) {
			err = PTR_ERR(handle);
			goto flags_out;
		}
		if (IS_SYNC(inode))
			handle->h_sync = 1;
		err = dedupfs_reserve_inode_write(handle, inode, &iloc);
		if (err)
			goto flags_err;

		flags = flags & DEDUPFS_FL_USER_MODIFIABLE;
		flags |= oldflags & ~DEDUPFS_FL_USER_MODIFIABLE;
		ei->i_flags = flags;

		dedupfs_set_inode_flags(inode);
		inode->i_ctime = CURRENT_TIME_SEC;

		err = dedupfs_mark_iloc_dirty(handle, inode, &iloc);
flags_err:
		dedupfs_journal_stop(handle);
		if (err)
			goto flags_out;

		if ((jflag ^ oldflags) & (DEDUPFS_JOURNAL_DATA_FL))
			err = dedupfs_change_inode_journal_flag(inode, jflag);
flags_out:
		mutex_unlock(&inode->i_mutex);
		mnt_drop_write(filp->f_path.mnt);
		return err;
	}
	case DEDUPFS_IOC_GETVERSION:
	case DEDUPFS_IOC_GETVERSION_OLD:
		return put_user(inode->i_generation, (int __user *) arg);
	case DEDUPFS_IOC_SETVERSION:
	case DEDUPFS_IOC_SETVERSION_OLD: {
		handle_t *handle;
		struct dedupfs_iloc iloc;
		__u32 generation;
		int err;

		if (!is_owner_or_cap(inode))
			return -EPERM;

		err = mnt_want_write(filp->f_path.mnt);
		if (err)
			return err;
		if (get_user(generation, (int __user *) arg)) {
			err = -EFAULT;
			goto setversion_out;
		}

		handle = dedupfs_journal_start(inode, 1);
		if (IS_ERR(handle)) {
			err = PTR_ERR(handle);
			goto setversion_out;
		}
		err = dedupfs_reserve_inode_write(handle, inode, &iloc);
		if (err == 0) {
			inode->i_ctime = CURRENT_TIME_SEC;
			inode->i_generation = generation;
			err = dedupfs_mark_iloc_dirty(handle, inode, &iloc);
		}
		dedupfs_journal_stop(handle);
setversion_out:
		mnt_drop_write(filp->f_path.mnt);
		return err;
	}
#ifdef CONFIG_JBD_DEBUG
	case DEDUPFS_IOC_WAIT_FOR_READONLY:
		/*
		 * This is racy - by the time we're woken up and running,
		 * the superblock could be released.  And the module could
		 * have been unloaded.  So sue me.
		 *
		 * Returns 1 if it slept, else zero.
		 */
		{
			struct super_block *sb = inode->i_sb;
			DECLARE_WAITQUEUE(wait, current);
			int ret = 0;

			set_current_state(TASK_INTERRUPTIBLE);
			add_wait_queue(&DEDUPFS_SB(sb)->ro_wait_queue, &wait);
			if (timer_pending(&DEDUPFS_SB(sb)->turn_ro_timer)) {
				schedule();
				ret = 1;
			}
			remove_wait_queue(&DEDUPFS_SB(sb)->ro_wait_queue, &wait);
			return ret;
		}
#endif
	case DEDUPFS_IOC_GETRSVSZ:
		if (test_opt(inode->i_sb, RESERVATION)
			&& S_ISREG(inode->i_mode)
			&& ei->i_block_alloc_info) {
			rsv_window_size = ei->i_block_alloc_info->rsv_window_node.rsv_goal_size;
			return put_user(rsv_window_size, (int __user *)arg);
		}
		return -ENOTTY;
	case DEDUPFS_IOC_SETRSVSZ: {
		int err;

		if (!test_opt(inode->i_sb, RESERVATION) ||!S_ISREG(inode->i_mode))
			return -ENOTTY;

		err = mnt_want_write(filp->f_path.mnt);
		if (err)
			return err;

		if (!is_owner_or_cap(inode)) {
			err = -EACCES;
			goto setrsvsz_out;
		}

		if (get_user(rsv_window_size, (int __user *)arg)) {
			err = -EFAULT;
			goto setrsvsz_out;
		}

		if (rsv_window_size > DEDUPFS_MAX_RESERVE_BLOCKS)
			rsv_window_size = DEDUPFS_MAX_RESERVE_BLOCKS;

		/*
		 * need to allocate reservation structure for this inode
		 * before set the window size
		 */
		mutex_lock(&ei->truncate_mutex);
		if (!ei->i_block_alloc_info)
			dedupfs_init_block_alloc_info(inode);

		if (ei->i_block_alloc_info){
			struct dedupfs_reserve_window_node *rsv = &ei->i_block_alloc_info->rsv_window_node;
			rsv->rsv_goal_size = rsv_window_size;
		}
		mutex_unlock(&ei->truncate_mutex);
setrsvsz_out:
		mnt_drop_write(filp->f_path.mnt);
		return err;
	}
	case DEDUPFS_IOC_GROUP_EXTEND: {
		dedupfsblk_t n_blocks_count;
		struct super_block *sb = inode->i_sb;
		int err, err2;

		if (!capable(CAP_SYS_RESOURCE))
			return -EPERM;

		err = mnt_want_write(filp->f_path.mnt);
		if (err)
			return err;

		if (get_user(n_blocks_count, (__u32 __user *)arg)) {
			err = -EFAULT;
			goto group_extend_out;
		}
		err = dedupfs_group_extend(sb, DEDUPFS_SB(sb)->s_es, n_blocks_count);
		journal_lock_updates(DEDUPFS_SB(sb)->s_journal);
		err2 = journal_flush(DEDUPFS_SB(sb)->s_journal);
		journal_unlock_updates(DEDUPFS_SB(sb)->s_journal);
		if (err == 0)
			err = err2;
group_extend_out:
		mnt_drop_write(filp->f_path.mnt);
		return err;
	}
	case DEDUPFS_IOC_GROUP_ADD: {
		struct dedupfs_new_group_data input;
		struct super_block *sb = inode->i_sb;
		int err, err2;

		if (!capable(CAP_SYS_RESOURCE))
			return -EPERM;

		err = mnt_want_write(filp->f_path.mnt);
		if (err)
			return err;

		if (copy_from_user(&input, (struct dedupfs_new_group_input __user *)arg,
				sizeof(input))) {
			err = -EFAULT;
			goto group_add_out;
		}

		err = dedupfs_group_add(sb, &input);
		journal_lock_updates(DEDUPFS_SB(sb)->s_journal);
		err2 = journal_flush(DEDUPFS_SB(sb)->s_journal);
		journal_unlock_updates(DEDUPFS_SB(sb)->s_journal);
		if (err == 0)
			err = err2;
group_add_out:
		mnt_drop_write(filp->f_path.mnt);
		return err;
	}


	default:
		return -ENOTTY;
	}
}

#ifdef CONFIG_COMPAT
long dedupfs_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	/* These are just misnamed, they actually get/put from/to user an int */
	switch (cmd) {
	case DEDUPFS_IOC32_GETFLAGS:
		cmd = DEDUPFS_IOC_GETFLAGS;
		break;
	case DEDUPFS_IOC32_SETFLAGS:
		cmd = DEDUPFS_IOC_SETFLAGS;
		break;
	case DEDUPFS_IOC32_GETVERSION:
		cmd = DEDUPFS_IOC_GETVERSION;
		break;
	case DEDUPFS_IOC32_SETVERSION:
		cmd = DEDUPFS_IOC_SETVERSION;
		break;
	case DEDUPFS_IOC32_GROUP_EXTEND:
		cmd = DEDUPFS_IOC_GROUP_EXTEND;
		break;
	case DEDUPFS_IOC32_GETVERSION_OLD:
		cmd = DEDUPFS_IOC_GETVERSION_OLD;
		break;
	case DEDUPFS_IOC32_SETVERSION_OLD:
		cmd = DEDUPFS_IOC_SETVERSION_OLD;
		break;
#ifdef CONFIG_JBD_DEBUG
	case DEDUPFS_IOC32_WAIT_FOR_READONLY:
		cmd = DEDUPFS_IOC_WAIT_FOR_READONLY;
		break;
#endif
	case DEDUPFS_IOC32_GETRSVSZ:
		cmd = DEDUPFS_IOC_GETRSVSZ;
		break;
	case DEDUPFS_IOC32_SETRSVSZ:
		cmd = DEDUPFS_IOC_SETRSVSZ;
		break;
	case DEDUPFS_IOC_GROUP_ADD:
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return dedupfs_ioctl(file, cmd, (unsigned long) compat_ptr(arg));
}
#endif
