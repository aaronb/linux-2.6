/*
 *  linux/include/linux/dedupfs_fs.h
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/include/linux/minix_fs.h
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#ifndef _LINUX_DEDUPFS_H
#define _LINUX_DEDUPFS_H

#include <linux/types.h>
#include <linux/magic.h>

/*
 * The second extended filesystem constants/structures
 */

/*
 * Define DEDUPFS_DEBUG to produce debug messages
 */
#undef DEDUPFS_DEBUG

/*
 * Define DEDUPFS_RESERVATION to reserve data blocks for expanding files
 */
#define DEDUPFS_DEFAULT_RESERVE_BLOCKS     8
/*max window size: 1024(direct blocks) + 3([t,d]indirect blocks) */
#define DEDUPFS_MAX_RESERVE_BLOCKS         1027
#define DEDUPFS_RESERVE_WINDOW_NOT_ALLOCATED 0
/*
 * The second extended file system version
 */
#define DEDUPFS_DATE		"95/08/09"
#define DEDUPFS_VERSION		"0.5b"

/*
 * Debug code
 */
#ifdef DEDUPFS_DEBUG
#	define dedupfs_debug(f, a...)	{ \
					printk ("DEDUPFS-fs DEBUG (%s, %d): %s:", \
						__FILE__, __LINE__, __func__); \
				  	printk (f, ## a); \
					}
#else
#	define dedupfs_debug(f, a...)	/**/
#endif

/*
 * Special inode numbers
 */
#define	DEDUPFS_BAD_INO		 1	/* Bad blocks inode */
#define DEDUPFS_ROOT_INO		 2	/* Root inode */
#define DEDUPFS_BOOT_LOADER_INO	 5	/* Boot loader inode */
#define DEDUPFS_UNDEL_DIR_INO	 6	/* Undelete directory inode */

/* First non-reserved inode for old dedupfs filesystems */
#define DEDUPFS_GOOD_OLD_FIRST_INO	11

#ifdef __KERNEL__
#include "dedupfs_include_sb.h"
static inline struct dedupfs_sb_info *DEDUPFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}
#else
/* Assume that user mode programs are passing in an dedupfsfs superblock, not
 * a kernel struct super_block.  This will allow us to call the feature-test
 * macros from user land. */
#define DEDUPFS_SB(sb)	(sb)
#endif

/*
 * Maximal count of links to a file
 */
#define DEDUPFS_LINK_MAX		32000

/*
 * Macro-instructions used to manage several block sizes
 */
#define DEDUPFS_MIN_BLOCK_SIZE		1024
#define	DEDUPFS_MAX_BLOCK_SIZE		4096
#define DEDUPFS_MIN_BLOCK_LOG_SIZE		  10
#ifdef __KERNEL__
# define DEDUPFS_BLOCK_SIZE(s)		((s)->s_blocksize)
#else
# define DEDUPFS_BLOCK_SIZE(s)		(DEDUPFS_MIN_BLOCK_SIZE << (s)->s_log_block_size)
#endif
#define	DEDUPFS_ADDR_PER_BLOCK(s)		(DEDUPFS_BLOCK_SIZE(s) / sizeof (__u32))
#ifdef __KERNEL__
# define DEDUPFS_BLOCK_SIZE_BITS(s)	((s)->s_blocksize_bits)
#else
# define DEDUPFS_BLOCK_SIZE_BITS(s)	((s)->s_log_block_size + 10)
#endif
#ifdef __KERNEL__
#define	DEDUPFS_ADDR_PER_BLOCK_BITS(s)	(DEDUPFS_SB(s)->s_addr_per_block_bits)
#define DEDUPFS_INODE_SIZE(s)		(DEDUPFS_SB(s)->s_inode_size)
#define DEDUPFS_FIRST_INO(s)		(DEDUPFS_SB(s)->s_first_ino)
#else
#define DEDUPFS_INODE_SIZE(s)	(((s)->s_rev_level == DEDUPFS_GOOD_OLD_REV) ? \
				 DEDUPFS_GOOD_OLD_INODE_SIZE : \
				 (s)->s_inode_size)
