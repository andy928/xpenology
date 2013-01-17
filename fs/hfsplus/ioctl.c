/*
 *  linux/fs/hfsplus/ioctl.c
 *
 * Copyright (C) 2003
 * Ethan Benson <erbenson@alaska.net>
 * partially derived from linux/fs/ext2/ioctl.c
 * Copyright (C) 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 * hfsplus ioctls
 */

#include <linux/capability.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/sched.h>
#include <linux/xattr.h>
#include <asm/uaccess.h>
#include "hfsplus_fs.h"

#ifdef MY_ABC_HERE
#define SZ_XATTR_NAME_RFORK			"com.apple.ResourceFork"
#define SZ_XATTR_NAME_FINDRINFO		"com.apple.FinderInfo"
#define SZ_XATTR_NAME_TYPE			"hfs.type"
#define SZ_XATTR_NAME_CREATOR		"hfs.creator"

// compare the cnid first, then attrName, finally start_block (for extent)
int hfsplus_attr_cmp_key(const hfsplus_btree_key *key1, const hfsplus_btree_key *key2)
{
	int retval = 0;

	retval = be32_to_cpu(key1->attr.file_id) - be32_to_cpu(key2->attr.file_id);
	if (retval) {
		return retval < 0 ? -1 : 1;
	}
	// treat empty name as any match
	if (be16_to_cpu(key1->attr.name.length) * be16_to_cpu(key2->attr.name.length) == 0) {
		return 0;
	}
	retval = hfsplus_strcmp((struct hfsplus_unistr *)&key1->attr.name, (struct hfsplus_unistr *)&key2->attr.name);

	return retval;
}

void hfsplus_attr_build_key(struct super_block *sb, hfsplus_btree_key *key, hfsplus_cnid file_id, const char *name, u32 start_block)
{
	int len;

	key->attr.pad = 0;
	key->attr.file_id = file_id;
	key->attr.start_block = cpu_to_be32(start_block);
	if (name) {
		hfsplus_asc2uni_ex(sb, (struct hfsplus_unistr *)&key->attr.name, name, strlen(name), 0);
		len = be16_to_cpu(key->attr.name.length);
	} else {
		key->attr.name.length = 0;
		len = 0;
	}
	key->key_len = cpu_to_be16(HFSPLUS_ATTR_KEYLEN_MIN + 2*len);
}
#endif

static int hfsplus_ioctl_getflags(struct file *file, int __user *user_flags)
{
	struct inode *inode = file->f_path.dentry->d_inode;
	struct hfsplus_inode_info *hip = HFSPLUS_I(inode);
	unsigned int flags = 0;

	if (inode->i_flags & S_IMMUTABLE)
		flags |= FS_IMMUTABLE_FL;
	if (inode->i_flags & S_APPEND)
		flags |= FS_APPEND_FL;
	if (hip->userflags & HFSPLUS_FLG_NODUMP)
		flags |= FS_NODUMP_FL;

	return put_user(flags, user_flags);
}

static int hfsplus_ioctl_setflags(struct file *file, int __user *user_flags)
{
	struct inode *inode = file->f_path.dentry->d_inode;
	struct hfsplus_inode_info *hip = HFSPLUS_I(inode);
	unsigned int flags;
	int err = 0;

	err = mnt_want_write(file->f_path.mnt);
	if (err)
		goto out;

	if (!inode_owner_or_capable(inode)) {
		err = -EACCES;
		goto out_drop_write;
	}

	if (get_user(flags, user_flags)) {
		err = -EFAULT;
		goto out_drop_write;
	}

	mutex_lock(&inode->i_mutex);

	if ((flags & (FS_IMMUTABLE_FL|FS_APPEND_FL)) ||
	    inode->i_flags & (S_IMMUTABLE|S_APPEND)) {
		if (!capable(CAP_LINUX_IMMUTABLE)) {
			err = -EPERM;
			goto out_unlock_inode;
		}
	}

	/* don't silently ignore unsupported ext2 flags */
	if (flags & ~(FS_IMMUTABLE_FL|FS_APPEND_FL|FS_NODUMP_FL)) {
		err = -EOPNOTSUPP;
		goto out_unlock_inode;
	}

	if (flags & FS_IMMUTABLE_FL)
		inode->i_flags |= S_IMMUTABLE;
	else
		inode->i_flags &= ~S_IMMUTABLE;

	if (flags & FS_APPEND_FL)
		inode->i_flags |= S_APPEND;
	else
		inode->i_flags &= ~S_APPEND;

	if (flags & FS_NODUMP_FL)
		hip->userflags |= HFSPLUS_FLG_NODUMP;
	else
		hip->userflags &= ~HFSPLUS_FLG_NODUMP;

	inode->i_ctime = CURRENT_TIME_SEC;
	mark_inode_dirty(inode);

out_unlock_inode:
	mutex_unlock(&inode->i_mutex);
out_drop_write:
	mnt_drop_write(file->f_path.mnt);
out:
	return err;
}

