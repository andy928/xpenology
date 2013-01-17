/*
 * File: fs/ext4/synoacl_int.h
 * Copyright (c) 2000-2010 Synology Inc.
 */
#ifndef __LINUX_SYNOACL_EXT4_INT_H
#define __LINUX_SYNOACL_EXT4_INT_H

struct synoacl_operations {
	int (*xattr_get_syno_acl) (struct inode *inode, void *buffer, size_t size);
	int (*xattr_set_syno_acl) (struct inode *inode, const void *value,  size_t size);
	size_t (*xattr_list_syno_acl) (struct inode *inode, char *list, size_t list_len, const char *name, size_t name_len);

	int (*get_syno_acl_inherit) (struct dentry *d, int cmd, void *value, size_t size);
	int (*syno_permission) (struct dentry *d, int mask);
	int (*syno_exec_permission) (struct dentry *d);
	int (*get_syno_permission) (struct dentry *, unsigned int *, unsigned int *);
	int (*syno_acl_chmod) (struct inode *inode);
	int (*init_syno_acl) (struct inode *inode, struct dentry *d);
	int (*rename_syno_acl) (struct inode *inode, struct inode *new_inode);
	int (*syno_inode_change_ok) (struct dentry *d, struct iattr *attr);
	void (*syno_archive_safe_clean) (struct inode *inode, unsigned int want);
	void (*synoacl_to_mode) (struct dentry *, struct inode *, struct kstat *);
	int (*dir_getattr) (struct vfsmount *, struct dentry *, struct kstat *);
	int (*syno_access) (struct dentry *, int);
};

//synoacl EXT4 API
int ext4_mod_syno_access(struct dentry *, int);
int ext4_mod_get_syno_acl_inherit(struct dentry *, int, void *, size_t);
int ext4_mod_syno_permission(struct dentry *, int);
int ext4_mod_syno_exec_permission(struct dentry *);
int ext4_mod_get_syno_permission(struct dentry *, unsigned int *, unsigned int *);
int ext4_mod_syno_acl_chmod(struct inode *);
int ext4_mod_init_syno_acl(struct inode *, struct dentry *);
int ext4_mod_rename_syno_acl(struct inode *, struct inode *);
int ext4_mod_syno_inode_change_ok(struct dentry *, struct iattr *);
int ext4_mod_syno_archive_safe_clean(struct inode *, unsigned int);
int ext4_mod_dir_getattr(struct vfsmount *, struct dentry *, struct kstat *);
void ext4_mod_synoacl_to_mode(struct dentry *, struct inode *, struct kstat *);

#endif  /* __LINUX_SYNOACL_EXT4_INT_H */
