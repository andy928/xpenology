/*
 *  linux/fs/stat.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/highuid.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/security.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>

#include <asm/uaccess.h>
#include <asm/unistd.h>

#ifdef MY_ABC_HERE
#include <linux/synolib.h>
extern int SynoDebugFlag;
extern int syno_hibernation_log_sec;
#endif

void generic_fillattr(struct inode *inode, struct kstat *stat)
{
	stat->dev = inode->i_sb->s_dev;
	stat->ino = inode->i_ino;
	stat->mode = inode->i_mode;
#ifdef MY_ABC_HERE
	stat->SynoMode = inode->i_mode2;
#endif
#ifdef MY_ABC_HERE
	stat->syno_archive_version = inode->i_archive_version;
#endif
	stat->nlink = inode->i_nlink;
	stat->uid = inode->i_uid;
	stat->gid = inode->i_gid;
	stat->rdev = inode->i_rdev;
	stat->size = i_size_read(inode);
	stat->atime = inode->i_atime;
	stat->mtime = inode->i_mtime;
	stat->ctime = inode->i_ctime;
#ifdef MY_ABC_HERE
	stat->SynoCreateTime = inode->i_CreateTime;
#endif
	stat->blksize = (1 << inode->i_blkbits);
	stat->blocks = inode->i_blocks;
}

EXPORT_SYMBOL(generic_fillattr);

int vfs_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
	struct inode *inode = dentry->d_inode;
	int retval;

	retval = security_inode_getattr(mnt, dentry);
	if (retval)
		return retval;

	if (inode->i_op->getattr)
		return inode->i_op->getattr(mnt, dentry, stat);

	generic_fillattr(inode, stat);
	return 0;
}

EXPORT_SYMBOL(vfs_getattr);

int vfs_fstat(unsigned int fd, struct kstat *stat)
{
	struct file *f = fget(fd);
	int error = -EBADF;

	if (f) {
		error = vfs_getattr(f->f_path.mnt, f->f_path.dentry, stat);
		fput(f);
	}
	return error;
}
EXPORT_SYMBOL(vfs_fstat);

int vfs_fstatat(int dfd, const char __user *filename, struct kstat *stat,
		int flag)
{
	struct path path;
	int error = -EINVAL;
	int lookup_flags = 0;

	if ((flag & ~(AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT |
		      AT_EMPTY_PATH)) != 0)
		goto out;

	if (!(flag & AT_SYMLINK_NOFOLLOW))
		lookup_flags |= LOOKUP_FOLLOW;
	if (flag & AT_EMPTY_PATH)
		lookup_flags |= LOOKUP_EMPTY;

	error = user_path_at(dfd, filename, lookup_flags, &path);
	if (error)
		goto out;

	error = vfs_getattr(path.mnt, path.dentry, stat);
	path_put(&path);
out:
	return error;
}
EXPORT_SYMBOL(vfs_fstatat);

int vfs_stat(const char __user *name, struct kstat *stat)
{
	return vfs_fstatat(AT_FDCWD, name, stat, 0);
}
EXPORT_SYMBOL(vfs_stat);

int vfs_lstat(const char __user *name, struct kstat *stat)
{
	return vfs_fstatat(AT_FDCWD, name, stat, AT_SYMLINK_NOFOLLOW);
}
EXPORT_SYMBOL(vfs_lstat);


#ifdef __ARCH_WANT_OLD_STAT

/*
 * For backward compatibility?  Maybe this should be moved
 * into arch/i386 instead?
 */