#define DEDUPFS_FIRST_INO(s)	(((s)->s_rev_level == DEDUPFS_GOOD_OLD_REV) ? \
				 DEDUPFS_GOOD_OLD_FIRST_INO : \
				 (s)->s_first_ino)
#endif

/*
 * Macro-instructions used to manage fragments
 */
#define DEDUPFS_MIN_FRAG_SIZE		1024
#define	DEDUPFS_MAX_FRAG_SIZE		4096
#define DEDUPFS_MIN_FRAG_LOG_SIZE		  10
#ifdef __KERNEL__
# define DEDUPFS_FRAG_SIZE(s)		(DEDUPFS_SB(s)->s_frag_size)
# define DEDUPFS_FRAGS_PER_BLOCK(s)	(DEDUPFS_SB(s)->s_frags_per_block)
#else
# define DEDUPFS_FRAG_SIZE(s)		(DEDUPFS_MIN_FRAG_SIZE << (s)->s_log_frag_size)
# define DEDUPFS_FRAGS_PER_BLOCK(s)	(DEDUPFS_BLOCK_SIZE(s) / DEDUPFS_FRAG_SIZE(s))
#endif

/*
 * Structure of a blocks group descriptor
 */
struct dedupfs_group_desc
{
	__le32	bg_block_bitmap;		/* Blocks bitmap block */
	__le32	bg_inode_bitmap;		/* Inodes bitmap block */
	__le32	bg_inode_table;		/* Inodes table block */
	__le16	bg_free_blocks_count;	/* Free blocks count */
	__le16	bg_free_inodes_count;	/* Free inodes count */
	__le16	bg_used_dirs_count;	/* Directories count */
	__le16	bg_pad;
	__le32	bg_reserved[3];
};

/*
 * Macro-instructions used to manage group descriptors
 */
#ifdef __KERNEL__
# define DEDUPFS_BLOCKS_PER_GROUP(s)	(DEDUPFS_SB(s)->s_blocks_per_group)
# define DEDUPFS_DESC_PER_BLOCK(s)		(DEDUPFS_SB(s)->s_desc_per_block)
# define DEDUPFS_INODES_PER_GROUP(s)	(DEDUPFS_SB(s)->s_inodes_per_group)
# define DEDUPFS_DESC_PER_BLOCK_BITS(s)	(DEDUPFS_SB(s)->s_desc_per_block_bits)
#else
# define DEDUPFS_BLOCKS_PER_GROUP(s)	((s)->s_blocks_per_group)
# define DEDUPFS_DESC_PER_BLOCK(s)		(DEDUPFS_BLOCK_SIZE(s) / sizeof (struct dedupfs_group_desc))
# define DEDUPFS_INODES_PER_GROUP(s)	((s)->s_inodes_per_group)
#endif

/*
 * Constants relative to the data blocks
 */
#define	DEDUPFS_NDIR_BLOCKS		12
#define	DEDUPFS_IND_BLOCK			DEDUPFS_NDIR_BLOCKS
#define	DEDUPFS_DIND_BLOCK			(DEDUPFS_IND_BLOCK + 1)
#define	DEDUPFS_TIND_BLOCK			(DEDUPFS_DIND_BLOCK + 1)
#define	DEDUPFS_N_BLOCKS			(DEDUPFS_TIND_BLOCK + 1)

/*
 * Inode flags (GETFLAGS/SETFLAGS)
 */
