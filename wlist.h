#include <linux/list.h>
#include <linux/slab.h>
#include <linux/delay.h>

struct wlist_head {
    struct list_head head;
    bool flag;
};


static inline void INIT_WLIST_HEAD(struct wlist_head * wlist){
    INIT_LIST_HEAD(wlist->head);
}

static inline void 