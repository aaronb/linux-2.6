#include <linux/fs.h>
//#include <linux/ext2_fs.h>
#include "dedupfs_include.h"

#define dedupfs_test_bit ext2_test_bit
#define dedupfs_find_next_zero_bit ext2_find_next_zero_bit
#define dedupfs_clear_bit_atomic ext2_clear_bit_atomic
#define dedupfs_set_bit_atomic ext2_set_bit_atomic
#define DEDUPFS_SUPER_MAGIC EXT2_SUPER_MAGIC

/*
 * dedupfs mount options
 */
struct dedupfs_mount_options {
	unsigned long s_mount_opt;
	uid_t s_resuid;
	gid_t s_resgid;
};

/*
 * second extended file system inode data in memory
 */
struct dedupfs_inode_info {
	__le32	i_data[15];
	__u32	i_flags;
	__u32	i_faddr;
	__u8	i_frag_no;
	__u8	i_frag_size;
	__u16	i_state;
	__u32	i_file_acl;
	__u32	i_dir_acl;
	__u32	i_dtime;

	/*
	 * i_block_group is the number of the block group which contains
	 * this file's inode.  Constant across the lifetime of the inode,
	 * it is used for making block allocation decisions - we try to
	 * place a file's data blocks near its inode block, and new inodes
	 * near to their parent directory's inode.
	 */
	__u32	i_block_group;

	/* block reservation info */
	struct dedupfs_block_alloc_info *i_block_alloc_info;

	__u32	i_dir_start_lookup;
#ifdef CONFIG_DEDUPFS_XATTR
	/*
	 * Extended attributes can be read independently of the main file
	 * data. Taking i_mutex even when reading would cause contention
	 * between readers of EAs and writers of regular file data, so
	 * instead we synchronize on xattr_sem when reading or changing
	 * EAs.
	 */
	struct rw_semaphore xattr_sem;
#endif
	rwlock_t i_meta_lock;

	/*
	 * truncate_mutex is for serialising dedupfs_truncate() against
	 * dedupfs_getblock().  It also protects the internals of the inode's
	 * reservation data structures: dedupfs_reserve_window and
	 * dedupfs_reserve_window_node.
	 */
	struct mutex truncate_mutex;
	struct inode	vfs_inode;
	struct list_head i_orphan;	/* unlinked but open inodes */
};

/*
 * Inode dynamic state flags
 */
#define DEDUPFS_STATE_NEW			0x00000001 /* inode is newly created */


/*
 * Function prototypes
 */

/*
 * Ok, these declarations are also in <linux/kernel.h> but none of the
 * dedupfs source programs needs to include it so they are duplicated here.
 */

static inline struct dedupfs_inode_info *DEDUPFS_I(struct inode *inode)
{
	return container_of(inode, struct dedupfs_inode_info, vfs_inode);
}

/* balloc.c */
extern int dedupfs_bg_has_super(struct super_block *sb, int group);
extern unsigned long dedupfs_bg_num_gdb(struct super_block *sb, int group);
extern dedupfs_fsblk_t dedupfs_new_block(struct inode *, unsigned long, int *);
extern dedupfs_fsblk_t dedupfs_new_blocks(struct inode *, unsigned long,
				unsigned long *, int *);
extern void dedupfs_free_blocks (struct inode *, unsigned long,
			      unsigned long);
extern unsigned long dedupfs_count_free_blocks (struct super_block *);
extern unsigned long dedupfs_count_dirs (struct super_block *);
extern void dedupfs_check_blocks_bitmap (struct super_block *);
extern struct dedupfs_group_desc * dedupfs_get_group_desc(struct super_block * sb,
						    unsigned int block_group,
						    struct buffer_head ** bh);
extern void dedupfs_discard_reservation (struct inode *);
extern int dedupfs_should_retry_alloc(struct super_block *sb, int *retries);
extern void dedupfs_init_block_alloc_info(struct inode *);
extern void dedupfs_rsv_window_add(struct super_block *sb, struct dedupfs_reserve_window_node *rsv);

