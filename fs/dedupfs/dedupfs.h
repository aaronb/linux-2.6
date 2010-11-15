/*
 *  linux/include/linux/dedupfs.h
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
#define DEDUPFS_SUPER_MAGIC EXT3_SUPER_MAGIC

/*
 * The second extended filesystem constants/structures
 */

/*
 * Define DEDUPFS_DEBUG to produce debug messages
 */
//#undef DEDUPFS_DEBUG
#define DEDUPFS_DEBUG

/*
 * Define DEDUPFS_RESERVATION to reserve data blocks for expanding files
 */
#define DEDUPFS_DEFAULT_RESERVE_BLOCKS     8
/*max window size: 1024(direct blocks) + 3([t,d]indirect blocks) */
#define DEDUPFS_MAX_RESERVE_BLOCKS         1027
#define DEDUPFS_RESERVE_WINDOW_NOT_ALLOCATED 0

/*
 * Debug code
 */
#ifdef DEDUPFS_DEBUG
#define dedupfs_debug(f, a...)						\
	do {								\
		printk (KERN_DEBUG "DEDUPFS-fs DEBUG (%s, %d): %s:",	\
			__FILE__, __LINE__, __func__);		\
		printk (KERN_DEBUG f, ## a);				\
	} while (0)
#else
#define dedupfs_debug(f, a...)	do {} while (0)
#endif

/*
 * Special inodes numbers
 */
#define	DEDUPFS_BAD_INO		 1	/* Bad blocks inode */
#define DEDUPFS_ROOT_INO		 2	/* Root inode */
#define DEDUPFS_BOOT_LOADER_INO	 5	/* Boot loader inode */
#define DEDUPFS_UNDEL_DIR_INO	 6	/* Undelete directory inode */
#define DEDUPFS_RESIZE_INO		 7	/* Reserved group descriptors inode */
#define DEDUPFS_JOURNAL_INO	 8	/* Journal inode */

/* First non-reserved inode for old dedupfs filesystems */
#define DEDUPFS_GOOD_OLD_FIRST_INO	11

/*
 * Maximal count of links to a file
 */
#define DEDUPFS_LINK_MAX		32000

/*
 * Macro-instructions used to manage several block sizes
 */
#define DEDUPFS_MIN_BLOCK_SIZE		1024
#define	DEDUPFS_MAX_BLOCK_SIZE		65536
#define DEDUPFS_MIN_BLOCK_LOG_SIZE		10
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
	__u16	bg_pad;
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
 * Inode flags
 */
#define	DEDUPFS_SECRM_FL			0x00000001 /* Secure deletion */
#define	DEDUPFS_UNRM_FL			0x00000002 /* Undelete */
#define	DEDUPFS_COMPR_FL			0x00000004 /* Compress file */
#define DEDUPFS_SYNC_FL			0x00000008 /* Synchronous updates */
#define DEDUPFS_IMMUTABLE_FL		0x00000010 /* Immutable file */
#define DEDUPFS_APPEND_FL			0x00000020 /* writes to file may only append */
#define DEDUPFS_NODUMP_FL			0x00000040 /* do not dump file */
#define DEDUPFS_NOATIME_FL			0x00000080 /* do not update atime */
/* Reserved for compression usage... */
#define DEDUPFS_DIRTY_FL			0x00000100
#define DEDUPFS_COMPRBLK_FL		0x00000200 /* One or more compressed clusters */
#define DEDUPFS_NOCOMPR_FL			0x00000400 /* Don't compress */
#define DEDUPFS_ECOMPR_FL			0x00000800 /* Compression error */
/* End compression flags --- maybe not all used */
#define DEDUPFS_INDEX_FL			0x00001000 /* hash-indexed directory */
#define DEDUPFS_IMAGIC_FL			0x00002000 /* AFS directory */
#define DEDUPFS_JOURNAL_DATA_FL		0x00004000 /* file data should be journaled */
#define DEDUPFS_NOTAIL_FL			0x00008000 /* file tail should not be merged */
#define DEDUPFS_DIRSYNC_FL			0x00010000 /* dirsync behaviour (directories only) */
#define DEDUPFS_TOPDIR_FL			0x00020000 /* Top of directory hierarchies*/
#define DEDUPFS_RESERVED_FL		0x80000000 /* reserved for dedupfs lib */

#define DEDUPFS_FL_USER_VISIBLE		0x0003DFFF /* User visible flags */
#define DEDUPFS_FL_USER_MODIFIABLE		0x000380FF /* User modifiable flags */

/* Flags that should be inherited by new inodes from their parent. */
#define DEDUPFS_FL_INHERITED (DEDUPFS_SECRM_FL | DEDUPFS_UNRM_FL | DEDUPFS_COMPR_FL |\
			   DEDUPFS_SYNC_FL | DEDUPFS_IMMUTABLE_FL | DEDUPFS_APPEND_FL |\
			   DEDUPFS_NODUMP_FL | DEDUPFS_NOATIME_FL | DEDUPFS_COMPRBLK_FL|\
			   DEDUPFS_NOCOMPR_FL | DEDUPFS_JOURNAL_DATA_FL |\
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

/* Used to pass group descriptor data when online resize is done */
struct dedupfs_new_group_input {
	__u32 group;            /* Group number for this data */
	__u32 block_bitmap;     /* Absolute block number of block bitmap */
	__u32 inode_bitmap;     /* Absolute block number of inode bitmap */
	__u32 inode_table;      /* Absolute block number of inode table start */
	__u32 blocks_count;     /* Total number of blocks in this group */
	__u16 reserved_blocks;  /* Number of reserved blocks in this group */
	__u16 unused;
};

/* The struct dedupfs_new_group_input in kernel space, with free_blocks_count */
struct dedupfs_new_group_data {
	__u32 group;
	__u32 block_bitmap;
	__u32 inode_bitmap;
	__u32 inode_table;
	__u32 blocks_count;
	__u16 reserved_blocks;
	__u16 unused;
	__u32 free_blocks_count;
};


/*
 * ioctl commands
 */
#define	DEDUPFS_IOC_GETFLAGS		FS_IOC_GETFLAGS
#define	DEDUPFS_IOC_SETFLAGS		FS_IOC_SETFLAGS
#define	DEDUPFS_IOC_GETVERSION		_IOR('f', 3, long)
#define	DEDUPFS_IOC_SETVERSION		_IOW('f', 4, long)
#define DEDUPFS_IOC_GROUP_EXTEND		_IOW('f', 7, unsigned long)
#define DEDUPFS_IOC_GROUP_ADD		_IOW('f', 8,struct dedupfs_new_group_input)
#define	DEDUPFS_IOC_GETVERSION_OLD		FS_IOC_GETVERSION
#define	DEDUPFS_IOC_SETVERSION_OLD		FS_IOC_SETVERSION
#ifdef CONFIG_JBD_DEBUG
#define DEDUPFS_IOC_WAIT_FOR_READONLY	_IOR('f', 99, long)
#endif
#define DEDUPFS_IOC_GETRSVSZ		_IOR('f', 5, long)
#define DEDUPFS_IOC_SETRSVSZ		_IOW('f', 6, long)

/*
 * ioctl commands in 32 bit emulation
 */
#define DEDUPFS_IOC32_GETFLAGS		FS_IOC32_GETFLAGS
#define DEDUPFS_IOC32_SETFLAGS		FS_IOC32_SETFLAGS
#define DEDUPFS_IOC32_GETVERSION		_IOR('f', 3, int)
#define DEDUPFS_IOC32_SETVERSION		_IOW('f', 4, int)
#define DEDUPFS_IOC32_GETRSVSZ		_IOR('f', 5, int)
#define DEDUPFS_IOC32_SETRSVSZ		_IOW('f', 6, int)
#define DEDUPFS_IOC32_GROUP_EXTEND		_IOW('f', 7, unsigned int)
#ifdef CONFIG_JBD_DEBUG
#define DEDUPFS_IOC32_WAIT_FOR_READONLY	_IOR('f', 99, int)
#endif
#define DEDUPFS_IOC32_GETVERSION_OLD	FS_IOC32_GETVERSION
#define DEDUPFS_IOC32_SETVERSION_OLD	FS_IOC32_SETVERSION


/*
 *  Mount options
 */
struct dedupfs_mount_options {
	unsigned long s_mount_opt;
	uid_t s_resuid;
	gid_t s_resgid;
	unsigned long s_commit_interval;
#ifdef CONFIG_QUOTA
	int s_jquota_fmt;
	char *s_qf_names[MAXQUOTAS];
#endif
};

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
			__u32  l_i_reserved1;
		} linux1;
		struct {
			__u32  h_i_translator;
		} hurd1;
		struct {
			__u32  m_i_reserved1;
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
			__u16	h_i_mode_high;
			__u16	h_i_uid_high;
			__u16	h_i_gid_high;
			__u32	h_i_author;
		} hurd2;
		struct {
			__u8	m_i_frag;	/* Fragment number */
			__u8	m_i_fsize;	/* Fragment size */
			__u16	m_pad1;
			__u32	m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
	__le16	i_extra_isize;
	__le16	i_pad1;
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

#elif defined(__GNU__)

#define i_translator	osd1.hurd1.h_i_translator
#define i_frag		osd2.hurd2.h_i_frag;
#define i_fsize		osd2.hurd2.h_i_fsize;
#define i_uid_high	osd2.hurd2.h_i_uid_high
#define i_gid_high	osd2.hurd2.h_i_gid_high
#define i_author	osd2.hurd2.h_i_author

#elif defined(__masix__)

#define i_reserved1	osd1.masix1.m_i_reserved1
#define i_frag		osd2.masix2.m_i_frag
#define i_fsize		osd2.masix2.m_i_fsize
#define i_reserved2	osd2.masix2.m_i_reserved2

#endif /* defined(__KERNEL__) || defined(__linux__) */

/*
 * File system states
 */
#define	DEDUPFS_VALID_FS			0x0001	/* Unmounted cleanly */
#define	DEDUPFS_ERROR_FS			0x0002	/* Errors detected */
#define	DEDUPFS_ORPHAN_FS			0x0004	/* Orphans being recovered */

/*
 * Misc. filesystem flags
 */
#define EXT2_FLAGS_SIGNED_HASH		0x0001  /* Signed dirhash in use */
#define EXT2_FLAGS_UNSIGNED_HASH	0x0002  /* Unsigned dirhash in use */
#define EXT2_FLAGS_TEST_FILESYS		0x0004	/* to test development code */

/*
 * Mount flags
 */
#define DEDUPFS_MOUNT_CHECK		0x00001	/* Do mount-time checks */
#define DEDUPFS_MOUNT_OLDALLOC		0x00002  /* Don't use the new Orlov allocator */
#define DEDUPFS_MOUNT_GRPID		0x00004	/* Create files with directory's group */
#define DEDUPFS_MOUNT_DEBUG		0x00008	/* Some debugging messages */
#define DEDUPFS_MOUNT_ERRORS_CONT		0x00010	/* Continue on errors */
#define DEDUPFS_MOUNT_ERRORS_RO		0x00020	/* Remount fs ro on errors */
#define DEDUPFS_MOUNT_ERRORS_PANIC		0x00040	/* Panic on errors */
#define DEDUPFS_MOUNT_MINIX_DF		0x00080	/* Mimics the Minix statfs */
#define DEDUPFS_MOUNT_NOLOAD		0x00100	/* Don't use existing journal*/
#define DEDUPFS_MOUNT_ABORT		0x00200	/* Fatal error detected */
#define DEDUPFS_MOUNT_DATA_FLAGS		0x00C00	/* Mode for data writes: */
#define DEDUPFS_MOUNT_JOURNAL_DATA		0x00400	/* Write data to journal */
#define DEDUPFS_MOUNT_ORDERED_DATA		0x00800	/* Flush data before commit */
#define DEDUPFS_MOUNT_WRITEBACK_DATA	0x00C00	/* No data ordering */
#define DEDUPFS_MOUNT_UPDATE_JOURNAL	0x01000	/* Update the journal format */
#define DEDUPFS_MOUNT_NO_UID32		0x02000  /* Disable 32-bit UIDs */
#define DEDUPFS_MOUNT_XATTR_USER		0x04000	/* Extended user attributes */
#define DEDUPFS_MOUNT_POSIX_ACL		0x08000	/* POSIX Access Control Lists */
#define DEDUPFS_MOUNT_RESERVATION		0x10000	/* Preallocation */
#define DEDUPFS_MOUNT_BARRIER		0x20000 /* Use block barriers */
#define DEDUPFS_MOUNT_QUOTA		0x80000 /* Some quota option set */
#define DEDUPFS_MOUNT_USRQUOTA		0x100000 /* "old" user quota */
#define DEDUPFS_MOUNT_GRPQUOTA		0x200000 /* "old" group quota */
#define DEDUPFS_MOUNT_DATA_ERR_ABORT	0x400000 /* Abort on file data write
						  * error in ordered mode */

/* Compatibility, for having both ext2_fs.h and dedupfs.h included at once */
#ifndef _LINUX_EXT2_FS_H
#define clear_opt(o, opt)		o &= ~DEDUPFS_MOUNT_##opt
#define set_opt(o, opt)			o |= DEDUPFS_MOUNT_##opt
#define test_opt(sb, opt)		(DEDUPFS_SB(sb)->s_mount_opt & \
					 DEDUPFS_MOUNT_##opt)
#else
#define EXT2_MOUNT_NOLOAD		DEDUPFS_MOUNT_NOLOAD
#define EXT2_MOUNT_ABORT		DEDUPFS_MOUNT_ABORT
#define EXT2_MOUNT_DATA_FLAGS		DEDUPFS_MOUNT_DATA_FLAGS
#endif

#define dedupfs_set_bit			ext2_set_bit
#define dedupfs_set_bit_atomic		ext2_set_bit_atomic
#define dedupfs_clear_bit			ext2_clear_bit
#define dedupfs_clear_bit_atomic		ext2_clear_bit_atomic
#define dedupfs_test_bit			ext2_test_bit
#define dedupfs_find_first_zero_bit	ext2_find_first_zero_bit
#define dedupfs_find_next_zero_bit		ext2_find_next_zero_bit

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
/*00*/	__le32	s_inodes_count;		/* Inodes count */
	__le32	s_blocks_count;		/* Blocks count */
	__le32	s_r_blocks_count;	/* Reserved blocks count */
	__le32	s_free_blocks_count;	/* Free blocks count */
/*10*/	__le32	s_free_inodes_count;	/* Free inodes count */
	__le32	s_first_data_block;	/* First Data Block */
	__le32	s_log_block_size;	/* Block size */
	__le32	s_log_frag_size;	/* Fragment size */
/*20*/	__le32	s_blocks_per_group;	/* # Blocks per group */
	__le32	s_frags_per_group;	/* # Fragments per group */
	__le32	s_inodes_per_group;	/* # Inodes per group */
	__le32	s_mtime;		/* Mount time */
/*30*/	__le32	s_wtime;		/* Write time */
	__le16	s_mnt_count;		/* Mount count */
	__le16	s_max_mnt_count;	/* Maximal mount count */
	__le16	s_magic;		/* Magic signature */
	__le16	s_state;		/* File system state */
	__le16	s_errors;		/* Behaviour when detecting errors */
	__le16	s_minor_rev_level;	/* minor revision level */
/*40*/	__le32	s_lastcheck;		/* time of last check */
	__le32	s_checkinterval;	/* max. time between checks */
	__le32	s_creator_os;		/* OS */
	__le32	s_rev_level;		/* Revision level */
/*50*/	__le16	s_def_resuid;		/* Default uid for reserved blocks */
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
	__le32	s_first_ino;		/* First non-reserved inode */
	__le16   s_inode_size;		/* size of inode structure */
	__le16	s_block_group_nr;	/* block group # of this superblock */
	__le32	s_feature_compat;	/* compatible feature set */
/*60*/	__le32	s_feature_incompat;	/* incompatible feature set */
	__le32	s_feature_ro_compat;	/* readonly-compatible feature set */
/*68*/	__u8	s_uuid[16];		/* 128-bit uuid for volume */
/*78*/	char	s_volume_name[16];	/* volume name */
/*88*/	char	s_last_mounted[64];	/* directory where last mounted */
/*C8*/	__le32	s_algorithm_usage_bitmap; /* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the DEDUPFS_FEATURE_COMPAT_DIR_PREALLOC flag is on.
	 */
	__u8	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
	__u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
	__le16	s_reserved_gdt_blocks;	/* Per group desc for online growth */
	/*
	 * Journaling support valid if DEDUPFS_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
/*D0*/	__u8	s_journal_uuid[16];	/* uuid of journal superblock */
/*E0*/	__le32	s_journal_inum;		/* inode number of journal file */
	__le32	s_journal_dev;		/* device number of journal file */
	__le32	s_last_orphan;		/* start of list of inodes to delete */
	__le32	s_hash_seed[4];		/* HTREE hash seed */
	__u8	s_def_hash_version;	/* Default hash version to use */
	__u8	s_reserved_char_pad;
	__u16	s_reserved_word_pad;
	__le32	s_default_mount_opts;
	__le32	s_first_meta_bg;	/* First metablock block group */
	__le32	s_mkfs_time;		/* When the filesystem was created */
	__le32	s_jnl_blocks[17];	/* Backup of the journal inode */
	/* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
/*150*/	__le32	s_blocks_count_hi;	/* Blocks count */
	__le32	s_r_blocks_count_hi;	/* Reserved blocks count */
	__le32	s_free_blocks_count_hi;	/* Free blocks count */
	__le16	s_min_extra_isize;	/* All inodes have at least # bytes */
	__le16	s_want_extra_isize; 	/* New inodes should reserve # bytes */
	__le32	s_flags;		/* Miscellaneous flags */
	__le16  s_raid_stride;		/* RAID stride */
	__le16  s_mmp_interval;         /* # seconds to wait in MMP checking */
	__le64  s_mmp_block;            /* Block for multi-mount protection */
	__le32  s_raid_stripe_width;    /* blocks on all data disks (N*stride)*/
	__u8	s_log_groups_per_flex;  /* FLEX_BG group size */
	__u8	s_reserved_char_pad2;
	__le16  s_reserved_pad;
	__u32   s_reserved[162];        /* Padding to the end of the block */
};

