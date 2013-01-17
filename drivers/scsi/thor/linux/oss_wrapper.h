/*
 * Operating System Service Wrapper 
 *
 * Notes :
 * ** DO NOT include headers other than common & os specific ones
 *
 * ** This header is included in mv_os.h, you don't have to include it
 *        explicitly in your code.
 *
 */
#ifndef __OSS_WRAPPER_H__
#define __OSS_WRAPPER_H__

#include "com_mod_mgmt.h"

/* Sychronization Services */


/* delay in milliseconds */
MV_U8 ossw_add_timer(struct timer_list *timer,
		    u32 msec, 
		    void (*function)(unsigned long),
		    unsigned long data);

void ossw_del_timer(struct timer_list *timer);

/* OS Information Query Services */
u32 ossw_get_time_in_sec(void);
u32 ossw_get_msec_of_time(void);

/* MISC Services */
#define ossw_mdelay(x) mdelay(x)
#define ossw_udelay(x) udelay(x)

#define MV_PCI_READ_CONFIG_DWORD(ext, offset, ptr) \
          pci_read_config_dword(ext->desc->hba_desc->dev, offset, ptr)

#define MV_PCI_READ_CONFIG_WORD(ext, offset, ptr) \
          pci_read_config_word(ext->desc->hba_desc->dev, offset, ptr)

#define MV_PCI_READ_CONFIG_BYTE(ext, offset, ptr) \
          pci_read_config_byte(ext->desc->hba_desc->dev, offset, ptr)

#define MV_PCI_WRITE_CONFIG_DWORD(ext, offset, val) \
          pci_write_config_dword(ext->desc->hba_desc->dev, offset, val)

#define MV_PCI_WRITE_CONFIG_WORD(ext, offset, val) \
          pci_write_config_word(ext->desc->hba_desc->dev, offset, val)

#define MV_PCI_WRITE_CONFIG_BYTE(ext, offset, val) \
          pci_write_config_byte(ext->desc->hba_desc->dev, offset, val)

/* System dependent macro for flushing CPU write cache */
#define MV_CPU_WRITE_BUFFER_FLUSH() smp_wmb()
#define MV_CPU_READ_BUFFER_FLUSH()  smp_rmb()
#define MV_CPU_BUFFER_FLUSH()       smp_mb()

/* register read write: memory io */
#define MV_REG_WRITE_BYTE(base, offset, val)    \
    writeb(val, base + offset)
#define MV_REG_WRITE_WORD(base, offset, val)    \
    writew(val, base + offset)
#define MV_REG_WRITE_DWORD(base, offset, val)    \
    writel(val, base + offset)

#define MV_REG_READ_BYTE(base, offset)			\
	readb(base + offset)
#define MV_REG_READ_WORD(base, offset)			\
	readw(base + offset)
#define MV_REG_READ_DWORD(base, offset)			\
	readl(base + offset)

/* register read write: port io */
#define MV_IO_WRITE_BYTE(base, offset, val)    \
    outb(val, (unsigned)(MV_PTR_INTEGER)(base + offset))
#define MV_IO_WRITE_WORD(base, offset, val)    \
    outw(val, (unsigned)(MV_PTR_INTEGER)(base + offset))
#define MV_IO_WRITE_DWORD(base, offset, val)    \
    outl(val, (unsigned)(MV_PTR_INTEGER)(base + offset))

#define MV_IO_READ_BYTE(base, offset)			\
	inb((unsigned)(MV_PTR_INTEGER)(base + offset))
#define MV_IO_READ_WORD(base, offset)			\
	inw((unsigned)(MV_PTR_INTEGER)(base + offset))
#define MV_IO_READ_DWORD(base, offset)			\
	inl((unsigned)(MV_PTR_INTEGER)(base + offset))

#if __LEGACY_OSSW__
void HBA_SleepMillisecond(MV_PVOID ext, MV_U32 msec);

#   define HBA_SleepMicrosecond(_x, _y) ossw_udelay(_y)
#   define HBA_GetTimeInSecond          ossw_get_time_in_sec
#   define HBA_GetMillisecondInDay      ossw_get_msec_of_time

#   define NO_CURRENT_TIMER  0xFF

#   define SmallTimer_AddRequest(_ext, _millsec, _func, _data, _bogus)    \
             ossw_add_timer(&_ext->timer,                               \
			     _millsec ,                           \
			     (OSSW_TIMER_FUNCTION) (_func),         \
			     (OSSW_TIMER_DATA) (_data))



#   define Timer_AddRequest(_ext, _sec, _func, _data, _bogus)    \
             ossw_add_timer(&_ext->timer,                               \
			     _sec * 1000,                           \
			     (OSSW_TIMER_FUNCTION) (_func),         \
			     (OSSW_TIMER_DATA) (_data))

#   define Timer_CancelRequest(_data, _bogus)                     \
              ossw_del_timer(&_data->timer)

#endif /* __LEGACY_OSSW__ */

void hba_msleep(MV_U32 msec);

#endif /* __OSS_WRAPPER_H__ */
