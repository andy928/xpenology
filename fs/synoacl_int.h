/*
 * File: fs/synoacl_int.h
 * Copyright (c) 2000-2010 Synology Inc.
 */
#ifndef __LINUX_SYNOACL_INT_H
#define __LINUX_SYNOACL_INT_H

#include <linux/syno_acl.h>

#define PROTECT_BY_ACL 0x0001
#define NEED_INODE_ACL_SUPPORT 0x0004
#define NEED_FS_ACL_SUPPORT 0x0008

struct synoacl_syscall_operations {
	int (*get_perm) (const char *szPath, int __user *pOutPerm);
	int (*is_acl_support) (const char *szPath, int fd, int tag);
	int (*check_perm) (const char *szPath, int mask);
};

struct synoacl_vfs_operations {
	void (*syno_acl_release) (struct syno_acl *acl);
	int (*archive_change_ok) (struct dentry *d, unsigned int cmd, int tag, int mask);
	int (*check_perm) (const char *szPath, int mask);
	int (*syno_acl_may_delete) (struct dentry *, struct inode *, int);
};

void synoacl_mod_release(struct syno_acl *);
int synoacl_mod_archive_change_ok(struct dentry *, unsigned int , int , int );
int synoacl_mod_may_delete(struct dentry *, struct inode *);

#endif  /* __LINUX_SYNOACL_INT_H */