#define	DEDUPFS_SECRM_FL			FS_SECRM_FL	/* Secure deletion */
#define	DEDUPFS_UNRM_FL			FS_UNRM_FL	/* Undelete */
#define	DEDUPFS_COMPR_FL			FS_COMPR_FL	/* Compress file */
#define DEDUPFS_SYNC_FL			FS_SYNC_FL	/* Synchronous updates */
#define DEDUPFS_IMMUTABLE_FL		FS_IMMUTABLE_FL	/* Immutable file */
#define DEDUPFS_APPEND_FL			FS_APPEND_FL	/* writes to file may only append */
#define DEDUPFS_NODUMP_FL			FS_NODUMP_FL	/* do not dump file */
#define DEDUPFS_NOATIME_FL			FS_NOATIME_FL	/* do not update atime */
/* Reserved for compression usage... */
#define DEDUPFS_DIRTY_FL			FS_DIRTY_FL
#define DEDUPFS_COMPRBLK_FL		FS_COMPRBLK_FL	/* One or more compressed clusters */
#define DEDUPFS_NOCOMP_FL			FS_NOCOMP_FL	/* Don't compress */
#define DEDUPFS_ECOMPR_FL			FS_ECOMPR_FL	/* Compression error */
/* End compression flags --- maybe not all used */	
#define DEDUPFS_BTREE_FL			FS_BTREE_FL	/* btree format dir */
#define DEDUPFS_INDEX_FL			FS_INDEX_FL	/* hash-indexed directory */
#define DEDUPFS_IMAGIC_FL			FS_IMAGIC_FL	/* AFS directory */
#define DEDUPFS_JOURNAL_DATA_FL		FS_JOURNAL_DATA_FL /* Reserved for ext3 */
#define DEDUPFS_NOTAIL_FL			FS_NOTAIL_FL	/* file tail should not be merged */
#define DEDUPFS_DIRSYNC_FL			FS_DIRSYNC_FL	/* dirsync behaviour (directories only) */
#define DEDUPFS_TOPDIR_FL			FS_TOPDIR_FL	/* Top of directory hierarchies*/
#define DEDUPFS_RESERVED_FL		FS_RESERVED_FL	/* reserved for dedupfs lib */

#define DEDUPFS_FL_USER_VISIBLE		FS_FL_USER_VISIBLE	/* User visible flags */
#define DEDUPFS_FL_USER_MODIFIABLE		FS_FL_USER_MODIFIABLE	/* User modifiable flags */

/* Flags that should be inherited by new inodes from their parent. */
#define DEDUPFS_FL_INHERITED (DEDUPFS_SECRM_FL | DEDUPFS_UNRM_FL | DEDUPFS_COMPR_FL |\
			   DEDUPFS_SYNC_FL | DEDUPFS_IMMUTABLE_FL | DEDUPFS_APPEND_FL |\
			   DEDUPFS_NODUMP_FL | DEDUPFS_NOATIME_FL | DEDUPFS_COMPRBLK_FL|\
			   DEDUPFS_NOCOMP_FL | DEDUPFS_JOURNAL_DATA_FL |\
			   DEDUPFS_NOTAIL_FL | DEDUPFS_DIRSYNC_FL)

/* Flags that are appropriate for regular files (all but dir-specific ones). */
#define DEDUPFS_REG_FLMASK (~(DEDUPFS_DIRSYNC_FL | DEDUPFS_TOPDIR_FL))

/* Flags that are appropriate for non-directories/regular files. */
#define DEDUPFS_OTHER_FLMASK (DEDUPFS_NODUMP_FL | DEDUPFS_NOATIME_FL)

/* Mask out flags that are inappropriate for the given type of inode. */
static inline __u32 dedupfs_mask_flags(umode_t mode, __u32 flags)
{
	if (S_ISDIR(mode))
		return flags;
	else if (S_ISREG(mode))
		return flags & DEDUPFS_REG_FLMASK;
	else
		return flags & DEDUPFS_OTHER_FLMASK;
}

/*
 * ioctl commands
 */
#define	DEDUPFS_IOC_GETFLAGS		FS_IOC_GETFLAGS
#define	DEDUPFS_IOC_SETFLAGS		FS_IOC_SETFLAGS
#define	DEDUPFS_IOC_GETVERSION		FS_IOC_GETVERSION
#define	DEDUPFS_IOC_SETVERSION		FS_IOC_SETVERSION
#define	DEDUPFS_IOC_GETRSVSZ		_IOR('f', 5, long)
#define	DEDUPFS_IOC_SETRSVSZ		_IOW('f', 6, long)

/*
 * ioctl commands in 32 bit emulation
 */
#define DEDUPFS_IOC32_GETFLAGS		FS_IOC32_GETFLAGS
#define DEDUPFS_IOC32_SETFLAGS		FS_IOC32_SETFLAGS
#define DEDUPFS_IOC32_GETVERSION		FS_IOC32_GETVERSION
#define DEDUPFS_IOC32_SETVERSION		FS_IOC32_SETVERSION

/*
 * Structure of an inode on the disk
 */
