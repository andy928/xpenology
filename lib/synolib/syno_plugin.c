/* kernel header */
#include <linux/synolib.h>
#include <linux/list.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/sched.h>


/* user-defined header */


/* static definition */
#define SYNO_PLUGIN_ERR(msg, arg...)    printk(KERN_ERR "SYNO_PLUGIN_ERR: %s() " msg, __FUNCTION__, ##arg)


typedef struct syno_plugin_s {
	void               *instance;
	int                 magic;
	int                 access_cnt;
	struct list_head    list;
} syno_plugin_t;


/* static variables */
static LIST_HEAD(sg_syno_plugin_list);
static DEFINE_SPINLOCK(sg_syno_plugin_lock);


/* ------- function implementation ------- */
static inline void syno_plugin_lock(void)
{
	spin_lock(&sg_syno_plugin_lock);
}

static inline void syno_plugin_unlock(void)
{
	spin_unlock(&sg_syno_plugin_lock);
}

static void __syno_plugin_free(syno_plugin_t *plugin)
{
	kfree(plugin);
}

static syno_plugin_t * __syno_plugin_alloc(int plugin_magic, void *instance)
{
	syno_plugin_t *plugin;

	plugin = kmalloc(sizeof(syno_plugin_t), GFP_KERNEL);
	if (!plugin) {
		SYNO_PLUGIN_ERR("kmalloc fail");
		goto EXIT;
	}

	plugin->magic = plugin_magic;
	plugin->instance = instance;
	plugin->access_cnt = 0;
	INIT_LIST_HEAD(&plugin->list);

EXIT:
	return plugin;
}

int syno_plugin_register(int plugin_magic, void *instance)
{
	int ret;
	struct list_head *pos;
	syno_plugin_t *plugin = NULL;
	syno_plugin_t *tmp;

	if (!instance) {
		ret = -EINVAL;
		goto EXIT;
	}

	plugin = __syno_plugin_alloc(plugin_magic, instance);
	if (!plugin) {
		ret = -ENOMEM;
		goto EXIT;
	}

	/* check if the plug-in has already registered */
	syno_plugin_lock();

	if (!list_empty(&sg_syno_plugin_list)) {
		list_for_each(pos, &sg_syno_plugin_list) {
			tmp = list_entry(pos, syno_plugin_t, list);
			if (tmp->magic == plugin_magic) {
				ret = -EEXIST;
				goto UNLOCK;
			}
		}
	}

	list_add_tail(&plugin->list, &sg_syno_plugin_list);

	ret = 0;

UNLOCK:
	syno_plugin_unlock();
	if (0 > ret) {
		if (plugin) {
			__syno_plugin_free(plugin);
		}
	}
EXIT:
	return ret;
}
EXPORT_SYMBOL(syno_plugin_register);

int syno_plugin_unregister(int plugin_magic)
{
	int ret;
	struct list_head *pos;
	syno_plugin_t *tmp = NULL;
	syno_plugin_t *plugin = NULL;

	syno_plugin_lock();

	list_for_each(pos, &sg_syno_plugin_list) {
		tmp = list_entry(pos, syno_plugin_t, list);
		if (tmp->magic == plugin_magic) {
			plugin = tmp;
			break;
		}
	}

	if (!plugin) {
		ret = -ENOENT;
		goto EXIT;
	}

	/* wait until no one use the plugin */
	if (0 < plugin->access_cnt) {
		ret = -EBUSY;
		goto EXIT;
	}

	list_del(&plugin->list);

	ret = 0;

EXIT:
	syno_plugin_unlock();

	if (0 == ret) {
		__syno_plugin_free(plugin);
	}

	return ret;
}
EXPORT_SYMBOL(syno_plugin_unregister);

int syno_plugin_handle_get(int plugin_magic, void **hnd)
{
	int ret;
	struct list_head *pos;
	syno_plugin_t *plugin;

	*hnd = NULL;
	ret = -ENOENT;

	syno_plugin_lock();

	list_for_each(pos, &sg_syno_plugin_list) {
		plugin = list_entry(pos, syno_plugin_t, list);
		if (plugin->magic == plugin_magic) {
			plugin->access_cnt++;
			*hnd = plugin;
			ret = 0;
			break;
		}
	}

	syno_plugin_unlock();

	return ret;
}
EXPORT_SYMBOL(syno_plugin_handle_get);

void * syno_plugin_handle_instance(void *hnd)
{
	if (hnd) {
		return ((syno_plugin_t *)hnd)->instance;
	} else {
		return NULL;
	}
}
EXPORT_SYMBOL(syno_plugin_handle_instance);

void syno_plugin_handle_put(void *hnd)
{
	syno_plugin_t *plugin;

	if (!hnd) {
		return;
	}

	plugin = (syno_plugin_t *)hnd;

	syno_plugin_lock();

	if (0 < plugin->access_cnt) {
		plugin->access_cnt--;
	} else {
		SYNO_PLUGIN_ERR("access_cnt is zero");
	}

	syno_plugin_unlock();
}
EXPORT_SYMBOL(syno_plugin_handle_put);

