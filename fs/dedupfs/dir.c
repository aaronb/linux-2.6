/*
 *  linux/fs/dedupfs/dir.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/dir.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  dedupfs directory handling functions
 *
 *  Big-endian to little-endian byte-swapping/bitmaps by
 *        David S. Miller (davem@caip.rutgers.edu), 1995
 *
 * All code that works with directory layout had been switched to pagecache
 * and moved here. AV
 */

#include "dedupfs.h"
#include <linux/buffer_head.h>
#include <linux/pagemap.h>
#include <linux/swap.h>

typedef struct dedupfs_dir_entry_2 dedupfs_dirent;

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
	else
		BUG_ON(len > (1 << 16));
	return cpu_to_le16(len);
}

/*
 * dedupfs uses block-sized chunks. Arguably, sector-sized ones would be
 * more robust, but we have what we have
 */
static inline unsigned dedupfs_chunk_size(struct inode *inode)
{
	return inode->i_sb->s_blocksize;
}

static inline void dedupfs_put_page(struct page *page)
{
	kunmap(page);
	page_cache_release(page);
}

static inline unsigned long dir_pages(struct inode *inode)
{
	return (inode->i_size+PAGE_CACHE_SIZE-1)>>PAGE_CACHE_SHIFT;
}

/*
 * Return the offset into page `page_nr' of the last valid
 * byte in that page, plus one.
 */
static unsigned
dedupfs_last_byte(struct inode *inode, unsigned long page_nr)
{
	unsigned last_byte = inode->i_size;

	last_byte -= page_nr << PAGE_CACHE_SHIFT;
	if (last_byte > PAGE_CACHE_SIZE)
		last_byte = PAGE_CACHE_SIZE;
	return last_byte;
}

static int dedupfs_commit_chunk(struct page *page, loff_t pos, unsigned len)
{
	struct address_space *mapping = page->mapping;
	struct inode *dir = mapping->host;
	int err = 0;

	dir->i_version++;
	block_write_end(NULL, mapping, pos, len, len, page, NULL);

	if (pos+len > dir->i_size) {
		i_size_write(dir, pos+len);
		mark_inode_dirty(dir);
	}

	if (IS_DIRSYNC(dir)) {
		err = write_one_page(page, 1);
		if (!err)
			err = dedupfs_sync_inode(dir);
	} else {
		unlock_page(page);
	}

	return err;
}

static void dedupfs_check_page(struct page *page, int quiet)
{
	struct inode *dir = page->mapping->host;
	struct super_block *sb = dir->i_sb;
	unsigned chunk_size = dedupfs_chunk_size(dir);
	char *kaddr = page_address(page);
	u32 max_inumber = le32_to_cpu(DEDUPFS_SB(sb)->s_es->s_inodes_count);
	unsigned offs, rec_len;
	unsigned limit = PAGE_CACHE_SIZE;
	dedupfs_dirent *p;
	char *error;

	if ((dir->i_size >> PAGE_CACHE_SHIFT) == page->index) {
		limit = dir->i_size & ~PAGE_CACHE_MASK;
		if (limit & (chunk_size - 1))
			goto Ebadsize;
		if (!limit)
			goto out;
	}
	for (offs = 0; offs <= limit - DEDUPFS_DIR_REC_LEN(1); offs += rec_len) {
		p = (dedupfs_dirent *)(kaddr + offs);
		rec_len = dedupfs_rec_len_from_disk(p->rec_len);

		if (rec_len < DEDUPFS_DIR_REC_LEN(1))
			goto Eshort;
		if (rec_len & 3)
			goto Ealign;
		if (rec_len < DEDUPFS_DIR_REC_LEN(p->name_len))
			goto Enamelen;
		if (((offs + rec_len - 1) ^ offs) & ~(chunk_size-1))
			goto Espan;
		if (le32_to_cpu(p->inode) > max_inumber)
			goto Einumber;
	}
	if (offs != limit)
		goto Eend;
out:
	SetPageChecked(page);
	return;

	/* Too bad, we had an error */

Ebadsize:
	if (!quiet)
		dedupfs_error(sb, __func__,
			"size of directory #%lu is not a multiple "
			"of chunk size", dir->i_ino);
	goto fail;
Eshort:
	error = "rec_len is smaller than minimal";
	goto bad_entry;
Ealign:
	error = "unaligned directory entry";
	goto bad_entry;
Enamelen:
	error = "rec_len is too small for name_len";
	goto bad_entry;
Espan:
	error = "directory entry across blocks";
	goto bad_entry;
Einumber:
	error = "inode out of bounds";
bad_entry:
	if (!quiet)
		dedupfs_error(sb, __func__, "bad entry in directory #%lu: : %s - "
			"offset=%lu, inode=%lu, rec_len=%d, name_len=%d",
			dir->i_ino, error, (page->index<<PAGE_CACHE_SHIFT)+offs,
			(unsigned long) le32_to_cpu(p->inode),
			rec_len, p->name_len);
	goto fail;
Eend:
	if (!quiet) {
		p = (dedupfs_dirent *)(kaddr + offs);
		dedupfs_error(sb, "dedupfs_check_page",
			"entry in directory #%lu spans the page boundary"
			"offset=%lu, inode=%lu",
			dir->i_ino, (page->index<<PAGE_CACHE_SHIFT)+offs,
			(unsigned long) le32_to_cpu(p->inode));
	}
fail:
	SetPageChecked(page);
	SetPageError(page);
}