long hfsplus_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case HFSPLUS_IOC_EXT2_GETFLAGS:
		return hfsplus_ioctl_getflags(file, argp);
	case HFSPLUS_IOC_EXT2_SETFLAGS:
		return hfsplus_ioctl_setflags(file, argp);
	default:
		return -ENOTTY;
	}
}

#ifdef MY_ABC_HERE
static ssize_t hfsplus_get_cat_entry(struct inode *inode, hfsplus_cat_entry *entry)
{
	struct hfs_find_data fd;
	ssize_t res = 0;

	res = hfs_find_init(HFSPLUS_SB(inode->i_sb)->cat_tree, &fd);
	if (res)
		return res;
	res = hfsplus_find_cat(inode->i_sb, inode->i_ino, &fd);
	if (res) {
		goto out;
	}
	hfs_bnode_read(fd.bnode, entry, fd.entryoffset,
			sizeof(hfsplus_cat_entry));
out:
	hfs_find_exit(&fd);
	return res;
}

// This function would lock the mutex of catalog tree.
static ssize_t hfsplus_set_cat_flag(struct inode *inode, u16 flags, int add)
{
	struct hfs_find_data fd;
	hfsplus_cat_entry entry;
	ssize_t res = 0;
	int is_file = 0;

	res = hfs_find_init(HFSPLUS_SB(inode->i_sb)->cat_tree, &fd);
	if (res)
		return res;
	res = hfsplus_find_cat(inode->i_sb, inode->i_ino, &fd);
	if (res) {
		goto out;
	}
	hfs_bnode_read(fd.bnode, &entry, fd.entryoffset, sizeof(hfsplus_cat_entry));
	switch (be16_to_cpu(entry.type)) {
		case HFSPLUS_FILE:
			is_file = 1;
			break;
		case HFSPLUS_FOLDER:
			is_file = 0;
			break;
		default:
			res = EINVAL;
			goto out;
	}
	if (is_file) {
		if (add) 
			entry.file.flags |= cpu_to_be16(flags);
		else
			entry.file.flags &= ~cpu_to_be16(flags);
	} else {
		if (add) 
			entry.folder.flags |= cpu_to_be16(flags);
		else
			entry.folder.flags &= ~cpu_to_be16(flags);
	}
	res = 0;
	hfs_bnode_write(fd.bnode, &entry, fd.entryoffset,
			is_file ? sizeof(struct hfsplus_cat_file) : sizeof(struct hfsplus_cat_folder));
	inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	hfsplus_mark_inode_dirty(inode, HFSPLUS_I_CAT_DIRTY);
	
out:
	hfs_find_exit(&fd);
	return res;
}

// 3802 byte for 8K node_size
static inline size_t hfsplus_get_maxinline_attrsize(struct hfs_btree *btree)
{
	unsigned int maxsize = btree->node_size;
	// Copied from Apple open source /xnu-1699.26.8/bsd/hfs/hfs_xattr.c:2169
	maxsize -= sizeof(struct hfs_bnode_desc);  	/* minus node descriptor */
	maxsize -= 3 * sizeof(u16);        		/* minus 3 index slots */
	maxsize /= 2;                            /* 2 key/rec pairs minumum */
	maxsize -= sizeof(struct hfsplus_attr_key);       /* minus maximum key size */
	maxsize -= sizeof(struct hfsplus_attr_data) - 2;  /* minus data header */
	maxsize &= 0xFFFFFFFE;                   /* multiple of 2 bytes */
	return maxsize;
}