struct dedupfs_inode {
	__le16	i_mode;		/* File mode */
	__le16	i_uid;		/* Low 16 bits of Owner Uid */
	__le32	i_size;		/* Size in bytes */
	__le32	i_atime;	/* Access time */
	__le32	i_ctime;	/* Creation time */
	__le32	i_mtime;	/* Modification time */
	__le32	i_dtime;	/* Deletion Time */
	__le16	i_gid;		/* Low 16 bits of Group Id */
	__le16	i_links_count;	/* Links count */
	__le32	i_blocks;	/* Blocks count */
	__le32	i_flags;	/* File flags */
	union {
		struct {
			__le32  l_i_reserved1;
		} linux1;
		struct {
			__le32  h_i_translator;
		} hurd1;
		struct {
			__le32  m_i_reserved1;
		} masix1;
	} osd1;				/* OS dependent 1 */
	__le32	i_block[DEDUPFS_N_BLOCKS];/* Pointers to blocks */
	__le32	i_generation;	/* File version (for NFS) */
	__le32	i_file_acl;	/* File ACL */
	__le32	i_dir_acl;	/* Directory ACL */
	__le32	i_faddr;	/* Fragment address */
	union {
		struct {
			__u8	l_i_frag;	/* Fragment number */
			__u8	l_i_fsize;	/* Fragment size */
			__u16	i_pad1;
			__le16	l_i_uid_high;	/* these 2 fields    */
			__le16	l_i_gid_high;	/* were reserved2[0] */
			__u32	l_i_reserved2;
		} linux2;
		struct {
			__u8	h_i_frag;	/* Fragment number */
			__u8	h_i_fsize;	/* Fragment size */
			__le16	h_i_mode_high;
			__le16	h_i_uid_high;
			__le16	h_i_gid_high;
			__le32	h_i_author;
		} hurd2;
		struct {
			__u8	m_i_frag;	/* Fragment number */
			__u8	m_i_fsize;	/* Fragment size */
			__u16	m_pad1;
			__u32	m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};

#define i_size_high	i_dir_acl

#if defined(__KERNEL__) || defined(__linux__)
#define i_reserved1	osd1.linux1.l_i_reserved1
#define i_frag		osd2.linux2.l_i_frag
#define i_fsize		osd2.linux2.l_i_fsize
#define i_uid_low	i_uid
#define i_gid_low	i_gid
#define i_uid_high	osd2.linux2.l_i_uid_high
#define i_gid_high	osd2.linux2.l_i_gid_high
#define i_reserved2	osd2.linux2.l_i_reserved2
#endif

#ifdef	__hurd__
#define i_translator	osd1.hurd1.h_i_translator
#define i_frag		osd2.hurd2.h_i_frag
#define i_fsize		osd2.hurd2.h_i_fsize
#define i_uid_high	osd2.hurd2.h_i_uid_high
#define i_gid_high	osd2.hurd2.h_i_gid_high
#define i_author	osd2.hurd2.h_i_author
#endif

#ifdef	__masix__
#define i_reserved1	osd1.masix1.m_i_reserved1
#define i_frag		osd2.masix2.m_i_frag
#define i_fsize		osd2.masix2.m_i_fsize
#define i_reserved2	osd2.masix2.m_i_reserved2
#endif

/*
 * File system states
 */
#define	DEDUPFS_VALID_FS			0x0001	/* Unmounted cleanly */
#define	DEDUPFS_ERROR_FS			0x0002	/* Errors detected */

/*
 * Mount flags
 */
#define DEDUPFS_MOUNT_CHECK		0x000001  /* Do mount-time checks */
#define DEDUPFS_MOUNT_OLDALLOC		0x000002  /* Don't use the new Orlov allocator */
#define DEDUPFS_MOUNT_GRPID		0x000004  /* Create files with directory's group */
#define DEDUPFS_MOUNT_DEBUG		0x000008  /* Some debugging messages */
#define DEDUPFS_MOUNT_ERRORS_CONT		0x000010  /* Continue on errors */
#define DEDUPFS_MOUNT_ERRORS_RO		0x000020  /* Remount fs ro on errors */
#define DEDUPFS_MOUNT_ERRORS_PANIC		0x000040  /* Panic on errors */
#define DEDUPFS_MOUNT_MINIX_DF		0x000080  /* Mimics the Minix statfs */
#define DEDUPFS_MOUNT_NOBH			0x000100  /* No buffer_heads */
#define DEDUPFS_MOUNT_NO_UID32		0x000200  /* Disable 32-bit UIDs */
#define DEDUPFS_MOUNT_XATTR_USER		0x004000  /* Extended user attributes */
#define DEDUPFS_MOUNT_POSIX_ACL		0x008000  /* POSIX Access Control Lists */
#define DEDUPFS_MOUNT_XIP			0x010000  /* Execute in place */
#define DEDUPFS_MOUNT_USRQUOTA		0x020000  /* user quota */
#define DEDUPFS_MOUNT_GRPQUOTA		0x040000  /* group quota */
#define DEDUPFS_MOUNT_RESERVATION		0x080000  /* Preallocation */


#define clear_opt(o, opt)		o &= ~DEDUPFS_MOUNT_##opt
#define set_opt(o, opt)			o |= DEDUPFS_MOUNT_##opt
#define test_opt(sb, opt)		(DEDUPFS_SB(sb)->s_mount_opt & \
					 DEDUPFS_MOUNT_##opt)
/*
 * Maximal mount counts between two filesystem checks
 */
#define DEDUPFS_DFL_MAX_MNT_COUNT		20	/* Allow 20 mounts */
#define DEDUPFS_DFL_CHECKINTERVAL		0	/* Don't use interval check */

/*
 * Behaviour when detecting errors
 */
#define DEDUPFS_ERRORS_CONTINUE		1	/* Continue execution */
#define DEDUPFS_ERRORS_RO			2	/* Remount fs read-only */
#define DEDUPFS_ERRORS_PANIC		3	/* Panic */
#define DEDUPFS_ERRORS_DEFAULT		DEDUPFS_ERRORS_CONTINUE

/*
 * Structure of the super block
 */
struct dedupfs_super_block {
	__le32	s_inodes_count;		/* Inodes count */
	__le32	s_blocks_count;		/* Blocks count */
	__le32	s_r_blocks_count;	/* Reserved blocks count */
	__le32	s_free_blocks_count;	/* Free blocks count */
	__le32	s_free_inodes_count;	/* Free inodes count */
	__le32	s_first_data_block;	/* First Data Block */
	__le32	s_log_block_size;	/* Block size */
	__le32	s_log_frag_size;	/* Fragment size */
	__le32	s_blocks_per_group;	/* # Blocks per group */
	__le32	s_frags_per_group;	/* # Fragments per group */
	__le32	s_inodes_per_group;	/* # Inodes per group */
	__le32	s_mtime;		/* Mount time */
	__le32	s_wtime;		/* Write time */
	__le16	s_mnt_count;		/* Mount count */
	__le16	s_max_mnt_count;	/* Maximal mount count */
	__le16	s_magic;		/* Magic signature */
	__le16	s_state;		/* File system state */
	__le16	s_errors;		/* Behaviour when detecting errors */
	__le16	s_minor_rev_level; 	/* minor revision level */
	__le32	s_lastcheck;		/* time of last check */
	__le32	s_checkinterval;	/* max. time between checks */
	__le32	s_creator_os;		/* OS */
	__le32	s_rev_level;		/* Revision level */
	__le16	s_def_resuid;		/* Default uid for reserved blocks */
	__le16	s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for DEDUPFS_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 * 
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	__le32	s_first_ino; 		/* First non-reserved inode */
	__le16   s_inode_size; 		/* size of inode structure */
	__le16	s_block_group_nr; 	/* block group # of this superblock */
	__le32	s_feature_compat; 	/* compatible feature set */
	__le32	s_feature_incompat; 	/* incompatible feature set */
	__le32	s_feature_ro_compat; 	/* readonly-compatible feature set */
	__u8	s_uuid[16];		/* 128-bit uuid for volume */
	char	s_volume_name[16]; 	/* volume name */
	char	s_last_mounted[64]; 	/* directory where last mounted */
	__le32	s_algorithm_usage_bitmap; /* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the DEDUPFS_COMPAT_PREALLOC flag is on.
	 */
	__u8	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
	__u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
	__u16	s_padding1;
	/*
	 * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	__u8	s_journal_uuid[16];	/* uuid of journal superblock */
	__u32	s_journal_inum;		/* inode number of journal file */
	__u32	s_journal_dev;		/* device number of journal file */
	__u32	s_last_orphan;		/* start of list of inodes to delete */
	__u32	s_hash_seed[4];		/* HTREE hash seed */
	__u8	s_def_hash_version;	/* Default hash version to use */
	__u8	s_reserved_char_pad;
	__u16	s_reserved_word_pad;
	__le32	s_default_mount_opts;
 	__le32	s_first_meta_bg; 	/* First metablock block group */
	__u32	s_reserved[190];	/* Padding to the end of the block */
};

/*
 * Codes for operating systems
 */
#define DEDUPFS_OS_LINUX		0
#define DEDUPFS_OS_HURD		1
#define DEDUPFS_OS_MASIX		2
#define DEDUPFS_OS_FREEBSD		3
#define DEDUPFS_OS_LITES		4

/*
 * Revision levels
 */
#define DEDUPFS_GOOD_OLD_REV	0	/* The good old (original) format */
#define DEDUPFS_DYNAMIC_REV	1 	/* V2 format w/ dynamic inode sizes */

#define DEDUPFS_CURRENT_REV	DEDUPFS_GOOD_OLD_REV
#define DEDUPFS_MAX_SUPP_REV	DEDUPFS_DYNAMIC_REV

#define DEDUPFS_GOOD_OLD_INODE_SIZE 128

/*
 * Feature set definitions
 */

#define DEDUPFS_HAS_COMPAT_FEATURE(sb,mask)			\
	( DEDUPFS_SB(sb)->s_es->s_feature_compat & cpu_to_le32(mask) )
#define DEDUPFS_HAS_RO_COMPAT_FEATURE(sb,mask)			\
	( DEDUPFS_SB(sb)->s_es->s_feature_ro_compat & cpu_to_le32(mask) )
#define DEDUPFS_HAS_INCOMPAT_FEATURE(sb,mask)			\
	( DEDUPFS_SB(sb)->s_es->s_feature_incompat & cpu_to_le32(mask) )
#define DEDUPFS_SET_COMPAT_FEATURE(sb,mask)			\
	DEDUPFS_SB(sb)->s_es->s_feature_compat |= cpu_to_le32(mask)
#define DEDUPFS_SET_RO_COMPAT_FEATURE(sb,mask)			\
	DEDUPFS_SB(sb)->s_es->s_feature_ro_compat |= cpu_to_le32(mask)
#define DEDUPFS_SET_INCOMPAT_FEATURE(sb,mask)			\
	DEDUPFS_SB(sb)->s_es->s_feature_incompat |= cpu_to_le32(mask)
#define DEDUPFS_CLEAR_COMPAT_FEATURE(sb,mask)			\
	DEDUPFS_SB(sb)->s_es->s_feature_compat &= ~cpu_to_le32(mask)
#define DEDUPFS_CLEAR_RO_COMPAT_FEATURE(sb,mask)			\
	DEDUPFS_SB(sb)->s_es->s_feature_ro_compat &= ~cpu_to_le32(mask)
#define DEDUPFS_CLEAR_INCOMPAT_FEATURE(sb,mask)			\
	DEDUPFS_SB(sb)->s_es->s_feature_incompat &= ~cpu_to_le32(mask)

#define DEDUPFS_FEATURE_COMPAT_DIR_PREALLOC	0x0001
#define DEDUPFS_FEATURE_COMPAT_IMAGIC_INODES	0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#define DEDUPFS_FEATURE_COMPAT_EXT_ATTR		0x0008
#define DEDUPFS_FEATURE_COMPAT_RESIZE_INO		0x0010
#define DEDUPFS_FEATURE_COMPAT_DIR_INDEX		0x0020
#define DEDUPFS_FEATURE_COMPAT_ANY			0xffffffff

#define DEDUPFS_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#define DEDUPFS_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
#define DEDUPFS_FEATURE_RO_COMPAT_BTREE_DIR	0x0004
#define DEDUPFS_FEATURE_RO_COMPAT_ANY		0xffffffff

#define DEDUPFS_FEATURE_INCOMPAT_COMPRESSION	0x0001
#define DEDUPFS_FEATURE_INCOMPAT_FILETYPE		0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER		0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV	0x0008
#define DEDUPFS_FEATURE_INCOMPAT_META_BG		0x0010
#define DEDUPFS_FEATURE_INCOMPAT_ANY		0xffffffff

#define DEDUPFS_FEATURE_COMPAT_SUPP	DEDUPFS_FEATURE_COMPAT_EXT_ATTR
#define DEDUPFS_FEATURE_INCOMPAT_SUPP	(DEDUPFS_FEATURE_INCOMPAT_FILETYPE| \
					 DEDUPFS_FEATURE_INCOMPAT_META_BG)
#define DEDUPFS_FEATURE_RO_COMPAT_SUPP	(DEDUPFS_FEATURE_RO_COMPAT_SPARSE_SUPER| \
					 DEDUPFS_FEATURE_RO_COMPAT_LARGE_FILE| \
					 DEDUPFS_FEATURE_RO_COMPAT_BTREE_DIR)