static struct page * dedupfs_get_page(struct inode *dir, unsigned long n,
				   int quiet)
{
	struct address_space *mapping = dir->i_mapping;
	struct page *page = read_mapping_page(mapping, n, NULL);
	if (!IS_ERR(page)) {
		kmap(page);
		if (!PageChecked(page))
			dedupfs_check_page(page, quiet);
		if (PageError(page))
			goto fail;
	}
	return page;

fail:
	dedupfs_put_page(page);
	return ERR_PTR(-EIO);
}

/*
 * NOTE! unlike strncmp, dedupfs_match returns 1 for success, 0 for failure.
 *
 * len <= DEDUPFS_NAME_LEN and de != NULL are guaranteed by caller.
 */
static inline int dedupfs_match (int len, const char * const name,
					struct dedupfs_dir_entry_2 * de)
{
	if (len != de->name_len)
		return 0;
	if (!de->inode)
		return 0;
	return !memcmp(name, de->name, len);
}

/*
 * p is at least 6 bytes before the end of page
 */
static inline dedupfs_dirent *dedupfs_next_entry(dedupfs_dirent *p)
{
	return (dedupfs_dirent *)((char *)p +
			dedupfs_rec_len_from_disk(p->rec_len));
}

static inline unsigned 
dedupfs_validate_entry(char *base, unsigned offset, unsigned mask)
{
	dedupfs_dirent *de = (dedupfs_dirent*)(base + offset);
	dedupfs_dirent *p = (dedupfs_dirent*)(base + (offset&mask));
	while ((char*)p < (char*)de) {
		if (p->rec_len == 0)
			break;
		p = dedupfs_next_entry(p);
	}
	return (char *)p - base;
}

static unsigned char dedupfs_filetype_table[DEDUPFS_FT_MAX] = {
	[DEDUPFS_FT_UNKNOWN]	= DT_UNKNOWN,
	[DEDUPFS_FT_REG_FILE]	= DT_REG,
	[DEDUPFS_FT_DIR]		= DT_DIR,
	[DEDUPFS_FT_CHRDEV]	= DT_CHR,
	[DEDUPFS_FT_BLKDEV]	= DT_BLK,
	[DEDUPFS_FT_FIFO]		= DT_FIFO,
	[DEDUPFS_FT_SOCK]		= DT_SOCK,
	[DEDUPFS_FT_SYMLINK]	= DT_LNK,
};