#ifdef __KERNEL__
#include "dedupfs_i.h"
#include "dedupfs_sb.h"
static inline struct dedupfs_sb_info * DEDUPFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}
static inline struct dedupfs_inode_info *DEDUPFS_I(struct inode *inode)
{
	return container_of(inode, struct dedupfs_inode_info, vfs_inode);
}

static inline int dedupfs_valid_inum(struct super_block *sb, unsigned long ino)
{
	return ino == DEDUPFS_ROOT_INO ||
		ino == DEDUPFS_JOURNAL_INO ||
		ino == DEDUPFS_RESIZE_INO ||
		(ino >= DEDUPFS_FIRST_INO(sb) &&
		 ino <= le32_to_cpu(DEDUPFS_SB(sb)->s_es->s_inodes_count));
}

/*
 * Inode dynamic state flags
 */
enum {
	DEDUPFS_STATE_JDATA,		/* journaled data exists */
	DEDUPFS_STATE_NEW,			/* inode is newly created */
	DEDUPFS_STATE_XATTR,		/* has in-inode xattrs */
	DEDUPFS_STATE_FLUSH_ON_CLOSE,	/* flush dirty pages on close */
};

static inline int dedupfs_test_inode_state(struct inode *inode, int bit)
{
	return test_bit(bit, &DEDUPFS_I(inode)->i_state_flags);
}