int hfsplus_setxattr_buildin(struct dentry *dentry, const char *name,
		     const void *value, size_t size, int flags)
{
	struct inode *inode = dentry->d_inode;
	struct hfs_btree *btree = HFSPLUS_SB(inode->i_sb)->cat_tree;
	struct hfs_find_data fd;
	hfsplus_cat_entry entry;
	struct hfsplus_cat_file *file;
	char empty_info[32] = {0};
	u8 is_file = 0;
	int res;

	if (HFSPLUS_IS_RSRC(inode))
		return -EOPNOTSUPP;
	res = hfs_find_init(btree, &fd);
	if (res)
		return res;
	res = hfsplus_find_cat(inode->i_sb, inode->i_ino, &fd);
	if (res)
		goto out;
	hfs_bnode_read(fd.bnode, &entry, fd.entryoffset,
			sizeof(struct hfsplus_cat_file));
	file = &entry.file;
	
	if (entry.type == cpu_to_be16(HFSPLUS_FILE)) {
		is_file = 1;
	}
	res = -ERANGE;
	if (!strcmp(name, SZ_XATTR_NAME_TYPE) && is_file) {
		if (size != 4)
			goto out;
		memcpy(&file->user_info.fdType, value, 4);
	} else if (!strcmp(name, SZ_XATTR_NAME_CREATOR) && is_file) {
		if (size != 4)
			goto out;
		memcpy(&file->user_info.fdCreator, value, 4);
	} else if (!strcmp(name, SZ_XATTR_NAME_FINDRINFO)) {
		if (memcmp(&file->user_info, &empty_info, sizeof(empty_info))) {
			if (flags & XATTR_CREATE) {
				res = -EEXIST;
				goto out;
			}
		} else {
			if (flags & XATTR_REPLACE) {
				res = -ENODATA;
				goto out;
			}
		}
		// reserve the date_added
		memcpy(&((struct FXInfo *)value + sizeof(struct FInfo))->date_added, &file->finder_info.date_added, sizeof(__be32));
		if (size != 32)
			goto out;
		memcpy(&file->user_info, value, 32);
	} else {
		res = -EOPNOTSUPP;
		goto out;
	}
	res = 0;
	hfs_bnode_write(fd.bnode, &entry, fd.entryoffset,
			is_file ? sizeof(struct hfsplus_cat_file) : sizeof(struct hfsplus_cat_folder));
	inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	hfsplus_mark_inode_dirty(inode, HFSPLUS_I_CAT_DIRTY);
	//if ((res = filemap_write_and_wait(HFSPLUS_SB(inode->i_sb)->cat_tree->inode->i_mapping))) {
	//	goto out;
	//}
out:
	hfs_find_exit(&fd);
	return res;
	
}

/* 
 * The flags XATTR_REPLACE and XATTR_CREATE
 * specify that an extended attribute must exist and must not exist
 * previous to the call, respectively.
 */