#define S_SHIFT 12
static unsigned char dedupfs_type_by_mode[S_IFMT >> S_SHIFT] = {
	[S_IFREG >> S_SHIFT]	= DEDUPFS_FT_REG_FILE,
	[S_IFDIR >> S_SHIFT]	= DEDUPFS_FT_DIR,
	[S_IFCHR >> S_SHIFT]	= DEDUPFS_FT_CHRDEV,
	[S_IFBLK >> S_SHIFT]	= DEDUPFS_FT_BLKDEV,
	[S_IFIFO >> S_SHIFT]	= DEDUPFS_FT_FIFO,
	[S_IFSOCK >> S_SHIFT]	= DEDUPFS_FT_SOCK,
	[S_IFLNK >> S_SHIFT]	= DEDUPFS_FT_SYMLINK,
};

static inline void dedupfs_set_de_type(dedupfs_dirent *de, struct inode *inode)
{
	mode_t mode = inode->i_mode;
	if (DEDUPFS_HAS_INCOMPAT_FEATURE(inode->i_sb, DEDUPFS_FEATURE_INCOMPAT_FILETYPE))
		de->file_type = dedupfs_type_by_mode[(mode & S_IFMT)>>S_SHIFT];
	else
		de->file_type = 0;
}

static int
dedupfs_readdir (struct file * filp, void * dirent, filldir_t filldir)
{
	loff_t pos = filp->f_pos;
	struct inode *inode = filp->f_path.dentry->d_inode;
	struct super_block *sb = inode->i_sb;
	unsigned int offset = pos & ~PAGE_CACHE_MASK;
	unsigned long n = pos >> PAGE_CACHE_SHIFT;
	unsigned long npages = dir_pages(inode);
	unsigned chunk_mask = ~(dedupfs_chunk_size(inode)-1);
	unsigned char *types = NULL;
	int need_revalidate = filp->f_version != inode->i_version;

	if (pos > inode->i_size - DEDUPFS_DIR_REC_LEN(1))
		return 0;

	if (DEDUPFS_HAS_INCOMPAT_FEATURE(sb, DEDUPFS_FEATURE_INCOMPAT_FILETYPE))
		types = dedupfs_filetype_table;

	for ( ; n < npages; n++, offset = 0) {
		char *kaddr, *limit;
		dedupfs_dirent *de;
		struct page *page = dedupfs_get_page(inode, n, 0);

		if (IS_ERR(page)) {
			dedupfs_error(sb, __func__,
				   "bad page in #%lu",
				   inode->i_ino);
			filp->f_pos += PAGE_CACHE_SIZE - offset;
			return PTR_ERR(page);
		}
		kaddr = page_address(page);
		if (unlikely(need_revalidate)) {
			if (offset) {
				offset = dedupfs_validate_entry(kaddr, offset, chunk_mask);
				filp->f_pos = (n<<PAGE_CACHE_SHIFT) + offset;
			}
			filp->f_version = inode->i_version;
			need_revalidate = 0;
		}
		de = (dedupfs_dirent *)(kaddr+offset);
		limit = kaddr + dedupfs_last_byte(inode, n) - DEDUPFS_DIR_REC_LEN(1);
		for ( ;(char*)de <= limit; de = dedupfs_next_entry(de)) {
			if (de->rec_len == 0) {
				dedupfs_error(sb, __func__,
					"zero-length directory entry");
				dedupfs_put_page(page);
				return -EIO;
			}
			if (de->inode) {
				int over;
				unsigned char d_type = DT_UNKNOWN;

				if (types && de->file_type < DEDUPFS_FT_MAX)
					d_type = types[de->file_type];

				offset = (char *)de - kaddr;
				over = filldir(dirent, de->name, de->name_len,
						(n<<PAGE_CACHE_SHIFT) | offset,
						le32_to_cpu(de->inode), d_type);
				if (over) {
					dedupfs_put_page(page);
					return 0;
				}
			}
			filp->f_pos += dedupfs_rec_len_from_disk(de->rec_len);
		}
		dedupfs_put_page(page);
	}
	return 0;
}

/*
 *	dedupfs_find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the page in which the entry was found (as a parameter - res_page),
 * and the entry itself. Page is returned mapped and unlocked.
 * Entry is guaranteed to be valid.
 */