static inline void dedupfs_set_inode_state(struct inode *inode, int bit)
{
	set_bit(bit, &DEDUPFS_I(inode)->i_state_flags);
}

static inline void dedupfs_clear_inode_state(struct inode *inode, int bit)
{
	clear_bit(bit, &DEDUPFS_I(inode)->i_state_flags);
}
#else
/* Assume that user mode programs are passing in an dedupfs superblock, not
 * a kernel struct super_block.  This will allow us to call the feature-test
 * macros from user land. */
#define DEDUPFS_SB(sb)	(sb)
#endif

#define NEXT_ORPHAN(inode) DEDUPFS_I(inode)->i_dtime

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
#define DEDUPFS_DYNAMIC_REV	1	/* V2 format w/ dynamic inode sizes */

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
#define DEDUPFS_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#define DEDUPFS_FEATURE_COMPAT_EXT_ATTR		0x0008
#define DEDUPFS_FEATURE_COMPAT_RESIZE_INODE	0x0010
#define DEDUPFS_FEATURE_COMPAT_DIR_INDEX		0x0020

#define DEDUPFS_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#define DEDUPFS_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
#define DEDUPFS_FEATURE_RO_COMPAT_BTREE_DIR	0x0004

#define DEDUPFS_FEATURE_INCOMPAT_COMPRESSION	0x0001
#define DEDUPFS_FEATURE_INCOMPAT_FILETYPE		0x0002
#define DEDUPFS_FEATURE_INCOMPAT_RECOVER		0x0004 /* Needs recovery */
#define DEDUPFS_FEATURE_INCOMPAT_JOURNAL_DEV	0x0008 /* Journal device */
#define DEDUPFS_FEATURE_INCOMPAT_META_BG		0x0010

