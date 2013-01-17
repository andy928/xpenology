#
# Makefile for the Linux 2.6 eCryptfs
#
#SYNO
EXTRA_CFLAGS += -I$(srctree)/crypto/ocf_20100325

obj-$(CONFIG_ECRYPT_FS) += ecryptfs.o

ecryptfs-objs := dentry.o file.o inode.o main.o super.o mmap.o read_write.o crypto.o keystore.o messaging.o miscdev.o kthread.o debug.o
