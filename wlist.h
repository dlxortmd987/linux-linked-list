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
        msleep(1);
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
    // flag 확인 (현재 노드(entry), 다음 노드, 이전 노드)
    // true
    while (entry->flag | (container_of(entry->head.next, struct wlist_head, head))->flag | (container_of(entry->head.prev, struct wlist_head, head))->flag) {
        msleep(1);
    }
    // false
    entry->flag = true;
    (container_of(entry->head.next, struct wlist_head, head))->flag = true;
    (container_of(entry->head.prev, struct wlist_head, head))->flag = true;
    __list_del_entry(&(entry->head));
    (container_of(entry->head.next, struct wlist_head, head))->flag = false;
    (container_of(entry->head.prev, struct wlist_head, head))->flag = false;
	entry->head.next = WLIST_POISON1;
	entry->head.prev = WLIST_POISON2;
}