#define DEDUPFS_FEATURE_COMPAT_SUPP	EXT2_FEATURE_COMPAT_EXT_ATTR
#define DEDUPFS_FEATURE_INCOMPAT_SUPP	(DEDUPFS_FEATURE_INCOMPAT_FILETYPE| \
					 DEDUPFS_FEATURE_INCOMPAT_RECOVER| \
					 DEDUPFS_FEATURE_INCOMPAT_META_BG)
#define DEDUPFS_FEATURE_RO_COMPAT_SUPP	(DEDUPFS_FEATURE_RO_COMPAT_SPARSE_SUPER| \
					 DEDUPFS_FEATURE_RO_COMPAT_LARGE_FILE| \
					 DEDUPFS_FEATURE_RO_COMPAT_BTREE_DIR)

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
#define DEDUPFS_DEFM_JMODE		0x0060
#define DEDUPFS_DEFM_JMODE_DATA	0x0020
#define DEDUPFS_DEFM_JMODE_ORDERED	0x0040
#define DEDUPFS_DEFM_JMODE_WBACK	0x0060

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
 * Ext3 directory file types.  Only the low 3 bits are used.  The
 * other bits are reserved for now.
 */
#define DEDUPFS_FT_UNKNOWN		0
#define DEDUPFS_FT_REG_FILE	1
#define DEDUPFS_FT_DIR		2
#define DEDUPFS_FT_CHRDEV		3
#define DEDUPFS_FT_BLKDEV		4
#define DEDUPFS_FT_FIFO		5
#define DEDUPFS_FT_SOCK		6
#define DEDUPFS_FT_SYMLINK		7