static int cp_old_stat(struct kstat *stat, struct __old_kernel_stat __user * statbuf)
{
	static int warncount = 5;
	struct __old_kernel_stat tmp;
	
	if (warncount > 0) {
		warncount--;
		printk(KERN_WARNING "VFS: Warning: %s using old stat() call. Recompile your binary.\n",
			current->comm);
	} else if (warncount < 0) {
		/* it's laughable, but... */
		warncount = 0;
	}

	memset(&tmp, 0, sizeof(struct __old_kernel_stat));
	tmp.st_dev = old_encode_dev(stat->dev);
	tmp.st_ino = stat->ino;
	if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
		return -EOVERFLOW;
	tmp.st_mode = stat->mode;
	tmp.st_nlink = stat->nlink;
	if (tmp.st_nlink != stat->nlink)
		return -EOVERFLOW;
	SET_UID(tmp.st_uid, stat->uid);
	SET_GID(tmp.st_gid, stat->gid);
	tmp.st_rdev = old_encode_dev(stat->rdev);
#if BITS_PER_LONG == 32
	if (stat->size > MAX_NON_LFS)
		return -EOVERFLOW;
#endif	
	tmp.st_size = stat->size;
	tmp.st_atime = stat->atime.tv_sec;
	tmp.st_mtime = stat->mtime.tv_sec;
	tmp.st_ctime = stat->ctime.tv_sec;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

SYSCALL_DEFINE2(stat, const char __user *, filename,
		struct __old_kernel_stat __user *, statbuf)
{
	struct kstat stat;
	int error;

	error = vfs_stat(filename, &stat);
#ifdef MY_ABC_HERE
	if(syno_hibernation_log_sec > 0) {
		syno_do_hibernation_log(filename);
	}
#endif
	if (error)
		return error;

	return cp_old_stat(&stat, statbuf);
}

SYSCALL_DEFINE2(lstat, const char __user *, filename,
		struct __old_kernel_stat __user *, statbuf)
{
	struct kstat stat;
	int error;

	error = vfs_lstat(filename, &stat);
	if (error)
		return error;

	return cp_old_stat(&stat, statbuf);
}

SYSCALL_DEFINE2(fstat, unsigned int, fd, struct __old_kernel_stat __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_fstat(fd, &stat);

	if (!error)
		error = cp_old_stat(&stat, statbuf);

	return error;
}

#endif /* __ARCH_WANT_OLD_STAT */

static int cp_new_stat(struct kstat *stat, struct stat __user *statbuf)
{
	struct stat tmp;

#if BITS_PER_LONG == 32
	if (!old_valid_dev(stat->dev) || !old_valid_dev(stat->rdev))
		return -EOVERFLOW;
#else
	if (!new_valid_dev(stat->dev) || !new_valid_dev(stat->rdev))
		return -EOVERFLOW;
#endif

	memset(&tmp, 0, sizeof(tmp));
#if BITS_PER_LONG == 32
	tmp.st_dev = old_encode_dev(stat->dev);
#else
	tmp.st_dev = new_encode_dev(stat->dev);
#endif
	tmp.st_ino = stat->ino;
	if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
		return -EOVERFLOW;
	tmp.st_mode = stat->mode;
	tmp.st_nlink = stat->nlink;
	if (tmp.st_nlink != stat->nlink)
		return -EOVERFLOW;
	SET_UID(tmp.st_uid, stat->uid);
	SET_GID(tmp.st_gid, stat->gid);
#if BITS_PER_LONG == 32
	tmp.st_rdev = old_encode_dev(stat->rdev);
#else
	tmp.st_rdev = new_encode_dev(stat->rdev);
#endif
#if BITS_PER_LONG == 32
	if (stat->size > MAX_NON_LFS)
		return -EOVERFLOW;
#endif	
	tmp.st_size = stat->size;
	tmp.st_atime = stat->atime.tv_sec;
	tmp.st_mtime = stat->mtime.tv_sec;
	tmp.st_ctime = stat->ctime.tv_sec;
#ifdef STAT_HAVE_NSEC
	tmp.st_atime_nsec = stat->atime.tv_nsec;
	tmp.st_mtime_nsec = stat->mtime.tv_nsec;
	tmp.st_ctime_nsec = stat->ctime.tv_nsec;
#endif
#ifdef MY_ABC_HERE
	tmp.st_SynoCreateTime = stat->SynoCreateTime.tv_sec;
#endif
#ifdef MY_ABC_HERE
	tmp.st_SynoMode = stat->SynoMode;
#endif
#ifdef MY_ABC_HERE
	tmp.st_syno_achv_ver = stat->syno_archive_version;
#endif
	tmp.st_blocks = stat->blocks;
	tmp.st_blksize = stat->blksize;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

SYSCALL_DEFINE2(newstat, const char __user *, filename,
		struct stat __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_stat(filename, &stat);

	if (error)
		return error;
	return cp_new_stat(&stat, statbuf);
}

#ifdef MY_ABC_HERE
#include "../fs/ecryptfs/ecryptfs_kernel.h"
int (*fecryptfs_decode_and_decrypt_filename)(char **plaintext_name,
                                        size_t *plaintext_name_size,
                                        struct dentry *ecryptfs_dir_dentry,
                                        const char *name, size_t name_size) = NULL;
EXPORT_SYMBOL(fecryptfs_decode_and_decrypt_filename);

asmlinkage long sys_SYNOEcryptName(char __user * src, char __user * dst)
{
	int                               err = -1;
	struct qstr                      *lower_path = NULL;
	struct path                       path;
	struct ecryptfs_dentry_info      *crypt_dentry = NULL;

	if (NULL == src || NULL == dst) {
		return -EINVAL;
	}

	err = user_path_at(AT_FDCWD, src, LOOKUP_FOLLOW, &path);

	if (err) {
		return -ENOENT;
	}
	if (!path.dentry->d_inode->i_sb->s_type || 
		strcmp(path.dentry->d_inode->i_sb->s_type->name, "ecryptfs")) {
		err = -EINVAL;
		goto OUT_RELEASE;
	}
	crypt_dentry = ecryptfs_dentry_to_private(path.dentry);
	if (!crypt_dentry) {
		err = -EINVAL;
		goto OUT_RELEASE;
	}
	lower_path = &crypt_dentry->lower_path.dentry->d_name;
	err = copy_to_user(dst, lower_path->name, lower_path->len + 1);

OUT_RELEASE:
	path_put(&path);

	return err;
}

asmlinkage long sys_SYNODecryptName(char __user * root, char __user * src, char __user * dst)
{
	int                           err;
	size_t                        plaintext_name_size = 0;
	char                         *plaintext_name = NULL;
	char                         *token = NULL;
	char                         *szTarget = NULL;
	char                         *root_name = NULL;
	char                         *src_name = NULL;
	char                         *src_walk = NULL;
	struct path					 path;

	if (NULL == src || NULL == root || NULL == dst) {
		return -EINVAL;
	}
	if (!fecryptfs_decode_and_decrypt_filename) {
		return -EPERM;
	}

	src_name = getname(src);
	if (IS_ERR(src_name)) {
		err = PTR_ERR(src_name);
		goto OUT_RELEASE;
	}
	// strsep() will move src_walk, so we should keep the head for free mem
	src_walk = src_name;
	root_name = getname(root);
	if (IS_ERR(root_name)) {
		err = PTR_ERR(root_name);
		goto OUT_RELEASE;
	}
	szTarget = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!szTarget) {
		err = -ENOMEM;
		goto OUT_RELEASE;
	}
	strncpy(szTarget, root_name, PATH_MAX-1);
	szTarget[PATH_MAX-1] = '\0';

	token = strsep(&src_walk, "/");
	if (*token == '\0') {
		token = strsep(&src_walk, "/");
	}
	while (token) {
		err = kern_path(szTarget, LOOKUP_FOLLOW, &path);
		if (err) {
			goto OUT_RELEASE;
		}
		path.dentry->d_flags |= DCACHE_ECRYPTFS_DECRYPT;
		err = fecryptfs_decode_and_decrypt_filename(
			&plaintext_name, &plaintext_name_size, path.dentry, token, strlen(token));
		path.dentry->d_flags &= ~DCACHE_ECRYPTFS_DECRYPT;
		if (err) {
			path_put(&path);
			goto OUT_RELEASE;
		}
		if (PATH_MAX < strlen(szTarget) + plaintext_name_size + 1) {
			path_put(&path);
			goto OUT_RELEASE;
		}
		strcat(szTarget, "/");
		strcat(szTarget, plaintext_name);

		kfree(plaintext_name);
		plaintext_name = NULL;
		path_put(&path);

		token = strsep(&src_walk, "/");
	}
	err = copy_to_user(dst, szTarget, strlen(szTarget)+1);

OUT_RELEASE:
	if (plaintext_name) {
		kfree(plaintext_name);
	}
	if (szTarget) {
		kfree(szTarget);
	}
	if (!IS_ERR(src_name)) {
		putname(src_name);
	}
	if (!IS_ERR(root_name)) {
		putname(root_name);
	}

	return err;
}
#endif

SYSCALL_DEFINE2(newlstat, const char __user *, filename,
		struct stat __user *, statbuf)
{
	struct kstat stat;
	int error;

	error = vfs_lstat(filename, &stat);
	if (error)
		return error;

	return cp_new_stat(&stat, statbuf);
}

#if !defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_SYS_NEWFSTATAT)
SYSCALL_DEFINE4(newfstatat, int, dfd, const char __user *, filename,
		struct stat __user *, statbuf, int, flag)
{
	struct kstat stat;
	int error;

	error = vfs_fstatat(dfd, filename, &stat, flag);
	if (error)
		return error;
	return cp_new_stat(&stat, statbuf);
}
#endif

SYSCALL_DEFINE2(newfstat, unsigned int, fd, struct stat __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_fstat(fd, &stat);

	if (!error)
		error = cp_new_stat(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE4(readlinkat, int, dfd, const char __user *, pathname,
		char __user *, buf, int, bufsiz)
{
	struct path path;
	int error;
	int empty = 0;

	if (bufsiz <= 0)
		return -EINVAL;

	error = user_path_at_empty(dfd, pathname, LOOKUP_EMPTY, &path, &empty);
	if (!error) {
		struct inode *inode = path.dentry->d_inode;

		error = empty ? -ENOENT : -EINVAL;
		if (inode->i_op->readlink) {
			error = security_inode_readlink(path.dentry);
			if (!error) {
				touch_atime(path.mnt, path.dentry);
				error = inode->i_op->readlink(path.dentry,
							      buf, bufsiz);
			}
		}
		path_put(&path);
	}
	return error;
}

SYSCALL_DEFINE3(readlink, const char __user *, path, char __user *, buf,
		int, bufsiz)
{
	return sys_readlinkat(AT_FDCWD, path, buf, bufsiz);
}

#ifdef MY_ABC_HERE
/* This stat is used by caseless protocol.
 * The filename will be convert to real filename and return to user space.
 * In caller, the length of filename must equal or be larger than SYNO_SMB_PSTRING_LEN.
*/
int __SYNOCaselessStat(char __user * filename, struct kstat *stat, unsigned flags, int *lastComponent)
{
	struct path path;
	int error;
	char *real_filename = NULL;
	int real_filename_len = 0;

	real_filename = kmalloc(SYNO_SMB_PSTRING_LEN, GFP_KERNEL);
	if (!real_filename) {
		return -ENOMEM;
	}

#ifdef MY_ABC_HERE
	if (SynoDebugFlag) {
		printk("%s(%d) orig name:[%s] len:[%u]\n", __FUNCTION__, __LINE__, filename, (unsigned int)strlen(filename));
	}
#endif
	error = syno_user_path_at(AT_FDCWD, filename, flags, &path, &real_filename, &real_filename_len, lastComponent);
	if (!error) {
		error = vfs_getattr(path.mnt, path.dentry, stat);
		path_put(&path);
		if (real_filename_len) {
			error = copy_to_user(filename, real_filename, real_filename_len) ? -EFAULT : error;
#ifdef MY_ABC_HERE
			if (SynoDebugFlag) {
				printk("%s(%d) convert name:[%s]\n",__FUNCTION__,__LINE__,filename);
			}
#endif
		}
	}
#ifdef MY_ABC_HERE
	if (error && SynoDebugFlag) {
		printk("%s(%d) convert name:[%s], error:[%d]\n",__FUNCTION__,__LINE__,filename, error);
	}
#endif

	kfree(real_filename);
#ifdef MY_ABC_HERE
	if(syno_hibernation_log_sec > 0) {
		syno_do_hibernation_log(filename);
	}
#endif

	return error;
}
EXPORT_SYMBOL(__SYNOCaselessStat);

asmlinkage long sys_SYNOCaselessStat(char __user * filename, struct stat __user *statbuf)
{
	int lastComponent = 0;
	long error = -1;
	struct kstat stat;

	error = __SYNOCaselessStat(filename, &stat, LOOKUP_FOLLOW|LOOKUP_CASELESS_COMPARE, &lastComponent);
	if (!error) {
		error = cp_new_stat(&stat, statbuf);
	} else if(error == -ENOENT) {
		struct stat tmp;

		memset(&tmp, 0, sizeof(tmp));
		tmp.st_SynoUnicodeStat = lastComponent;
		error = copy_to_user(statbuf, &tmp, sizeof(tmp)) ? -EFAULT : error;
	}

	return error;
}

asmlinkage long sys_SYNOCaselessLStat(char __user * filename, struct stat __user *statbuf)
{
	int lastComponent = 0;
	long error = -1;
	struct kstat stat;

	error = __SYNOCaselessStat(filename, &stat, LOOKUP_CASELESS_COMPARE, &lastComponent);
	if (!error) {
		error = cp_new_stat(&stat, statbuf);
	} else if(error == -ENOENT) {
		struct stat tmp;

		memset(&tmp, 0, sizeof(tmp));
		tmp.st_SynoUnicodeStat = lastComponent;
		error = copy_to_user(statbuf, &tmp, sizeof(tmp)) ? -EFAULT : error;
	}

	return error;
}

#endif


/* ---------- LFS-64 ----------- */
#ifdef __ARCH_WANT_STAT64

static long cp_new_stat64(struct kstat *stat, struct stat64 __user *statbuf)
{
	struct stat64 tmp;

	memset(&tmp, 0, sizeof(struct stat64));
#ifdef CONFIG_MIPS
	/* mips has weird padding, so we don't get 64 bits there */
	if (!new_valid_dev(stat->dev) || !new_valid_dev(stat->rdev))
		return -EOVERFLOW;
	tmp.st_dev = new_encode_dev(stat->dev);
	tmp.st_rdev = new_encode_dev(stat->rdev);
#else
	tmp.st_dev = huge_encode_dev(stat->dev);
	tmp.st_rdev = huge_encode_dev(stat->rdev);
#endif
	tmp.st_ino = stat->ino;
	if (sizeof(tmp.st_ino) < sizeof(stat->ino) && tmp.st_ino != stat->ino)
		return -EOVERFLOW;
#ifdef STAT64_HAS_BROKEN_ST_INO
	tmp.__st_ino = stat->ino;
#endif
	tmp.st_mode = stat->mode;
	tmp.st_nlink = stat->nlink;
	tmp.st_uid = stat->uid;
	tmp.st_gid = stat->gid;
	tmp.st_atime = stat->atime.tv_sec;
	tmp.st_atime_nsec = stat->atime.tv_nsec;
	tmp.st_mtime = stat->mtime.tv_sec;
	tmp.st_mtime_nsec = stat->mtime.tv_nsec;
	tmp.st_ctime = stat->ctime.tv_sec;
	tmp.st_ctime_nsec = stat->ctime.tv_nsec;
#ifdef MY_ABC_HERE
	tmp.st_SynoCreateTime = stat->SynoCreateTime.tv_sec;
#endif
#ifdef	MY_ABC_HERE
	tmp.st_SynoMode = stat->SynoMode;
#endif
#ifdef MY_ABC_HERE
	tmp.st_syno_achv_ver = stat->syno_archive_version;
#endif
	tmp.st_size = stat->size;
	tmp.st_blocks = stat->blocks;
	tmp.st_blksize = stat->blksize;
	return copy_to_user(statbuf,&tmp,sizeof(tmp)) ? -EFAULT : 0;
}

SYSCALL_DEFINE2(stat64, const char __user *, filename,
		struct stat64 __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_stat(filename, &stat);

#ifdef MY_ABC_HERE
	if(syno_hibernation_log_sec > 0) {
		syno_do_hibernation_log(filename);
	}
#endif
	if (!error)
		error = cp_new_stat64(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE2(lstat64, const char __user *, filename,
		struct stat64 __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_lstat(filename, &stat);

	if (!error)
		error = cp_new_stat64(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE2(fstat64, unsigned long, fd, struct stat64 __user *, statbuf)
{
	struct kstat stat;
	int error = vfs_fstat(fd, &stat);

	if (!error)
		error = cp_new_stat64(&stat, statbuf);

	return error;
}

SYSCALL_DEFINE4(fstatat64, int, dfd, const char __user *, filename,
		struct stat64 __user *, statbuf, int, flag)
{
	struct kstat stat;
	int error;

	error = vfs_fstatat(dfd, filename, &stat, flag);
	if (error)
		return error;
	return cp_new_stat64(&stat, statbuf);
}

#ifdef MY_ABC_HERE
asmlinkage long sys_SYNOCaselessStat64(char __user * filename, struct stat64 __user *statbuf)
{
	int lastComponent = 0;
	long error = -1;
	struct kstat stat;

	error = __SYNOCaselessStat(filename, &stat, LOOKUP_FOLLOW|LOOKUP_CASELESS_COMPARE, &lastComponent);
	if (!error) {
		error = cp_new_stat64(&stat, statbuf);
	} else if(error == -ENOENT) {
		struct stat64 tmp;

		memset(&tmp, 0, sizeof(tmp));
		tmp.st_SynoUnicodeStat = lastComponent;
		error = copy_to_user(statbuf, &tmp, sizeof(tmp)) ? -EFAULT : error;
	}

	return error;
}

asmlinkage long sys_SYNOCaselessLStat64(char __user * filename, struct stat64 __user *statbuf)
{
	int lastComponent = 0;
	long error = -1;
	struct kstat stat;

	error = __SYNOCaselessStat(filename, &stat, LOOKUP_CASELESS_COMPARE, &lastComponent);
	if (!error) {
		error = cp_new_stat64(&stat, statbuf);
	} else if(error == -ENOENT) {
		struct stat64 tmp;

		memset(&tmp, 0, sizeof(tmp));
		tmp.st_SynoUnicodeStat = lastComponent;
		error = copy_to_user(statbuf, &tmp, sizeof(tmp)) ? -EFAULT : error;
	}

	return error;
}
#endif

#endif /* __ARCH_WANT_STAT64 */

/* Caller is here responsible for sufficient locking (ie. inode->i_lock) */
void __inode_add_bytes(struct inode *inode, loff_t bytes)
{
	inode->i_blocks += bytes >> 9;
	bytes &= 511;
	inode->i_bytes += bytes;
	if (inode->i_bytes >= 512) {
		inode->i_blocks++;
		inode->i_bytes -= 512;
	}
}

void inode_add_bytes(struct inode *inode, loff_t bytes)
{
	spin_lock(&inode->i_lock);
	__inode_add_bytes(inode, bytes);
	spin_unlock(&inode->i_lock);
}

EXPORT_SYMBOL(inode_add_bytes);

void inode_sub_bytes(struct inode *inode, loff_t bytes)
{
	spin_lock(&inode->i_lock);
	inode->i_blocks -= bytes >> 9;
	bytes &= 511;
	if (inode->i_bytes < bytes) {
		inode->i_blocks--;
		inode->i_bytes += 512;
	}
	inode->i_bytes -= bytes;
	spin_unlock(&inode->i_lock);
}

EXPORT_SYMBOL(inode_sub_bytes);

loff_t inode_get_bytes(struct inode *inode)
{
	loff_t ret;

	spin_lock(&inode->i_lock);
	ret = (((loff_t)inode->i_blocks) << 9) + inode->i_bytes;
	spin_unlock(&inode->i_lock);
	return ret;
}

EXPORT_SYMBOL(inode_get_bytes);

void inode_set_bytes(struct inode *inode, loff_t bytes)
{
	/* Caller is here responsible for sufficient locking
	 * (ie. inode->i_lock) */
	inode->i_blocks = bytes >> 9;
	inode->i_bytes = bytes & 511;
}

EXPORT_SYMBOL(inode_set_bytes);