int hfsplus_setxattr(struct dentry *dentry, const char *name,
		     const void *value, size_t size, int flags)
{
	struct inode *inode = dentry->d_inode;
	struct hfs_btree *btree = HFSPLUS_SB(inode->i_sb)->attr_tree;
	struct hfs_find_data fd;
	hfsplus_attr_entry *entry = NULL;
	int res = 0, entry_size = 0;

	dprint(DBG_XATTR, "hfs: setattr [%s][%s] [%lu]\n", dentry->d_name.name, name, inode->i_ino);
	
	if (!value) {
		return -EINVAL;
	}
	if (!strcmp(name, SZ_XATTR_NAME_TYPE) ||
		!strcmp(name, SZ_XATTR_NAME_CREATOR) ||
		!strcmp(name, SZ_XATTR_NAME_FINDRINFO)) {
		return hfsplus_setxattr_buildin(dentry, name, value, size, flags);
	} else if (!strcmp(name, SZ_XATTR_NAME_RFORK)) {
		return -EOPNOTSUPP;
	}

#if 0
#define HFSPLUS_MAX_ATTR_LEN (128*1024)
	if (size > HFSPLUS_MAX_ATTR_LEN) {
		return E2BIG;
	}
#endif
	if (size > hfsplus_get_maxinline_attrsize(btree)) {
		return -E2BIG;
	}

	res = hfs_find_init(btree, &fd);
	if (res) {
		return res;
	}
	hfsplus_attr_build_key(inode->i_sb, fd.search_key, cpu_to_be32((u32)(unsigned long)dentry->d_fsdata), name, 0);
	res = hfs_brec_find(&fd);
	if (res == -ENOENT) {
		if (flags & XATTR_REPLACE) {
			res = -ENODATA;
			goto out;
		}
	} else if (res != 0) {
		goto out;
	} else { // res == 0
		if (flags & XATTR_CREATE) {
			res = -EEXIST;
			goto out;
		}
		if ((res = hfs_brec_remove(&fd))) {
			goto out;
		}
		// decrease record count to avoid the next insert worng.
		--fd.record;
		//hfsplus_mark_inode_dirty(btree->inode, HFSPLUS_I_ATTR_DIRTY);
		//if ((res = filemap_write_and_wait(btree->inode->i_mapping))) {
		//	goto out;
		//}
	}
	entry_size = sizeof(struct hfsplus_attr_data) - 2 + size + ((size & 1) ? 1 : 0);
	entry = kmalloc(entry_size * sizeof(char), GFP_NOFS);
	if (!entry) {
		res = -ENOMEM;
		goto out;
	}
	entry->type = cpu_to_be32(kHFSPlusAttrData);
	entry->data.attr_size = cpu_to_be32(size);
	memcpy(entry->data.attr_data, value, size);
	res = hfs_brec_insert(&fd, entry, entry_size);

	// set has attribute bit on file/folder
	hfsplus_set_cat_flag(inode, HFS_HAS_ATTR_MASK, 1);
	inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	hfsplus_mark_inode_dirty(inode, HFSPLUS_I_ATTR_DIRTY);
	//if ((res = filemap_write_and_wait(btree->inode->i_mapping))) {
	//	goto out;
	//}
out:
	if (entry) {
		kfree(entry);
	}
	hfs_find_exit(&fd);
	return res;
}

static ssize_t hfsplus_getxattr_buildin(struct dentry *dentry, const char *name,
			 void *value, size_t size)
{
	struct inode *inode = dentry->d_inode;
	hfsplus_cat_entry entry;
	struct hfsplus_cat_file *file;
	ssize_t res = 0;

	dprint(DBG_XATTR, "hfs: getattr [%s][%s] [%lu]\n", dentry->d_name.name, name, inode->i_ino);
	if (HFSPLUS_IS_RSRC(inode))
		return -EOPNOTSUPP;

	res = hfsplus_get_cat_entry(inode, &entry);
	file = &entry.file;

	if (!strcmp(name, SZ_XATTR_NAME_TYPE) && S_ISREG(inode->i_mode)) {
		if (size >= 4) {
			memcpy(value, &file->user_info.fdType, 4);
			res = 4;
		} else {
			res = size ? -ERANGE : 4;
		}
	} else if (!strcmp(name, SZ_XATTR_NAME_CREATOR) && S_ISREG(inode->i_mode)) {
		if (size >= 4) {
			memcpy(value, &file->user_info.fdCreator, 4);
			res = 4;
		} else {
			res = size ? -ERANGE : 4;
		}
	} else if (!strcmp(name, SZ_XATTR_NAME_FINDRINFO)) {
		// 32 byte = user_info + finder_info
		if (size >= 32) {
			// follow xnu implementation: zero out the dateadded
			file->finder_info.date_added = 0;
			memcpy(value, &file->user_info, 32);
			res = 32;
		} else {
			res = size ? -ERANGE : 32;
		}
	} else {
		res = -EOPNOTSUPP;
	}
	return res;
}

