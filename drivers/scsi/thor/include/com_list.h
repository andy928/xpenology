#if !defined(COMMON_LIST_H)
#define COMMON_LIST_H

#include "com_define.h"

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

#ifdef NEW_LINUX_DRIVER
/* use native code */
#define List_Head           struct list_head
#define PList_Head          struct list_head *

#define MV_LIST_HEAD        LIST_HEAD
#define MV_INIT_LIST_HEAD   LIST_HEAD_INIT
#define MV_LIST_HEAD_INIT   INIT_LIST_HEAD

//#define List_Add            list_add
//#define List_AddTail        list_add_tail
//#define List_Del            list_del
#define List_DelInit        list_del_init
#define List_Move           list_move
#define List_MoveTail       list_move_tail
#define List_Empty          list_empty

#define CONTAINER_OF        container_of
#define LIST_ENTRY          list_entry
#define LIST_FOR_EACH       list_for_each
#define LIST_FOR_EACH_PREV  list_for_each_prev
#define LIST_FOR_EACH_ENTRY list_for_each_entry
#define LIST_FOR_EACH_ENTRY_PREV list_for_each_entry_prev
#define LIST_FOR_EACH_ENTRY_TYPE(ptr, list_ptr, type, entry) \
           list_for_each_entry(ptr, list_ptr, entry)

#define List_Splice         list_splice
#define List_Splice_Init    list_splice_init
#define __List_Del          __list_del  /* internal function, use with care */
#if 0

static MV_INLINE void List_Del(List_Head *entry)
{
#ifdef SUPPORT_REQUEST_TIMER
	if((entry->prev) || (entry->next))
#endif
	__List_Del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}


static inline struct list_head *List_GetFirst(struct list_head *head)
{
        struct list_head * one = NULL;
        if (list_empty(head))
		return NULL;

        one = head->next;
        list_del(one);
        return one;
}

static inline struct list_head *List_GetLast(struct list_head *head)
{
        struct list_head * one = NULL;
        if (list_empty(head))
		return NULL;

        one = head->prev;
        list_del(one);
        return one;
}
#endif

#else /* _OS_LINUX */
/*
 *
 *
 * Data Structure
 *
 *
 */
typedef struct _List_Head {
	struct _List_Head *prev, *next;
} List_Head, * PList_Head;


/*
 *
 *
 * Exposed Functions
 *
 *
 */
 
#define MV_LIST_HEAD(name) \
	List_Head name = { &(name), &(name) }

#define MV_LIST_HEAD_INIT(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static MV_INLINE void List_Add(List_Head *new_one, List_Head *head);

static MV_INLINE void List_AddTail(List_Head *new_one, List_Head *head);

static MV_INLINE void List_Del(List_Head *entry);

static MV_INLINE void List_DelInit(List_Head *entry);

static MV_INLINE void List_Move(List_Head *list, List_Head *head);

static MV_INLINE void List_MoveTail(List_Head *list,
				  List_Head *head);

static MV_INLINE int List_Empty(const List_Head *head);


#define CONTAINER_OF(ptr, type, member) 			\
        ( (type *)( (char *)(ptr) - OFFSET_OF(type,member) ) )

#define LIST_ENTRY(ptr, type, member) \
	CONTAINER_OF(ptr, type, member)

/**
 * LIST_FOR_EACH	-	iterate over a list
 * @pos:	the &List_Head to use as a loop counter.
 * @head:	the head for your list.
 */
#define LIST_FOR_EACH(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * LIST_FOR_EACH_PREV	-	iterate over a list backwards
 * @pos:	the &List_Head to use as a loop counter.
 * @head:	the head for your list.
 */
#define LIST_FOR_EACH_PREV(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * LIST_FOR_EACH_ENTRY	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define LIST_FOR_EACH_ENTRY(pos, head, member)				\
	for (pos = LIST_ENTRY((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = LIST_ENTRY(pos->member.next, typeof(*pos), member))

/**
 * LIST_FOR_EACH_ENTRY_TYPE	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 * @type:	the type of the struct this is embedded in.
*/
#define LIST_FOR_EACH_ENTRY_TYPE(pos, head, type, member)       \
	for (pos = LIST_ENTRY((head)->next, type, member);	\
	     &pos->member != (head); 	                        \
	     pos = LIST_ENTRY(pos->member.next, type, member))

/**
 * LIST_FOR_EACH_ENTRY_PREV - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define LIST_FOR_EACH_ENTRY_PREV(pos, head, member)			\
	for (pos = LIST_ENTRY((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = LIST_ENTRY(pos->member.prev, typeof(*pos), member))

#ifndef _OS_BIOS
#include "com_list.c"
#endif

#endif /* _OS_LINUX */

#define List_GetFirstEntry(head, type, member)	\
	LIST_ENTRY(List_GetFirst(head), type, member)

#define List_GetLastEntry(head, type, member)	\
	LIST_ENTRY(List_GetLast(head), type, member)

#endif /* COMMON_LIST_H */
