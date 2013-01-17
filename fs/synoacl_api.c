/*
 * linux/fs/synoacl_api.c
 *
 * Copyright (c) 2000-2010 Synology Inc.
 */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/errno.h>

#include "synoacl_int.h"

#define VFS_MODULE	psynoacl_vfs_op
#define SYSCALL_MODULE	psynoacl_syscall_op

#define IS_VFS_ACL_READY(x) (VFS_MODULE && VFS_MODULE->x)
#define IS_SYSCALL_ACL_READY(x) (SYSCALL_MODULE && SYSCALL_MODULE->x)
#define DO_VFS(x, ...) VFS_MODULE->x(__VA_ARGS__)
#define DO_SYSCALL(x, ...) SYSCALL_MODULE->x(__VA_ARGS__)

struct synoacl_vfs_operations *VFS_MODULE = NULL;
struct synoacl_syscall_operations *SYSCALL_MODULE = NULL;

/* --------------- Register Function ---------------- */
int synoacl_vfs_register(struct synoacl_vfs_operations *pvfs, struct synoacl_syscall_operations *psys)
{
	if (!pvfs || !psys) {
		return -1;
	}

	VFS_MODULE = pvfs;
	SYSCALL_MODULE = psys;

	return 0;
}
EXPORT_SYMBOL(synoacl_vfs_register);

void synoacl_vfs_unregister(void)
{
	VFS_MODULE = NULL;
	SYSCALL_MODULE = NULL;
}
EXPORT_SYMBOL(synoacl_vfs_unregister);

/* --------------- VFS API ---------------- */
void synoacl_mod_release(struct syno_acl *acl)
{
	if (IS_VFS_ACL_READY(syno_acl_release)) {
		DO_VFS(syno_acl_release, acl);
	}
}

int synoacl_mod_archive_change_ok(struct dentry *d, unsigned int cmd, int tag, int mask)
{
	if (IS_VFS_ACL_READY(archive_change_ok)) {
		return DO_VFS(archive_change_ok, d, cmd, tag, mask);
	}
	return 0; //is settable
}

int synoacl_mod_may_delete(struct dentry *d, struct inode *dir)
{
	if (IS_VFS_ACL_READY(syno_acl_may_delete)) {
		return DO_VFS(syno_acl_may_delete, d, dir, 1);
	}
	return inode_permission(dir, MAY_WRITE | MAY_EXEC);
}
EXPORT_SYMBOL(synoacl_mod_may_delete);

/* --------------- System Call API ---------------- */
asmlinkage long sys_SYNOACLIsSupport(const char *szPath, int fd, int tag)
{
	if (IS_SYSCALL_ACL_READY(is_acl_support)) {
		return DO_SYSCALL(is_acl_support, szPath, fd, tag);
	}
	return -EOPNOTSUPP;
}

asmlinkage long sys_SYNOACLCheckPerm(const char *szPath, int mask)
{
	if (IS_SYSCALL_ACL_READY(check_perm)) {
		return DO_SYSCALL(check_perm, szPath, mask);
	}
	return -EOPNOTSUPP;
}

asmlinkage long sys_SYNOACLGetPerm(const char *szPath, int __user *pOutPerm)
{
	if (IS_SYSCALL_ACL_READY(get_perm)) {
		return DO_SYSCALL(get_perm, szPath, pOutPerm);
	}
	return -EOPNOTSUPP;
}