#define DEDUPFS_FT_MAX		8

/*
 * DEDUPFS_DIR_PAD defines the directory entries boundaries
 *
 * NOTE: It must be a multiple of 4
 */
#define DEDUPFS_DIR_PAD			4
#define DEDUPFS_DIR_ROUND			(DEDUPFS_DIR_PAD - 1)
#define DEDUPFS_DIR_REC_LEN(name_len)	(((name_len) + 8 + DEDUPFS_DIR_ROUND) & \
					 ~DEDUPFS_DIR_ROUND)
#define DEDUPFS_MAX_REC_LEN		((1<<16)-1)

static inline unsigned dedupfs_rec_len_from_disk(__le16 dlen)
{
	unsigned len = le16_to_cpu(dlen);

	if (len == DEDUPFS_MAX_REC_LEN)
		return 1 << 16;
	return len;
}

static inline __le16 dedupfs_rec_len_to_disk(unsigned len)
{
	if (len == (1 << 16))
		return cpu_to_le16(DEDUPFS_MAX_REC_LEN);
	else if (len > (1 << 16))
		BUG();
	return cpu_to_le16(len);
}

/*
 * Hash Tree Directory indexing
 * (c) Daniel Phillips, 2001
 */

#define is_dx(dir) (DEDUPFS_HAS_COMPAT_FEATURE(dir->i_sb, \
				      DEDUPFS_FEATURE_COMPAT_DIR_INDEX) && \
		      (DEDUPFS_I(dir)->i_flags & DEDUPFS_INDEX_FL))