struct dedupfs_dir_entry_2 *dedupfs_find_entry (struct inode * dir,
			struct qstr *child, struct page ** res_page)
{
	const char *name = child->name;
	int namelen = child->len;
	unsigned reclen = DEDUPFS_DIR_REC_LEN(namelen);
	unsigned long start, n;
	unsigned long npages = dir_pages(dir);
	struct page *page = NULL;
	struct dedupfs_inode_info *ei = DEDUPFS_I(dir);
	dedupfs_dirent * de;
	int dir_has_error = 0;

	if (npages == 0)
		goto out;

	/* OFFSET_CACHE */
	*res_page = NULL;

	start = ei->i_dir_start_lookup;
	if (start >= npages)
		start = 0;
	n = start;
	do {
		char *kaddr;
		page = dedupfs_get_page(dir, n, dir_has_error);
		if (!IS_ERR(page)) {
			kaddr = page_address(page);
			de = (dedupfs_dirent *) kaddr;
			kaddr += dedupfs_last_byte(dir, n) - reclen;
			while ((char *) de <= kaddr) {
				if (de->rec_len == 0) {
					dedupfs_error(dir->i_sb, __func__,
						"zero-length directory entry");
					dedupfs_put_page(page);
					goto out;
				}
				if (dedupfs_match (namelen, name, de))
					goto found;
				de = dedupfs_next_entry(de);
			}
			dedupfs_put_page(page);
		} else
			dir_has_error = 1;

		if (++n >= npages)
			n = 0;
		/* next page is past the blocks we've got */
		if (unlikely(n > (dir->i_blocks >> (PAGE_CACHE_SHIFT - 9)))) {
			dedupfs_error(dir->i_sb, __func__,
				"dir %lu size %lld exceeds block count %llu",
				dir->i_ino, dir->i_size,
				(unsigned long long)dir->i_blocks);
			goto out;
		}
	} while (n != start);
out:
	return NULL;

found:
	*res_page = page;
	ei->i_dir_start_lookup = n;
	return de;
}

struct dedupfs_dir_entry_2 * dedupfs_dotdot (struct inode *dir, struct page **p)
{
	struct page *page = dedupfs_get_page(dir, 0, 0);
	dedupfs_dirent *de = NULL;

	if (!IS_ERR(page)) {
		de = dedupfs_next_entry((dedupfs_dirent *) page_address(page));
		*p = page;
	}
	return de;
}

ino_t dedupfs_inode_by_name(struct inode *dir, struct qstr *child)
{
	ino_t res = 0;
	struct dedupfs_dir_entry_2 *de;
	struct page *page;
	
	de = dedupfs_find_entry (dir, child, &page);
	if (de) {
		res = le32_to_cpu(de->inode);
		dedupfs_put_page(page);
	}
	return res;
}

static int dedupfs_prepare_chunk(struct page *page, loff_t pos, unsigned len)
{
	return __block_write_begin(page, pos, len, dedupfs_get_block);
}

/* Releases the page */
void dedupfs_set_link(struct inode *dir, struct dedupfs_dir_entry_2 *de,
		   struct page *page, struct inode *inode, int update_times)
{
	loff_t pos = page_offset(page) +
			(char *) de - (char *) page_address(page);
	unsigned len = dedupfs_rec_len_from_disk(de->rec_len);
	int err;

	lock_page(page);
	err = dedupfs_prepare_chunk(page, pos, len);
	BUG_ON(err);
	de->inode = cpu_to_le32(inode->i_ino);
	dedupfs_set_de_type(de, inode);
	err = dedupfs_commit_chunk(page, pos, len);
	dedupfs_put_page(page);
	if (update_times)
		dir->i_mtime = dir->i_ctime = CURRENT_TIME_SEC;
	DEDUPFS_I(dir)->i_flags &= ~DEDUPFS_BTREE_FL;
	mark_inode_dirty(dir);
}

/*
 *	Parent is locked.
 */