/* dir.c */
extern int dedupfs_add_link (struct dentry *, struct inode *);
extern ino_t dedupfs_inode_by_name(struct inode *, struct qstr *);
extern int dedupfs_make_empty(struct inode *, struct inode *);
extern struct dedupfs_dir_entry_2 * dedupfs_find_entry (struct inode *,struct qstr *, struct page **);
extern int dedupfs_delete_entry (struct dedupfs_dir_entry_2 *, struct page *);
extern int dedupfs_empty_dir (struct inode *);
extern struct dedupfs_dir_entry_2 * dedupfs_dotdot (struct inode *, struct page **);
extern void dedupfs_set_link(struct inode *, struct dedupfs_dir_entry_2 *, struct page *, struct inode *, int);

/* ialloc.c */
extern struct inode * dedupfs_new_inode (struct inode *, int);
extern void dedupfs_free_inode (struct inode *);
extern unsigned long dedupfs_count_free_inodes (struct super_block *);
extern void dedupfs_check_inodes_bitmap (struct super_block *);
extern unsigned long dedupfs_count_free (struct buffer_head *, unsigned);

/* inode.c */
extern struct inode *dedupfs_iget (struct super_block *, unsigned long);
extern int dedupfs_write_inode (struct inode *, struct writeback_control *);
extern void dedupfs_evict_inode(struct inode *);
extern int dedupfs_sync_inode (struct inode *);
extern int dedupfs_get_block(struct inode *, sector_t, struct buffer_head *, int);
extern int dedupfs_setattr (struct dentry *, struct iattr *);
extern void dedupfs_set_inode_flags(struct inode *inode);
extern void dedupfs_get_inode_flags(struct dedupfs_inode_info *);
extern int dedupfs_fiemap(struct inode *inode, struct fiemap_extent_info *fieinfo,
		       u64 start, u64 len);

/* ioctl.c */
extern long dedupfs_ioctl(struct file *, unsigned int, unsigned long);
extern long dedupfs_compat_ioctl(struct file *, unsigned int, unsigned long);

/* namei.c */
struct dentry *dedupfs_get_parent(struct dentry *child);

/* super.c */
extern void dedupfs_error (struct super_block *, const char *, const char *, ...)
	__attribute__ ((format (printf, 3, 4)));
extern void dedupfs_msg(struct super_block *, const char *, const char *, ...)
	__attribute__ ((format (printf, 3, 4)));
extern void dedupfs_update_dynamic_rev (struct super_block *sb);
extern void dedupfs_write_super (struct super_block *);

/*
 * Inodes and files operations
 */

/* dir.c */
extern const struct file_operations dedupfs_dir_operations;

/* file.c */
extern int dedupfs_fsync(struct file *file, int datasync);
extern const struct inode_operations dedupfs_file_inode_operations;
extern const struct file_operations dedupfs_file_operations;
extern const struct file_operations dedupfs_xip_file_operations;

/* inode.c */
extern const struct address_space_operations dedupfs_aops;
extern const struct address_space_operations dedupfs_aops_xip;
extern const struct address_space_operations dedupfs_nobh_aops;

/* namei.c */
extern const struct inode_operations dedupfs_dir_inode_operations;
extern const struct inode_operations dedupfs_special_inode_operations;

/* symlink.c */
extern const struct inode_operations dedupfs_fast_symlink_inode_operations;
extern const struct inode_operations dedupfs_symlink_inode_operations;

static inline dedupfs_fsblk_t
dedupfs_group_first_block_no(struct super_block *sb, unsigned long group_no)
{
	return group_no * (dedupfs_fsblk_t)DEDUPFS_BLOCKS_PER_GROUP(sb) +
		le32_to_cpu(DEDUPFS_SB(sb)->s_es->s_first_data_block);
}
