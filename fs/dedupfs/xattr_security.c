/*
 * linux/fs/dedupfs/xattr_security.c
 * Handler for storing security labels as extended attributes.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/dedupfs_fs.h>
#include <linux/security.h>
#include "xattr.h"

static size_t
dedupfs_xattr_security_list(struct dentry *dentry, char *list, size_t list_size,
			 const char *name, size_t name_len, int type)
{
	const int prefix_len = XATTR_SECURITY_PREFIX_LEN;
	const size_t total_len = prefix_len + name_len + 1;

	if (list && total_len <= list_size) {
		memcpy(list, XATTR_SECURITY_PREFIX, prefix_len);
		memcpy(list+prefix_len, name, name_len);
		list[prefix_len + name_len] = '\0';
	}
	return total_len;
}

static int
dedupfs_xattr_security_get(struct dentry *dentry, const char *name,
		       void *buffer, size_t size, int type)
{
	if (strcmp(name, "") == 0)
		return -EINVAL;
	return dedupfs_xattr_get(dentry->d_inode, EXT2_XATTR_INDEX_SECURITY, name,
			      buffer, size);
}

static int
dedupfs_xattr_security_set(struct dentry *dentry, const char *name,
		const void *value, size_t size, int flags, int type)
{
	if (strcmp(name, "") == 0)
		return -EINVAL;
	return dedupfs_xattr_set(dentry->d_inode, EXT2_XATTR_INDEX_SECURITY, name,
			      value, size, flags);
}

int
dedupfs_init_security(struct inode *inode, struct inode *dir)
{
	int err;
	size_t len;
	void *value;
	char *name;

	err = security_inode_init_security(inode, dir, &name, &value, &len);
	if (err) {
		if (err == -EOPNOTSUPP)
			return 0;
		return err;
	}
	err = dedupfs_xattr_set(inode, EXT2_XATTR_INDEX_SECURITY,
			     name, value, len, 0);
	kfree(name);
	kfree(value);
	return err;
}

const struct xattr_handler dedupfs_xattr_security_handler = {
	.prefix	= XATTR_SECURITY_PREFIX,
	.list	= dedupfs_xattr_security_list,
	.get	= dedupfs_xattr_security_get,
	.set	= dedupfs_xattr_security_set,
};