#define DEDUPFS_DIR_LINK_MAX(dir) (!is_dx(dir) && (dir)->i_nlink >= DEDUPFS_LINK_MAX)
#define DEDUPFS_DIR_LINK_EMPTY(dir) ((dir)->i_nlink == 2 || (dir)->i_nlink == 1)

/* Legal values for the dx_root hash_version field: */

#define DX_HASH_LEGACY		0
#define DX_HASH_HALF_MD4	1
#define DX_HASH_TEA		2
#define DX_HASH_LEGACY_UNSIGNED	3
#define DX_HASH_HALF_MD4_UNSIGNED	4
#define DX_HASH_TEA_UNSIGNED		5

#ifdef __KERNEL__

/* hash info structure used by the directory hash */
struct dx_hash_info
{
	u32		hash;
	u32		minor_hash;
	int		hash_version;
	u32		*seed;
};

#define DEDUPFS_HTREE_EOF	0x7fffffff

/*
 * Control parameters used by dedupfs_htree_next_block
 */
#define HASH_NB_ALWAYS		1


/*
 * Describe an inode's exact location on disk and in memory
 */
struct dedupfs_iloc
{
	struct buffer_head *bh;
	unsigned long offset;
	unsigned long block_group;
};

static inline struct dedupfs_inode *dedupfs_raw_inode(struct dedupfs_iloc *iloc)
{
	return (struct dedupfs_inode *) (iloc->bh->b_data + iloc->offset);
}