ssize_t hfsplus_getxattr(struct dentry *dentry, const char *name,
			 void *value, size_t size)
{
	struct inode *inode = dentry->d_inode;
	struct hfs_find_data fd;
	hfsplus_attr_entry *tmp = NULL;
	u32 attr_size = 0;
	ssize_t res = 0;

	dprint(DBG_XATTR, "hfs: getattr [%s][%s] [%lu]\n", dentry->d_name.name, name, inode->i_ino);

	if (!strcmp(name, SZ_XATTR_NAME_TYPE) ||
		!strcmp(name, SZ_XATTR_NAME_CREATOR) ||
		!strcmp(name, SZ_XATTR_NAME_FINDRINFO)) {
		return hfsplus_getxattr_buildin(dentry, name, value, size);
	} else if (!strcmp(name, SZ_XATTR_NAME_RFORK)) {
		return -EOPNOTSUPP;
	}

	res = hfs_find_init(HFSPLUS_SB(inode->i_sb)->attr_tree, &fd);
	if (res) {
		return res;
	}
	hfsplus_attr_build_key(inode->i_sb, fd.search_key, cpu_to_be32((u32)(unsigned long)dentry->d_fsdata), name, 0);
	res = hfs_brec_find(&fd);
	if (res) {
		goto out;
	}
	tmp = kmalloc(fd.entrylength*sizeof(char), GFP_NOFS);
	if (tmp == NULL) {
		res = -ENOMEM;
		goto out;
	}
	hfs_bnode_read(fd.bnode, tmp, fd.entryoffset, fd.entrylength);

	switch (be32_to_cpu(tmp->type)) {
		case kHFSPlusAttrData:
			attr_size = be32_to_cpu(tmp->data.attr_size);
			if (size >= attr_size) {
				memcpy(value, tmp->data.attr_data, attr_size);
				res = attr_size;
				dprint(DBG_XATTR, "hfs: [rec:%d] [attr_size:%u] [key_off:%d, key_len:%d] [entry_off:%d, entry_len:%d]\n", fd.record, attr_size, fd.keyoffset, fd.keylength, fd.entryoffset, fd.entrylength);
			} else {
				res = size ? -ERANGE : attr_size;
			}
			break;
		case kHFSPlusAttrForkData:
		case kHFSPlusAttrExtents:
			res = -EOPNOTSUPP;
			break;
		default:
			printk(KERN_ERR "hfs: found bad record type in attribute btree\n");
			res = -EINVAL;
			goto out;
	}
out:
	if (tmp) {
		kfree(tmp);
	}
	hfs_find_exit(&fd);
	return res;
}

static ssize_t hfsplus_listxattr_ex(struct dentry *dentry, char *buffer, size_t size, int check_buildin)
{
	ssize_t res = 0;
	char strbuf[HFSPLUS_XATTR_MAX_NAMELEN + 1] = {0}, empty_info[32] = {0};
	hfsplus_cnid cnid = cpu_to_be32((u32)(unsigned long)dentry->d_fsdata);
	int len = 0;
	size_t strbuflen = 0;
	struct inode *inode = dentry->d_inode;
	struct hfs_find_data fd;
	hfsplus_cat_entry entry;
	struct hfsplus_cat_file *file;
	u16 type = 0;

#define __APPEND_XATTRNAME(name, namelen) \
	do { \
		if (size > strbuflen + namelen) strcpy(buffer + strbuflen, name); \
		strbuflen += namelen + 1; \
	} while (0)

	if (HFSPLUS_IS_RSRC(inode))
		return -EOPNOTSUPP;

	if (!buffer)
		size = 0;

	// check FinderInfo & ResourceFork(file only)
	if (check_buildin && !hfsplus_get_cat_entry(inode, &entry)) {
		file = &entry.file;
		type = be16_to_cpu(entry.type);

		if (type != HFSPLUS_FILE && type != HFSPLUS_FOLDER) {
			printk(KERN_ERR "hfs: invalid catalog entry type in lookup\n");
			res = -EIO;
			goto out;
		}
		if (type == HFSPLUS_FILE) {
			if (0 < be64_to_cpu(entry.file.rsrc_fork.total_size)) {
				len = strlen(SZ_XATTR_NAME_RFORK);
				__APPEND_XATTRNAME(SZ_XATTR_NAME_RFORK, len);
			}
		}

		entry.file.finder_info.date_added = 0;
		if (memcmp(&entry.file.user_info, &empty_info, sizeof(empty_info))) {
			len = strlen(SZ_XATTR_NAME_FINDRINFO);
			__APPEND_XATTRNAME(SZ_XATTR_NAME_FINDRINFO, len);
		}
	}
	if ((res = hfs_find_init(HFSPLUS_SB(inode->i_sb)->attr_tree, &fd))) {
		goto out;
	}
	hfsplus_attr_build_key(inode->i_sb, fd.search_key, cnid, NULL, 0);
	if ((res = hfsplus_brec_find_first(&fd))) {
		goto out;
	}
	// traverse other EA
	while (!res && fd.key->attr.file_id == cnid) {
		len = sizeof(strbuf);
		memset(strbuf, 0, len);
		hfsplus_uni2asc(inode->i_sb, (struct hfsplus_unistr *)&fd.key->attr.name, strbuf, &len);
		__APPEND_XATTRNAME(strbuf, len);
		dprint(DBG_XATTR, "hfs: enum [%u] bnode:[%u,%d][%s] key:[%d,%d] entry:[%d,%d].\n", 
			be32_to_cpu(fd.key->attr.file_id), fd.bnode->this, fd.record, strbuf, fd.keyoffset, fd.keylength, fd.entryoffset, fd.entrylength);
		res = hfs_brec_goto(&fd, 1);
	}
#undef __APPEND_XATTRNAME

out:
	if (strbuflen > 0 && (res == -ENOENT || res == 0)) {
		res = strbuflen;
		if (size < strbuflen && size > 0) {
			res = -ERANGE;
		}
	}
	hfs_find_exit(&fd);
	return res;
}

