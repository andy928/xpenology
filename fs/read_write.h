/*
 * This file is only for sharing some helpers from read_write.c with compat.c.
 * Don't use anywhere else.
 */
#ifdef CONFIG_ARCH_FEROCEON
#ifndef _READ_WRITE_H_
#define _READ_WRITE_H_

#include <net/sock.h>

#define WRITE_RECEIVE_SIZE 131072

#ifdef COLLECT_WRITE_SOCK_TO_FILE_STAT
struct write_sock_to_file_stat {
	unsigned long errors;
	unsigned long buf_4k;
	unsigned long buf_8k;
	unsigned long buf_16k;
	unsigned long buf_32k;
	unsigned long buf_64k;
	unsigned long buf_128k;
};
#define INC_WRITE_FROM_SOCK_ERR_CNT	(write_from_sock.errors++)
#define INC_WRITE_FROM_SOCK_4K_BUF_CNT	(write_from_sock.buf_4k++)
#define INC_WRITE_FROM_SOCK_8K_BUF_CNT	(write_from_sock.buf_8k++)
#define INC_WRITE_FROM_SOCK_16K_BUF_CNT	(write_from_sock.buf_16k++)
#define INC_WRITE_FROM_SOCK_32K_BUF_CNT	(write_from_sock.buf_32k++)
#define INC_WRITE_FROM_SOCK_64K_BUF_CNT	(write_from_sock.buf_64k++)
#define INC_WRITE_FROM_SOCK_128K_BUF_CNT	(write_from_sock.buf_128k++)
#else /* COLLECT_WRITE_SOCK_TO_FILE_STAT */
#define INC_WRITE_FROM_SOCK_ERR_CNT
#endif /* COLLECT_WRITE_SOCK_TO_FILE_STAT */


#ifdef DEBUG
#define dprintk(fmt, args...) printk(KERN_ERR "%s: " fmt, __func__, ##args)
#else
#define dprintk(fmt, args...)
#endif
#endif /* _READ_WRITE_H_ */
#endif /* CONFIG_ARCH_FEROCEON */

typedef ssize_t (*io_fn_t)(struct file *, char __user *, size_t, loff_t *);
typedef ssize_t (*iov_fn_t)(struct kiocb *, const struct iovec *,
		unsigned long, loff_t);

ssize_t do_sync_readv_writev(struct file *filp, const struct iovec *iov,
		unsigned long nr_segs, size_t len, loff_t *ppos, iov_fn_t fn);
ssize_t do_loop_readv_writev(struct file *filp, struct iovec *iov,
		unsigned long nr_segs, loff_t *ppos, io_fn_t fn);

#ifdef CONFIG_ARCH_FEROCEON
#ifdef _READ_WRITE_H_
ssize_t write_from_socket_to_file(struct socket *sock, struct file *file,
		loff_t __user *ppos, size_t len);
#endif /* _READ_WRITE_H_ */
#endif /* CONFIG_ARCH_FEROCEON */


