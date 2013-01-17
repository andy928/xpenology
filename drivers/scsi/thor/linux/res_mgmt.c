#include "hba_header.h"
#include "res_mgmt.h"
#include "oss_wrapper.h"
#include "linux_main.h"

struct mv_request_pool *res_reserve_req_pool(MV_U32 mod_id,
					     MV_U32 size,
					     MV_U32 sg_count)
{
	int i;
	unsigned int mem_size;
	struct mv_request_pool *pool;
	struct _MV_Request *req;
	MV_SG_Entry *sg;

	MV_DBG(DMSG_RES, "module %u reserves reqcount=%d, sgcount=%d,request pool of size %lu.\n",
	       mod_id,size,sg_count,
	       (unsigned long) (sizeof(struct _MV_Request) * size + 
				sizeof(MV_SG_Entry)* sg_count * size));

	/* 
	 * since we almost always use sgd_t with pctx, we need x2 memory
	 * than normal sgd_t table
	 */
	sg_count *= 2;

	mem_size = sizeof(struct mv_request_pool);
	pool = vmalloc(mem_size);
	if (NULL == pool)
		goto res_err_pool;
	memset(pool, 0, mem_size);

	/* assuming its size is already 64bit-aligned */
	mem_size = sizeof(struct _MV_Request) * size;
	req = vmalloc(mem_size);
	if (NULL == req)
		goto res_err_req;
	memset(req, 0, mem_size);

	mem_size = sizeof(MV_SG_Entry) * sg_count * size;
	sg  = vmalloc(mem_size);
	if (NULL == sg)
		goto res_err_sg;
	memset(sg, 0, mem_size);



	INIT_LIST_HEAD(&pool->free_list);
	INIT_LIST_HEAD(&pool->use_list);
	OSSW_INIT_SPIN_LOCK(&pool->lock);

	pool->mod_id  = mod_id;
	pool->size    = size;
	pool->req_mem = (void *) req;
	pool->sg_mem = (void *)sg;
	
	for (i = 0; i < size; i++) {
		//MV_DBG(DMSG_RES, "req no.%d at %p with sg at %p.\n",
		 //      i, req, sg);

		req->SG_Table.Entry_Ptr  = sg;
		MV_ASSERT(sg_count<=255);
		req->SG_Table.Max_Entry_Count = sg_count;
		list_add_tail(&req->pool_entry, &pool->free_list);
		req++;
		sg += sg_count;
	}
#ifdef SEPARATE_IOCTL_REQ
	INIT_LIST_HEAD(&pool->ioctl_free_list);
	INIT_LIST_HEAD(&pool->ioctl_use_list);
	OSSW_INIT_SPIN_LOCK(&pool->ioctl_lock);
	mem_size = sizeof(struct _MV_Request) * MV_MAX_IOCTL_REQUEST;
	req = vmalloc(mem_size);
	if (NULL == req)
		goto res_err_req;
	memset(req, 0, mem_size);

	mem_size = sizeof(MV_SG_Entry) * sg_count * MV_MAX_IOCTL_REQUEST;
	sg  = vmalloc(mem_size);
	if (NULL == sg)
		goto res_err_sg;
	memset(sg, 0, mem_size);

	pool->ioctl_req_mem = (void *) req;
	pool->ioctl_sg_mem = (void *)sg;

	for (i = 0; i < MV_MAX_IOCTL_REQUEST; i++) {
		req->SG_Table.Entry_Ptr  = sg;
		MV_ASSERT(sg_count<=255);
		req->SG_Table.Max_Entry_Count = sg_count;
		list_add_tail(&req->pool_entry, &pool->ioctl_free_list);
		req++;
		sg += sg_count;
	}
	
#endif
		
	return pool;

res_err_sg:
	MV_ASSERT(MV_FALSE);
	vfree(req);
res_err_req:
	MV_ASSERT(MV_FALSE);
	vfree(pool);
res_err_pool:
	MV_ASSERT(MV_FALSE);
	return NULL;
}

#ifdef SUPPORT_OS_REQ_TIMER
void mv_init_os_req_timer(struct _MV_Request * req);
#endif

struct _MV_Request *res_get_req_from_pool(struct mv_request_pool *pool)
{
	struct _MV_Request *req;
	unsigned long flags;

	BUG_ON(pool == NULL);
	OSSW_SPIN_LOCK_IRQSAVE(&pool->lock, flags);
	if (list_empty(&pool->free_list)) {
		res_dump_pool_info(pool);
		OSSW_SPIN_UNLOCK_IRQRESTORE(&pool->lock, flags);
		return NULL;
	}
	
	req = list_entry(pool->free_list.next, struct _MV_Request, pool_entry);
	MV_ZeroMvRequest(req); /* FIX: we can do better than this */
	list_move_tail(pool->free_list.next, &pool->use_list);
#ifdef SUPPORT_OS_REQ_TIMER
	mv_init_os_req_timer(req);
#endif
	OSSW_SPIN_UNLOCK_IRQRESTORE(&pool->lock, flags);
	
	return req;
}

void res_free_req_to_pool(struct mv_request_pool *pool,
			  struct _MV_Request *req)
{
	unsigned long flags;

	BUG_ON((NULL == pool) || (NULL == req));
	OSSW_SPIN_LOCK_IRQSAVE(&pool->lock, flags);
	list_move_tail(&req->pool_entry, &pool->free_list);
	OSSW_SPIN_UNLOCK_IRQRESTORE(&pool->lock, flags);
}

void res_dump_pool_info(struct mv_request_pool *pool)
{
	
}

void res_release_req_pool(struct mv_request_pool *pool)
{
	BUG_ON(NULL == pool);
	
	MV_DBG(DMSG_RES, "module %d release pool at %p.\n",
	       pool->mod_id, pool);
	vfree(pool->req_mem);
	vfree(pool->sg_mem);
#ifdef SEPARATE_IOCTL_REQ
	vfree(pool->ioctl_req_mem);
	vfree(pool->ioctl_sg_mem);
#endif
	vfree(pool);
}
#ifdef SEPARATE_IOCTL_REQ
struct _MV_Request *get_ioctl_req_from_pool(struct mv_request_pool *pool)
{
	struct _MV_Request *req;
	unsigned long flags;
	
	BUG_ON(pool == NULL);
	OSSW_SPIN_LOCK_IRQSAVE(&pool->ioctl_lock, flags);
	if (list_empty(&pool->ioctl_free_list)) {
		res_dump_pool_info(pool);
		OSSW_SPIN_UNLOCK_IRQRESTORE(&pool->ioctl_lock, flags);
		return NULL;
	}
	
	req = list_entry(pool->ioctl_free_list.next, struct _MV_Request, pool_entry);
	MV_ZeroMvRequest(req); 
	list_move_tail(pool->ioctl_free_list.next, &pool->ioctl_use_list);
	OSSW_SPIN_UNLOCK_IRQRESTORE(&pool->ioctl_lock, flags);
	
	return req;
}

void free_ioctl_req_to_pool(struct mv_request_pool *pool,struct _MV_Request *req)
{
	unsigned long flags;

	BUG_ON((NULL == pool) || (NULL == req));
	OSSW_SPIN_LOCK_IRQSAVE(&pool->ioctl_lock, flags);
	list_move_tail(&req->pool_entry, &pool->ioctl_free_list);
	OSSW_SPIN_UNLOCK_IRQRESTORE(&pool->ioctl_lock, flags);
}

#endif