ssize_t hfsplus_listxattr(struct dentry *dentry, char *buffer, size_t size)
{
	return hfsplus_listxattr_ex(dentry, buffer, size, 1);
}

// this function won't check build-in attr. (behave like mac kernel xnu-1699.26.8)
static int hfsplus_has_xattr(struct dentry *dentry)
{
	if (0 < hfsplus_listxattr_ex(dentry, NULL, 0, 0)) {
		return 1;
	}
	return 0;
}

int hfsplus_rmxattr(struct dentry *dentry, const char *name)
{
	struct inode *inode = dentry->d_inode;
	struct hfs_btree *btree = HFSPLUS_SB(inode->i_sb)->attr_tree;
	struct hfs_find_data fd;
	hfsplus_attr_entry entry;
	char tmp[32] = {0};
	int res = 0;

	dprint(DBG_XATTR, "hfs: rmattr [%s][%s] [%lu]\n", dentry->d_name.name, name, dentry->d_inode->i_ino);

	if (!strcmp(name, SZ_XATTR_NAME_TYPE) ||
		!strcmp(name, SZ_XATTR_NAME_CREATOR)) {
		return hfsplus_setxattr_buildin(dentry, name, tmp, 4, 0);
	}
	if (!strcmp(name, SZ_XATTR_NAME_FINDRINFO)) {
		return hfsplus_setxattr_buildin(dentry, name, tmp, 32, 0);
	} else if (!strcmp(name, SZ_XATTR_NAME_RFORK)) {
		return -EOPNOTSUPP;
	}

	res = hfs_find_init(btree, &fd);
	if (res) {
		return res;
	}
	hfsplus_attr_build_key(inode->i_sb, fd.search_key, cpu_to_be32((u32)(unsigned long)dentry->d_fsdata), name, 0);
	if ((res = hfs_brec_find(&fd))) {
		hfs_find_exit(&fd);
		goto out;
	}
	hfs_bnode_read(fd.bnode, &entry, fd.entryoffset, sizeof(hfsplus_attr_entry));
	if (be32_to_cpu(entry.type) != kHFSPlusAttrData) {
		res = -EOPNOTSUPP;
		hfs_find_exit(&fd);
		goto out;
	}
	res = hfs_brec_remove(&fd);

	inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	hfsplus_mark_inode_dirty(inode, HFSPLUS_I_ATTR_DIRTY);
	hfs_find_exit(&fd);

	// check xattr after hfs_find_exit (unlock attr btree)
	if (!hfsplus_has_xattr(dentry)) {
		hfsplus_set_cat_flag(inode, HFS_HAS_ATTR_MASK, 0); // here would lock cat tree.
	}
	//if ((res = filemap_write_and_wait(inode->i_mapping))) {
	//	goto out;
	//}
out:
	return res;
}
#else
int hfsplus_setxattr(struct dentry *dentry, const char *name,
		     const void *value, size_t size, int flags)
{
	struct inode *inode = dentry->d_inode;
	struct hfs_find_data fd;
	hfsplus_cat_entry entry;
	struct hfsplus_cat_file *file;
	int res;

	if (!S_ISREG(inode->i_mode) || HFSPLUS_IS_RSRC(inode))
		return -EOPNOTSUPP;

	res = hfs_find_init(HFSPLUS_SB(inode->i_sb)->cat_tree, &fd);
	if (res)
		return res;
	res = hfsplus_find_cat(inode->i_sb, inode->i_ino, &fd);
	if (res)
		goto out;
	hfs_bnode_read(fd.bnode, &entry, fd.entryoffset,
			sizeof(struct hfsplus_cat_file));
	file = &entry.file;

	if (!strcmp(name, "hfs.type")) {
		if (size == 4)
			memcpy(&file->user_info.fdType, value, 4);
		else
			res = -ERANGE;
	} else if (!strcmp(name, "hfs.creator")) {
		if (size == 4)
			memcpy(&file->user_info.fdCreator, value, 4);
		else
			res = -ERANGE;
	} else
		res = -EOPNOTSUPP;
	if (!res) {
		hfs_bnode_write(fd.bnode, &entry, fd.entryoffset,
				sizeof(struct hfsplus_cat_file));
		hfsplus_mark_inode_dirty(inode, HFSPLUS_I_CAT_DIRTY);
	}
out:
	hfs_find_exit(&fd);
	return res;
}

