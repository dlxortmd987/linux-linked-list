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

//search
#define wlist_for_each_entry_safe(pos, n, wlist, member)
    for ((pos)->whead = list_first_entry((wlist)->head, typeof(*((pos)->whead)), member),	
            (n)->whead = list_next_entry((pos)->whead, member);			\
            !list_entry_is_head(pos->whead, wlist->head, member); 			\
            (pos)->whead = (n)->whead, (n)-> = list_next_entry(n->whead, member), (pos)->data = (n)->data)