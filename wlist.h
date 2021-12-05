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
    INIT_LIST_HEAD(&(wlist->head));
}

//whead : prev
//whead->next : next
//insert
static inline void wlist_add(struct wlist_head *new, struct wlist_head *whead)
{
    // flag true false 인지 확인
    // true -> msleep
    while (whead->flag | (container_of(whead->head.next, struct wlist_head, head))->flag) {
    	printk("stuck\n");
        msleep(10);
    }
    //printk("no stuck\n");
    // false -> 그대로 진행
    // flag 달기 (next, new, prev)
    whead->flag = true;
    (container_of(whead->head.next, struct wlist_head, head))->flag = true;
    list_add(&(new->head), &(whead->head));
    whead->flag = false;
    (container_of(new->head.next, struct wlist_head, head))->flag = false;
    
}

//delete
static inline void wlist_del(struct wlist_head *entry)
{
    __list_del_entry(&(entry->head));
	entry->head.next = WLIST_POISON1;
	entry->head.prev = WLIST_POISON2;
}



// #define list_for_each_entry_safe(pos, n, head, member)			\
// 	for (pos = list_first_entry(head, typeof(*pos), member),	\
// 		n = list_next_entry(pos, member);			\
// 	     !list_entry_is_head(pos, head, member); 			\
// 	     pos = n, n = list_next_entry(n, member))

//search
// whead_ptr: wlist_head *

/*

#define wlist_for_each_entry_safe(pos, n, whead_ptr, member)    \
    for (pos = container_of(list_first_entry((whead_ptr)->head, typeof((pos)->whead), member), typeof(pos), whead_ptr), \
            n = container_of(list_next_entry((pos)->whead, member), typeof(n), whead_ptr);			\
            !list_entry_is_head(pos->whead, (whead_ptr)->head, member); 			\
            pos = n, n = container_of(list_next_entry(n->whead, member), typeof(n), whead_ptr))
            



#define wlist_for_each_entry_safe(pos, n, whead_ptr, member)\
	for(pos = container_of(list_first_entry(whead_ptr, typeof(&((pos)->whead)), member),typeof(*pos),whead), \
	 n = container_of(list_next_entry(&((pos)->whead), member), typeof(*n), whead) ; \
	 !list_entry_is_head(&((pos)->whead), whead_ptr, member); \
	 pos = n, n = container_of(list_next_entry(&((pos)->whead), member), typeof(*n), whead))

*/
