#ifndef __MODULE_MANAGE_H__
#define __MODULE_MANAGE_H__
#include "hba_header.h"

#include "com_mod_mgmt.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 11)
/* will wait for atomic value atomic to become zero until timed out */
/* return how much 'timeout' is left or 0 if already timed out */
int __hba_wait_for_atomic_timeout(atomic_t *atomic, unsigned long timeout);
#endif

int  mv_hba_init(struct pci_dev *dev, MV_U32 max_io);
void mv_hba_release(struct pci_dev *dev);
void mv_hba_stop(struct pci_dev *dev);
int  mv_hba_start(struct pci_dev *dev);

int __mv_get_adapter_count(void);
struct hba_extension *__mv_get_ext_from_adp_id(int id);
 void raid_capability( MV_PVOID This, PAdapter_Info pAdInfo);
#endif /* __MODULE_MANAGE_H__ */

