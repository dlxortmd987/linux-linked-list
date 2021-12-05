// test code
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h> // list.h
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#include "wlist.h"


#define BILLION 1000000000 

spinlock_t counter_lock;
struct timespec64 spclock[2];

#define COUNT 300 // the number of nodes

unsigned long long res_time;
int cnt = 1;

struct task_struct* writer_thread1, * writer_thread2, * writer_thread3, * writer_thread4;

struct my_node {
	struct wlist_head whead;
	int data;
};

struct wlist_head my_list; //head
struct my_node * cnode = NULL;
struct wlist_head *current_node = NULL;
struct wlist_head *tmp = NULL;

void test(void);
unsigned long long calclock3(struct timespec64 *spclock, unsigned long long *total_time);


int __init mod_init(void){
	spin_lock_init(&counter_lock);
	INIT_WLIST_HEAD(&my_list); // initialize list head
	printk("modified kernel data structure\n");
	
	test();
	printk("hello module! \n");
	return 0;
}

void __exit mod_cleanup(void) {
	
	printk("bye module \n");
}

module_init(mod_init);
module_exit(mod_cleanup);
MODULE_LICENSE("GPL");


int winsert(void* data) 
{
	int i;

	for (i = 0; i < COUNT; i++){
		struct my_node *new = kmalloc(sizeof(struct my_node),GFP_KERNEL);
		new->data = i;
		wlist_add(&new->whead, &my_list);
	}	
	return 0;
}

//travarsal and insert
int wsearch(void* data)
{
	int i;
	for (i = 0; i < COUNT; i++){
		list_for_each_entry_safe(current_node,tmp, &(my_list.head), head ){
			cnode = container_of(current_node, struct my_node, whead);
			if(cnode->data == i){
				struct my_node *new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
				new->data = i+1000;
				wlist_add(&new->whead, &(cnode->whead));
			}
		}
	}	
	return 0;
}

int wdelete(void* data){
	
	list_for_each_entry_safe(current_node, tmp, &(my_list.head), head){
		cnode = container_of(current_node, struct my_node, whead);
		if(cnode->data == cnt++){
			wlist_del(&(cnode->whead));
			kfree(cnode);
		}
	}
	
	return 0;
}

void test(void) 
{
	res_time = 0;
	
	int i;
	res_time = 0;
	
	//insert
	ktime_get_real_ts64(&spclock[0]);
	writer_thread1 = kthread_run(winsert, NULL, "winsert");
	writer_thread2 = kthread_run(winsert, NULL, "winsert");
	writer_thread3 = kthread_run(winsert, NULL, "winsert");
	winsert(NULL);
	ktime_get_real_ts64(&spclock[1]);
	res_time = (spclock[1].tv_sec - spclock[0].tv_sec) * BILLION;
	res_time += (spclock[1].tv_nsec - spclock[0].tv_nsec);
	printk("insert %lld ns\n", res_time);
	
	//search
	ktime_get_real_ts64(&spclock[0]);
	
	writer_thread1 = kthread_run(wsearch, NULL, "wsearch");
	writer_thread2 = kthread_run(wsearch, NULL, "wsearch");
	writer_thread3 = kthread_run(wsearch, NULL, "wsearch");
	wsearch(NULL);
	ktime_get_real_ts64(&spclock[1]);
	res_time = (spclock[1].tv_sec - spclock[0].tv_sec) * BILLION;
	res_time += (spclock[1].tv_nsec - spclock[0].tv_nsec);
	printk("search %lld ns\n", res_time);
	
	
	//delete
	ktime_get_real_ts64(&spclock[0]);
	writer_thread1 = kthread_run(wdelete, NULL, "wdelete");
	writer_thread2 = kthread_run(wdelete, NULL, "wdelete");
	writer_thread3 = kthread_run(wdelete, NULL, "wdelete");
	wdelete(NULL);
	ktime_get_real_ts64(&spclock[1]);
	res_time = (spclock[1].tv_sec - spclock[0].tv_sec) * BILLION;
	res_time += (spclock[1].tv_nsec - spclock[0].tv_nsec);
	printk("delete %lld ns\n", res_time);

}
