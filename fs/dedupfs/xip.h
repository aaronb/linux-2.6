/*
 *  linux/fs/dedupfs/xip.h
 *
 * Copyright (C) 2005 IBM Corporation
 * Author: Carsten Otte (cotte@de.ibm.com)
 */

#ifdef CONFIG_EXT2_FS_XIP
extern void dedupfs_xip_verify_sb (struct super_block *);
extern int dedupfs_clear_xip_target (struct inode *, sector_t);

static inline int dedupfs_use_xip (struct super_block *sb)
{
	struct dedupfs_sb_info *sbi = EXT2_SB(sb);
	return (sbi->s_mount_opt & EXT2_MOUNT_XIP);
}
int dedupfs_get_xip_mem(struct address_space *, pgoff_t, int,
				void **, unsigned long *);
#define mapping_is_xip(map) unlikely(map->a_ops->get_xip_mem)
#else
#define mapping_is_xip(map)			0
#define dedupfs_xip_verify_sb(sb)			do { } while (0)
#define dedupfs_use_xip(sb)			0
#define dedupfs_clear_xip_target(inode, chain)	0
#define dedupfs_get_xip_mem			NULL
#endif