int dedupfs_add_link (struct dentry *dentry, struct inode *inode)
{
	struct inode *dir = dentry->d_parent->d_inode;
	const char *name = dentry->d_name.name;
	int namelen = dentry->d_name.len;
	unsigned chunk_size = dedupfs_chunk_size(dir);
	unsigned reclen = DEDUPFS_DIR_REC_LEN(namelen);
	unsigned short rec_len, name_len;
	struct page *page = NULL;
	dedupfs_dirent * de;
	unsigned long npages = dir_pages(dir);
	unsigned long n;
	char *kaddr;
	loff_t pos;
	int err;

	/*
	 * We take care of directory expansion in the same loop.
	 * This code plays outside i_size, so it locks the page
	 * to protect that region.
	 */
	for (n = 0; n <= npages; n++) {
		char *dir_end;

		page = dedupfs_get_page(dir, n, 0);
		err = PTR_ERR(page);
		if (IS_ERR(page))
			goto out;
		lock_page(page);
		kaddr = page_address(page);
		dir_end = kaddr + dedupfs_last_byte(dir, n);
		de = (dedupfs_dirent *)kaddr;
		kaddr += PAGE_CACHE_SIZE - reclen;
		while ((char *)de <= kaddr) {
			if ((char *)de == dir_end) {
				/* We hit i_size */
				name_len = 0;
				rec_len = chunk_size;
				de->rec_len = dedupfs_rec_len_to_disk(chunk_size);
				de->inode = 0;
				goto got_it;
			}
			if (de->rec_len == 0) {
				dedupfs_error(dir->i_sb, __func__,
					"zero-length directory entry");
				err = -EIO;
				goto out_unlock;
			}
			err = -EEXIST;
			if (dedupfs_match (namelen, name, de))
				goto out_unlock;
			name_len = DEDUPFS_DIR_REC_LEN(de->name_len);
			rec_len = dedupfs_rec_len_from_disk(de->rec_len);
			if (!de->inode && rec_len >= reclen)
				goto got_it;
			if (rec_len >= name_len + reclen)
				goto got_it;
			de = (dedupfs_dirent *) ((char *) de + rec_len);
		}
		unlock_page(page);
		dedupfs_put_page(page);
	}
	BUG();
	return -EINVAL;

got_it:
	pos = page_offset(page) +
		(char*)de - (char*)page_address(page);
	err = dedupfs_prepare_chunk(page, pos, rec_len);
	if (err)
		goto out_unlock;
	if (de->inode) {
		dedupfs_dirent *de1 = (dedupfs_dirent *) ((char *) de + name_len);
		de1->rec_len = dedupfs_rec_len_to_disk(rec_len - name_len);
		de->rec_len = dedupfs_rec_len_to_disk(name_len);
		de = de1;
	}
	de->name_len = namelen;
	memcpy(de->name, name, namelen);
	de->inode = cpu_to_le32(inode->i_ino);
	dedupfs_set_de_type (de, inode);
	err = dedupfs_commit_chunk(page, pos, rec_len);
	dir->i_mtime = dir->i_ctime = CURRENT_TIME_SEC;
	DEDUPFS_I(dir)->i_flags &= ~DEDUPFS_BTREE_FL;
	mark_inode_dirty(dir);
	/* OFFSET_CACHE */
out_put:
	dedupfs_put_page(page);
out:
	return err;
out_unlock:
	unlock_page(page);
	goto out_put;
}

/*
 * dedupfs_delete_entry deletes a directory entry by merging it with the
 * previous entry. Page is up-to-date. Releases the page.
 */
int dedupfs_delete_entry (struct dedupfs_dir_entry_2 * dir, struct page * page )
{
	struct inode *inode = page->mapping->host;
	char *kaddr = page_address(page);
	unsigned from = ((char*)dir - kaddr) & ~(dedupfs_chunk_size(inode)-1);
	unsigned to = ((char *)dir - kaddr) +
				dedupfs_rec_len_from_disk(dir->rec_len);
	loff_t pos;
	dedupfs_dirent * pde = NULL;
	dedupfs_dirent * de = (dedupfs_dirent *) (kaddr + from);
	int err;

	while ((char*)de < (char*)dir) {
		if (de->rec_len == 0) {
			dedupfs_error(inode->i_sb, __func__,
				"zero-length directory entry");
			err = -EIO;
			goto out;
		}
		pde = de;
		de = dedupfs_next_entry(de);
	}
	if (pde)
		from = (char*)pde - (char*)page_address(page);
	pos = page_offset(page) + from;
	lock_page(page);
	err = dedupfs_prepare_chunk(page, pos, to - from);
	BUG_ON(err);
	if (pde)
		pde->rec_len = dedupfs_rec_len_to_disk(to - from);
	dir->inode = 0;
	err = dedupfs_commit_chunk(page, pos, to - from);
	inode->i_ctime = inode->i_mtime = CURRENT_TIME_SEC;
	DEDUPFS_I(inode)->i_flags &= ~DEDUPFS_BTREE_FL;
	mark_inode_dirty(inode);
out:
	dedupfs_put_page(page);
	return err;
}

