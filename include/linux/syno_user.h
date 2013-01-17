/* Copyright (c) 2010 Synology Inc. All rights reserved. */

#ifndef __SYNO_USER_H_
#define __SYNO_USER_H_

/**
 * Dsc: Synology multimedia server feature. For indexing video, photo,
 *      music.
 * Ref: libsynosdk, lnxnetatalk, lnxsdk, rsync, samba, smbftpd
 */
#define MY_ABC_HERE
#define SYNO_INDEX_SHARES		"photo,video,music"

/**
 * Dsc: This definition is used to enhance samba's performance. 
 *      This modify should sync with samba
 */
#define MY_ABC_HERE

/**
 * Dsc: This modify should sync with netatalk
 */
#define MY_ABC_HERE

/**
 * Dsc: This modify should sync with samba
 */
#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif

#ifdef MY_ABC_HERE
#define SYNO_MAXPATH    4095
#define SYNO_MAXNAME    491
#endif

/**
 * Fix: DS20 bug #1405
 * Dsc: Avoid scan all inodes of ext3 while doing quotacheck
 */
#define MY_ABC_HERE

#if defined(SYNO_X86) || defined(SYNO_X64) || defined(SYNO_BROMOLOW)
#define MY_ABC_HERE
#endif

#define MY_ABC_HERE

#endif /* __SYNO_USER_H_ */