/*
 * This structure is stuffed into the struct file's private_data field
 * for directories.  It is where we put information so that we can do
 * readdir operations in hash tree order.
 */
struct dir_private_info {
	struct rb_root	root;
	struct rb_node	*curr_node;
	struct fname	*extra_fname;
	loff_t		last_pos;
	__u32		curr_hash;
	__u32		curr_minor_hash;
	__u32		next_hash;
};

/* calculate the first block number of the group */
static inline dedupfsblk_t
dedupfs_group_first_block_no(struct super_block *sb, unsigned long group_no)
{
	return group_no * (dedupfsblk_t)DEDUPFS_BLOCKS_PER_GROUP(sb) +
		le32_to_cpu(DEDUPFS_SB(sb)->s_es->s_first_data_block);
}

/*
 * Special error return code only used by dx_probe() and its callers.
 */
#define ERR_BAD_DX_DIR	-75000

/*
 * Function prototypes
 */

/*
 * Ok, these declarations are also in <linux/kernel.h> but none of the
 * dedupfs source programs needs to include it so they are duplicated here.
 */
# define NORET_TYPE    /**/
# define ATTRIB_NORET  __attribute__((noreturn))
# define NORET_AND     noreturn,

/* balloc.c */
extern int dedupfs_bg_has_super(struct super_block *sb, int group);
extern unsigned long dedupfs_bg_num_gdb(struct super_block *sb, int group);
extern dedupfsblk_t dedupfs_new_block (handle_t *handle, struct inode *inode,
			dedupfsblk_t goal, int *errp);
extern dedupfsblk_t dedupfs_new_blocks (handle_t *handle, struct inode *inode,
			dedupfsblk_t goal, unsigned long *count, int *errp);
extern void dedupfs_free_blocks (handle_t *handle, struct inode *inode,
			dedupfsblk_t block, unsigned long count);
extern void dedupfs_free_blocks_sb (handle_t *handle, struct super_block *sb,
				 dedupfsblk_t block, unsigned long count,
				unsigned long *pdquot_freed_blocks);
extern dedupfsblk_t dedupfs_count_free_blocks (struct super_block *);
extern void dedupfs_check_blocks_bitmap (struct super_block *);
extern struct dedupfs_group_desc * dedupfs_get_group_desc(struct super_block * sb,
						    unsigned int block_group,
						    struct buffer_head ** bh);
extern int dedupfs_should_retry_alloc(struct super_block *sb, int *retries);
extern void dedupfs_init_block_alloc_info(struct inode *);
extern void dedupfs_rsv_window_add(struct super_block *sb, struct dedupfs_reserve_window_node *rsv);

/* dir.c */
extern int dedupfs_check_dir_entry(const char *, struct inode *,
				struct dedupfs_dir_entry_2 *,
				struct buffer_head *, unsigned long);
extern int dedupfs_htree_store_dirent(struct file *dir_file, __u32 hash,
				    __u32 minor_hash,
				    struct dedupfs_dir_entry_2 *dirent);
extern void dedupfs_htree_free_dir_info(struct dir_private_info *p);

/* fsync.c */
extern int dedupfs_sync_file(struct file *, int);

/* hash.c */
extern int dedupfs_dirhash(const char *name, int len, struct
			  dx_hash_info *hinfo);