/*
 * Set the first fragment of directory.
 */
int dedupfs_make_empty(struct inode *inode, struct inode *parent)
{
	struct page *page = grab_cache_page(inode->i_mapping, 0);
	unsigned chunk_size = dedupfs_chunk_size(inode);
	struct dedupfs_dir_entry_2 * de;
	int err;
	void *kaddr;

	if (!page)
		return -ENOMEM;

	err = dedupfs_prepare_chunk(page, 0, chunk_size);
	if (err) {
		unlock_page(page);
		goto fail;
	}
	kaddr = kmap_atomic(page, KM_USER0);
	memset(kaddr, 0, chunk_size);
	de = (struct dedupfs_dir_entry_2 *)kaddr;
	de->name_len = 1;
	de->rec_len = dedupfs_rec_len_to_disk(DEDUPFS_DIR_REC_LEN(1));
	memcpy (de->name, ".\0\0", 4);
	de->inode = cpu_to_le32(inode->i_ino);
	dedupfs_set_de_type (de, inode);

	de = (struct dedupfs_dir_entry_2 *)(kaddr + DEDUPFS_DIR_REC_LEN(1));
	de->name_len = 2;
	de->rec_len = dedupfs_rec_len_to_disk(chunk_size - DEDUPFS_DIR_REC_LEN(1));
	de->inode = cpu_to_le32(parent->i_ino);
	memcpy (de->name, "..\0", 4);
	dedupfs_set_de_type (de, inode);
	kunmap_atomic(kaddr, KM_USER0);
	err = dedupfs_commit_chunk(page, 0, chunk_size);
fail:
	page_cache_release(page);
	return err;
}

/*
 * routine to check that the specified directory is empty (for rmdir)
 */
int dedupfs_empty_dir (struct inode * inode)
{
	struct page *page = NULL;
	unsigned long i, npages = dir_pages(inode);
	int dir_has_error = 0;

	for (i = 0; i < npages; i++) {
		char *kaddr;
		dedupfs_dirent * de;
		page = dedupfs_get_page(inode, i, dir_has_error);

		if (IS_ERR(page)) {
			dir_has_error = 1;
			continue;
		}

		kaddr = page_address(page);
		de = (dedupfs_dirent *)kaddr;
		kaddr += dedupfs_last_byte(inode, i) - DEDUPFS_DIR_REC_LEN(1);

		while ((char *)de <= kaddr) {
			if (de->rec_len == 0) {
				dedupfs_error(inode->i_sb, __func__,
					"zero-length directory entry");
				printk("kaddr=%p, de=%p\n", kaddr, de);
				goto not_empty;
			}
			if (de->inode != 0) {
				/* check for . and .. */
				if (de->name[0] != '.')
					goto not_empty;
				if (de->name_len > 2)
					goto not_empty;
				if (de->name_len < 2) {
					if (de->inode !=
					    cpu_to_le32(inode->i_ino))
						goto not_empty;
				} else if (de->name[1] != '.')
					goto not_empty;
			}
			de = dedupfs_next_entry(de);
		}
		dedupfs_put_page(page);
	}
	return 1;

not_empty:
	dedupfs_put_page(page);
	return 0;
}

const struct file_operations dedupfs_dir_operations = {
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.readdir	= dedupfs_readdir,
	.unlocked_ioctl = dedupfs_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= dedupfs_compat_ioctl,
#endif
	.fsync		= dedupfs_fsync,
};
