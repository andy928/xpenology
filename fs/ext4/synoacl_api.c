/*
 * linux/fs/ext4/synoacl.c
 *
 * Copyright (c) 2000-2010 Synology Inc.
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/capability.h>
#include <linux/fs.h>
#include <linux/security.h>
#include "ext4_jbd2.h"
#include "ext4.h"
#include "xattr.h"

#ifdef CONFIG_FS_SYNO_ACL
#include <linux/export.h>
#include "synoacl_int.h"

#define EXT4_MODULE	psynoacl_ext4_op

#define IS_EXT4_ACL_READY(x) (EXT4_MODULE && EXT4_MODULE->x)
#define DO_EXT4_ACL(x, ...) EXT4_MODULE->x(__VA_ARGS__)

struct synoacl_operations * EXT4_MODULE = NULL;

int synoacl_ext4_register(struct synoacl_operations *pext4)
{
	if (!pext4) {
		return -1;
	}

	EXT4_MODULE = pext4;

	return 0;
}
EXPORT_SYMBOL(synoacl_ext4_register);

int synoacl_ext4_unregister(void)
{
	EXT4_MODULE = NULL;
	return 0;
}
EXPORT_SYMBOL(synoacl_ext4_unregister);

/* ----------------------- EXT4 SYNOACL API ------------------------------ */
int ext4_mod_syno_access(struct dentry *d, int mask)
{
	if (IS_EXT4_ACL_READY(syno_access)) {
		return DO_EXT4_ACL(syno_access, d, mask);
	}
	return inode_permission(d->d_inode, mask);
}

int ext4_mod_get_syno_acl_inherit(struct dentry *d, int cmd, void *value, size_t size)
{
	if (IS_EXT4_ACL_READY(get_syno_acl_inherit)) {
		return DO_EXT4_ACL(get_syno_acl_inherit, d, cmd, value, size);
	}
	return -EOPNOTSUPP;
}

int ext4_mod_syno_permission(struct dentry *d, int mask)
{
	if (IS_EXT4_ACL_READY(syno_permission)) {
		return DO_EXT4_ACL(syno_permission, d, mask);
	}
	return inode_permission(d->d_inode, mask);
}

int ext4_mod_syno_exec_permission(struct dentry *d)
{
	if (IS_EXT4_ACL_READY(syno_exec_permission)) {
		return DO_EXT4_ACL(syno_exec_permission, d);
	}
	return 0;
}

int ext4_mod_get_syno_permission(struct dentry *d, unsigned int *pPermAllow, unsigned int *pPermDeny)
{
	if (IS_EXT4_ACL_READY(get_syno_permission)) {
		return DO_EXT4_ACL(get_syno_permission, d, pPermAllow, pPermDeny);
	}
	return -EOPNOTSUPP;
}

int ext4_mod_syno_acl_chmod(struct inode *inode)
{
	if (IS_EXT4_ACL_READY(syno_acl_chmod)) {
		return DO_EXT4_ACL(syno_acl_chmod, inode);
	}
	return -EOPNOTSUPP;
}

int ext4_mod_init_syno_acl(struct inode *inode, struct dentry *d)
{
	if (IS_EXT4_ACL_READY(init_syno_acl)) {
		return DO_EXT4_ACL(init_syno_acl, inode, d);
	}
	return -EOPNOTSUPP;
}

int ext4_mod_rename_syno_acl(struct inode *inode, struct inode *newdir)
{
	if (IS_EXT4_ACL_READY(rename_syno_acl)) {
		return DO_EXT4_ACL(rename_syno_acl, inode, newdir);
	}
	return -EOPNOTSUPP;
}

int ext4_mod_syno_inode_change_ok(struct dentry *d, struct iattr *attr)
{
	if (IS_EXT4_ACL_READY(syno_inode_change_ok)) {
		return DO_EXT4_ACL(syno_inode_change_ok, d, attr);
	}
	return inode_change_ok(d->d_inode, attr);
}

int ext4_mod_syno_archive_safe_clean(struct inode *inode, unsigned int want)
{
	if (IS_EXT4_ACL_READY(syno_archive_safe_clean)) {
		DO_EXT4_ACL(syno_archive_safe_clean, inode, want);
	}
	return -EOPNOTSUPP;
}

void ext4_mod_synoacl_to_mode(struct dentry *d, struct inode *inode, struct kstat *stat)
{
	if (IS_EXT4_ACL_READY(synoacl_to_mode)) {
		DO_EXT4_ACL(synoacl_to_mode, d, inode, stat);
	}
}

int ext4_mod_dir_getattr(struct vfsmount *mnt, struct dentry *d, struct kstat *stat)
{
	if (IS_EXT4_ACL_READY(dir_getattr)) {
		return DO_EXT4_ACL(dir_getattr, mnt, d, stat);
	}
	generic_fillattr(d->d_inode, stat);
	return 0;
}
/*
 * API: Extended attribute handlers
 */
static size_t ext4_mod_xattr_list_syno_acl_access(struct dentry *dentry, char *list, size_t list_len,
			   const char *name, size_t name_len, int handler_flags)
{
	if (IS_EXT4_ACL_READY(xattr_list_syno_acl)) {
		return DO_EXT4_ACL(xattr_list_syno_acl, dentry->d_inode, list, list_len, name, name_len);
	}
	return -EOPNOTSUPP;
}

static int ext4_mod_xattr_get_syno_acl_access(struct dentry *dentry, const char *name,
			  void *buffer, size_t size, int handler_flags)
{
	if (strcmp(name, "") != 0){
		return -EINVAL;
	}
	if (IS_EXT4_ACL_READY(xattr_get_syno_acl)) {
		return DO_EXT4_ACL(xattr_get_syno_acl, dentry->d_inode, buffer, size);
	}
	return -EOPNOTSUPP;
}

static int ext4_mod_xattr_set_syno_acl_access(struct dentry *dentry, const char *name,
			  const void *value, size_t size, int flags, int handler_flags)
{
	if (strcmp(name, "") != 0){
		return -EINVAL;
	}
	if (IS_EXT4_ACL_READY(xattr_set_syno_acl)) {
		return DO_EXT4_ACL(xattr_set_syno_acl, dentry->d_inode, value, size);
	}
	return -EOPNOTSUPP;
}

struct xattr_handler ext4_xattr_synoacl_access_handler = {
	.prefix	= SYNO_ACL_XATTR_ACCESS,
	.list	= ext4_mod_xattr_list_syno_acl_access,
	.get	= ext4_mod_xattr_get_syno_acl_access,
	.set	= ext4_mod_xattr_set_syno_acl_access,
};
#endif /* CONFIG_FS_SYNO_ACL */
