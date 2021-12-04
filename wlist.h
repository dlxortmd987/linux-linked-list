#include <linux/list.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define WLIST_POISON1   ((void*) 0x00100100)
#define WLIST_POISON2   ((void*) 0x00200200)

struct wlist_head {
    struct list_head head;
    bool flag;
};


static inline void INIT_WLIST_HEAD(struct wlist_head * wlist){
    INIT_LIST_HEAD(wlist->head);
}

//insert
static inline void wlist_add(struct wlist_head *new, struct wlist_head *whead)
{
    __list_add(new->head, whead->head);
}

//delete
static inline void wlist_del(struct wlist_head *entry)
{
    __list_del_entry(entry->head);
	entry->head.next = WLIST_POISON1;
	entry->head.prev = WLIST_POISON2;
}

#define offsetof(s,m)   (size_t)&(((s *)0)->m)

#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))

//search
#define wlist_for_each_entry_safe(pos, n, whead, member)
    for (pos = container_of(list_first_entry((whead)->head, typeof(*((pos)->whead)), member), typeof(*pos), whead),	
            n = container_of(list_next_entry((pos)->whead, member), typeof(*n), whead);			\
            !list_entry_is_head(pos->whead, whead->head, member); 			\
            pos = n, n = container_of(list_next_entry(n->whead, member), typeof(*n), whead))