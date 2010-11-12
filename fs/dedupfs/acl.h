/*
  File: fs/dedupfs/acl.h

  (C) 2001 Andreas Gruenbacher, <a.gruenbacher@computer.org>
*/

#include <linux/posix_acl_xattr.h>

#define DEDUPFS_ACL_VERSION	0x0001

typedef struct {
	__le16		e_tag;
	__le16		e_perm;
	__le32		e_id;
} dedupfs_acl_entry;

typedef struct {
	__le16		e_tag;
	__le16		e_perm;
} dedupfs_acl_entry_short;

typedef struct {
	__le32		a_version;
} dedupfs_acl_header;

static inline size_t dedupfs_acl_size(int count)
{
	if (count <= 4) {
		return sizeof(dedupfs_acl_header) +
		       count * sizeof(dedupfs_acl_entry_short);
	} else {
		return sizeof(dedupfs_acl_header) +
		       4 * sizeof(dedupfs_acl_entry_short) +
		       (count - 4) * sizeof(dedupfs_acl_entry);
	}
}

static inline int dedupfs_acl_count(size_t size)
{
	ssize_t s;
	size -= sizeof(dedupfs_acl_header);
	s = size - 4 * sizeof(dedupfs_acl_entry_short);
	if (s < 0) {
		if (size % sizeof(dedupfs_acl_entry_short))
			return -1;
		return size / sizeof(dedupfs_acl_entry_short);
	} else {
		if (s % sizeof(dedupfs_acl_entry))
			return -1;
		return s / sizeof(dedupfs_acl_entry) + 4;
	}
}

#ifdef CONFIG_DEDUPFS_POSIX_ACL

/* acl.c */
extern int dedupfs_check_acl (struct inode *, int);
extern int dedupfs_acl_chmod (struct inode *);
extern int dedupfs_init_acl (handle_t *, struct inode *, struct inode *);

#else  /* CONFIG_DEDUPFS_POSIX_ACL */
#include <linux/sched.h>
#define dedupfs_check_acl NULL

static inline int
dedupfs_acl_chmod(struct inode *inode)
{
	return 0;
}

static inline int
dedupfs_init_acl(handle_t *handle, struct inode *inode, struct inode *dir)
{
	return 0;
}
#endif  /* CONFIG_DEDUPFS_POSIX_ACL */