/* ialloc.c */
extern struct inode * dedupfs_new_inode (handle_t *, struct inode *, int);
extern void dedupfs_free_inode (handle_t *, struct inode *);
extern struct inode * dedupfs_orphan_get (struct super_block *, unsigned long);
extern unsigned long dedupfs_count_free_inodes (struct super_block *);
extern unsigned long dedupfs_count_dirs (struct super_block *);
extern void dedupfs_check_inodes_bitmap (struct super_block *);
extern unsigned long dedupfs_count_free (struct buffer_head *, unsigned);


/* inode.c */
int dedupfs_forget(handle_t *handle, int is_metadata, struct inode *inode,
		struct buffer_head *bh, dedupfsblk_t blocknr);
struct buffer_head * dedupfs_getblk (handle_t *, struct inode *, long, int, int *);
struct buffer_head * dedupfs_bread (handle_t *, struct inode *, int, int, int *);
int dedupfs_get_blocks_handle(handle_t *handle, struct inode *inode,
	sector_t iblock, unsigned long maxblocks, struct buffer_head *bh_result,
	int create);

extern struct inode *dedupfs_iget(struct super_block *, unsigned long);
extern int  dedupfs_write_inode (struct inode *, struct writeback_control *);
extern int  dedupfs_setattr (struct dentry *, struct iattr *);
extern void dedupfs_evict_inode (struct inode *);
extern int  dedupfs_sync_inode (handle_t *, struct inode *);
extern void dedupfs_discard_reservation (struct inode *);
extern void dedupfs_dirty_inode(struct inode *);
extern int dedupfs_change_inode_journal_flag(struct inode *, int);
extern int dedupfs_get_inode_loc(struct inode *, struct dedupfs_iloc *);
extern int dedupfs_can_truncate(struct inode *inode);
extern void dedupfs_truncate (struct inode *);
extern void dedupfs_set_inode_flags(struct inode *);
extern void dedupfs_get_inode_flags(struct dedupfs_inode_info *);
extern void dedupfs_set_aops(struct inode *inode);
extern int dedupfs_fiemap(struct inode *inode, struct fiemap_extent_info *fieinfo,
		       u64 start, u64 len);

/* ioctl.c */
extern long dedupfs_ioctl(struct file *, unsigned int, unsigned long);
extern long dedupfs_compat_ioctl(struct file *, unsigned int, unsigned long);

/* namei.c */
extern int dedupfs_orphan_add(handle_t *, struct inode *);
extern int dedupfs_orphan_del(handle_t *, struct inode *);
extern int dedupfs_htree_fill_tree(struct file *dir_file, __u32 start_hash,
				__u32 start_minor_hash, __u32 *next_hash);

/* resize.c */
extern int dedupfs_group_add(struct super_block *sb,
				struct dedupfs_new_group_data *input);
extern int dedupfs_group_extend(struct super_block *sb,
				struct dedupfs_super_block *es,
				dedupfsblk_t n_blocks_count);

/* super.c */
extern void dedupfs_error (struct super_block *, const char *, const char *, ...)
	__attribute__ ((format (printf, 3, 4)));
extern void __dedupfs_std_error (struct super_block *, const char *, int);
extern void dedupfs_abort (struct super_block *, const char *, const char *, ...)
	__attribute__ ((format (printf, 3, 4)));
extern void dedupfs_warning (struct super_block *, const char *, const char *, ...)
	__attribute__ ((format (printf, 3, 4)));
extern void dedupfs_msg(struct super_block *, const char *, const char *, ...)
	__attribute__ ((format (printf, 3, 4)));
extern void dedupfs_update_dynamic_rev (struct super_block *sb);

#define dedupfs_std_error(sb, errno)				\
do {								\
	if ((errno))						\
		__dedupfs_std_error((sb), __func__, (errno));	\
} while (0)

/*
 * Inodes and files operations
 */

/* dir.c */
extern const struct file_operations dedupfs_dir_operations;

/* file.c */
extern const struct inode_operations dedupfs_file_inode_operations;
extern const struct file_operations dedupfs_file_operations;

/* namei.c */
extern const struct inode_operations dedupfs_dir_inode_operations;
extern const struct inode_operations dedupfs_special_inode_operations;

/* symlink.c */
extern const struct inode_operations dedupfs_symlink_inode_operations;
extern const struct inode_operations dedupfs_fast_symlink_inode_operations;


#endif	/* __KERNEL__ */

#endif	/* _LINUX_DEDUPFS_H */