ssize_t hfsplus_getxattr(struct dentry *dentry, const char *name,
			 void *value, size_t size)
{
	struct inode *inode = dentry->d_inode;
	struct hfs_find_data fd;
	hfsplus_cat_entry entry;
	struct hfsplus_cat_file *file;
	ssize_t res = 0;

	if (!S_ISREG(inode->i_mode) || HFSPLUS_IS_RSRC(inode))
		return -EOPNOTSUPP;

	if (size) {
		res = hfs_find_init(HFSPLUS_SB(inode->i_sb)->cat_tree, &fd);
		if (res)
			return res;
		res = hfsplus_find_cat(inode->i_sb, inode->i_ino, &fd);
		if (res)
			goto out;
		hfs_bnode_read(fd.bnode, &entry, fd.entryoffset,
				sizeof(struct hfsplus_cat_file));
	}
	file = &entry.file;

	if (!strcmp(name, "hfs.type")) {
		if (size >= 4) {
			memcpy(value, &file->user_info.fdType, 4);
			res = 4;
		} else
			res = size ? -ERANGE : 4;
	} else if (!strcmp(name, "hfs.creator")) {
		if (size >= 4) {
			memcpy(value, &file->user_info.fdCreator, 4);
			res = 4;
		} else
			res = size ? -ERANGE : 4;
	} else
		res = -EOPNOTSUPP;
out:
	if (size)
		hfs_find_exit(&fd);
	return res;
}

#define HFSPLUS_ATTRLIST_SIZE (sizeof("hfs.creator")+sizeof("hfs.type"))

ssize_t hfsplus_listxattr(struct dentry *dentry, char *buffer, size_t size)
{
	struct inode *inode = dentry->d_inode;

	if (!S_ISREG(inode->i_mode) || HFSPLUS_IS_RSRC(inode))
		return -EOPNOTSUPP;

	if (!buffer || !size)
		return HFSPLUS_ATTRLIST_SIZE;
	if (size < HFSPLUS_ATTRLIST_SIZE)
		return -ERANGE;
	strcpy(buffer, "hfs.type");
	strcpy(buffer + sizeof("hfs.type"), "hfs.creator");

	return HFSPLUS_ATTRLIST_SIZE;
}
#endif