#define DEDUPFS_FEATURE_RO_COMPAT_UNSUPPORTED	~DEDUPFS_FEATURE_RO_COMPAT_SUPP
#define DEDUPFS_FEATURE_INCOMPAT_UNSUPPORTED	~DEDUPFS_FEATURE_INCOMPAT_SUPP

/*
 * Default values for user and/or group using reserved blocks
 */
#define	DEDUPFS_DEF_RESUID		0
#define	DEDUPFS_DEF_RESGID		0

/*
 * Default mount options
 */
#define DEDUPFS_DEFM_DEBUG		0x0001
#define DEDUPFS_DEFM_BSDGROUPS	0x0002
#define DEDUPFS_DEFM_XATTR_USER	0x0004
#define DEDUPFS_DEFM_ACL		0x0008
#define DEDUPFS_DEFM_UID16		0x0010
    /* Not used by dedupfs, but reserved for use by ext3 */
#define EXT3_DEFM_JMODE		0x0060 
#define EXT3_DEFM_JMODE_DATA	0x0020
#define EXT3_DEFM_JMODE_ORDERED	0x0040
#define EXT3_DEFM_JMODE_WBACK	0x0060

/*
 * Structure of a directory entry
 */
#define DEDUPFS_NAME_LEN 255

struct dedupfs_dir_entry {
	__le32	inode;			/* Inode number */
	__le16	rec_len;		/* Directory entry length */
	__le16	name_len;		/* Name length */
	char	name[DEDUPFS_NAME_LEN];	/* File name */
};

