/*
 * Copyright (C) 2005-2011 Junjiro R. Okajima
 *
 * This program, aufs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * support for loopback block device as a branch
 */

#include <linux/loop.h>
#include "aufs.h"

/*
 * test if two lower dentries have overlapping branches.
 */
int au_test_loopback_overlap(struct super_block *sb, struct dentry *h_adding)
{
	struct super_block *h_sb;
	struct loop_device *l;

	h_sb = h_adding->d_sb;
	if (MAJOR(h_sb->s_dev) != LOOP_MAJOR)
		return 0;

	l = h_sb->s_bdev->bd_disk->private_data;
	h_adding = l->lo_backing_file->f_dentry;
	/*
	 * h_adding can be local NFS.
	 * in this case aufs cannot detect the loop.
	 */
	if (unlikely(h_adding->d_sb == sb))
		return 1;
	return !!au_test_subdir(h_adding, sb->s_root);
}

/* true if a kernel thread named 'loop[0-9].*' accesses a file */
int au_test_loopback_kthread(void)
{
	int ret;
	struct task_struct *tsk = current;

	ret = 0;
	if (tsk->flags & PF_KTHREAD) {
		const char c = tsk->comm[4];
		ret = ('0' <= c && c <= '9'
		       && !strncmp(tsk->comm, "loop", 4));
	}

	return ret;
}
