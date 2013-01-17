/*
 *  linux/arch/arm/lib/uaccess_memcpy.c
 *
 *  Copyright (C) 2009 Lennert Buytenhek <buytenh@wantstofly.org>
 *  Copyright (C) 2009 Marvell Semiconductor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include <linux/rwsem.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/page.h>
#include <linux/sysdev.h>

/*
 * Called with mm semaphore held.
 */
static int
pin_page_for_write(const void __user *_addr, pte_t **ptep, spinlock_t **ptlp)
{
	unsigned long addr = (unsigned long)_addr;
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	spinlock_t *ptl;

	pgd = pgd_offset(current->mm, addr);
	if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
		return 0;

	pmd = pmd_offset(pgd, addr);
	if (pmd_none(*pmd) || unlikely(pmd_bad(*pmd)))
		return 0;

	pte = pte_offset_map_lock(current->mm, pmd, addr, &ptl);
	if (!pte_present(*pte) || !pte_young(*pte) ||
	    !pte_write(*pte) || !pte_dirty(*pte)) {
		pte_unmap_unlock(pte, ptl);
		return 0;
	}

	*ptep = pte;
	*ptlp = ptl;

	return 1;
}

#if defined (CONFIG_MV_XOR_MEMCOPY) || defined (CONFIG_MV_IDMA_MEMCOPY)
#define memcpy asm_memcpy
#endif

unsigned long ___copy_to_user(void *to, const void *from, unsigned long n)
{
	if (get_fs() == KERNEL_DS) {
		memcpy(to, from, n);
		return 0;
	}

	if (n < 256)
		return __arch_copy_to_user(to, from, n);

	down_read(&current->mm->mmap_sem);
	while (n) {
		pte_t *pte;
		spinlock_t *ptl;
		int tocopy;

		while (unlikely(!pin_page_for_write(to, &pte, &ptl))) {
			up_read(&current->mm->mmap_sem);
			if (put_user(*((u8 *)from), (u8 *)to))
				goto out;
			down_read(&current->mm->mmap_sem);
		}

		tocopy = ((~((unsigned long)to)) & (PAGE_SIZE - 1)) + 1;
		if (tocopy > n)
			tocopy = n;

		memcpy(to, from, tocopy);
		to += tocopy;
		from += tocopy;
		n -= tocopy;

		pte_unmap_unlock(pte, ptl);
	}

out:
	up_read(&current->mm->mmap_sem);

	return n;
}
#ifndef CONFIG_UACCESS_WITH_MEMCPY
EXPORT_SYMBOL(___copy_to_user);
#endif