/*
 * The new version of the directory entry.  Since DEDUPFS structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
struct dedupfs_dir_entry_2 {
	__le32	inode;			/* Inode number */
	__le16	rec_len;		/* Directory entry length */
	__u8	name_len;		/* Name length */
	__u8	file_type;
	char	name[DEDUPFS_NAME_LEN];	/* File name */
};

/*
 * Ext2 directory file types.  Only the low 3 bits are used.  The
 * other bits are reserved for now.
 */
enum {
	DEDUPFS_FT_UNKNOWN		= 0,
	DEDUPFS_FT_REG_FILE	= 1,
	DEDUPFS_FT_DIR		= 2,
	DEDUPFS_FT_CHRDEV		= 3,
	DEDUPFS_FT_BLKDEV		= 4,
	DEDUPFS_FT_FIFO		= 5,
	DEDUPFS_FT_SOCK		= 6,
	DEDUPFS_FT_SYMLINK		= 7,
	DEDUPFS_FT_MAX
};

/*
 * DEDUPFS_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define DEDUPFS_DIR_PAD		 	4
#define DEDUPFS_DIR_ROUND 			(DEDUPFS_DIR_PAD - 1)
#define DEDUPFS_DIR_REC_LEN(name_len)	(((name_len) + 8 + DEDUPFS_DIR_ROUND) & \
					 ~DEDUPFS_DIR_ROUND)
#define DEDUPFS_MAX_REC_LEN		((1<<16)-1)

#endif	/* _LINUX_DEDUPFS_H */
