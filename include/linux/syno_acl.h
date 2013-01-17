/*
 * File: linux/syno_acl.h
 * Copyright (c) 2000-2010 Synology Inc.
 */
#ifndef __LINUX_SYNO_ACL_H
#define __LINUX_SYNO_ACL_H

#include <linux/syno_acl_xattr_ds.h>

/* e_tag entry in struct syno_acl_entry */
#define SYNO_ACL_USER		(0x01)
#define SYNO_ACL_GROUP		(0x02)
#define SYNO_ACL_EVERYONE	(0x04)
#define SYNO_ACL_OWNER	(0x08)
#define SYNO_ACL_TAG_ALL  (SYNO_ACL_USER | SYNO_ACL_GROUP | \
						   SYNO_ACL_OWNER | SYNO_ACL_EVERYONE)
						   
/* e_allow */
#define SYNO_ACL_ALLOW		(0x01)
#define SYNO_ACL_DENY		(0x02)

struct syno_acl_entry {
    unsigned short  e_tag; 
    unsigned int    e_id;
    unsigned int	e_perm;
    unsigned short  e_inherit;
    unsigned short  e_allow;
	unsigned int  	e_level;
};

struct syno_acl { 
    atomic_t                a_refcount;
    unsigned int            a_count;
    struct syno_acl_entry   a_entries[0];
};

#endif  /* __LINUX_SYNO_ACL_H */
