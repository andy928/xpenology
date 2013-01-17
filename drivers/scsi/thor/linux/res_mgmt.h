#ifndef __RES_MGMT_H__
#define __RES_MGMT_H__

#include "com_type.h"

struct mv_request_pool {
	struct list_head free_list;
	struct list_head use_list;
#ifdef SEPARATE_IOCTL_REQ
	struct list_head ioctl_free_list;
	struct list_head ioctl_use_list;
	void 		*ioctl_sg_mem;
	void 		*ioctl_req_mem;
	spinlock_t  ioctl_lock;
#endif

	void        *req_mem;  /* starting address of the request mem pool */
	void 		*sg_mem;
	spinlock_t  lock;
	MV_U32      mod_id;
	MV_U32      size;
};

/* request structure related
 * mod_id   : id of the module that is making the request
 * size     : size of the pool, measured in the number of requests
 * sg_count : scatter gather list entries supported by each request
 */
struct mv_request_pool *res_reserve_req_pool(MV_U32 mod_id,
					     MV_U32 size,
					     MV_U32 sg_count);

struct _MV_Request *res_get_req_from_pool(struct mv_request_pool *pool);

void res_free_req_to_pool(struct mv_request_pool *pool,
			  struct _MV_Request *req);

void res_release_req_pool(struct mv_request_pool *pool);

void res_dump_pool_info(struct mv_request_pool *pool);
#ifdef SEPARATE_IOCTL_REQ
struct _MV_Request *get_ioctl_req_from_pool(struct mv_request_pool *pool);
void free_ioctl_req_to_pool(struct mv_request_pool *pool,struct _MV_Request *req);
#endif


#endif /* __RES_MGMT_H__ */
