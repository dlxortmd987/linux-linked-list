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

#define COUNT 3000 // node 100 개

unsigned long long res_time;

struct task_struct* writer_thread1, * writer_thread2, * writer_thread3, * writer_thread4;
// struct tast_struct* writer_thread[4];

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
	
	// writer_thread1 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
	// writer_thread2 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
	// writer_thread3 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
	// writer_thread4 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
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

	/*먼저 100개 노드를 insert해 둔다. */
	for (i = 0; i < COUNT; i++){
		struct my_node *new = kmalloc(sizeof(struct my_node),GFP_KERNEL);
		new->data = i;
		wlist_add(&new->whead, &my_list);
	}	
	return 0;
}

int wsearch(void* data)
{
	// search 
	int i;
	for (i = 0; i < COUNT; i++){
		list_for_each_entry_safe(current_node,tmp, &(my_list.head), head ){
			cnode = container_of(current_node, struct my_node, whead);
			if(cnode->data == i){
				//printk("current node->data: %d \n", current_node->data);
				struct my_node *new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
				new->data = i*1000;
				wlist_add(&new->whead, &(cnode->whead));
			}
		}
		
	}	
	ssleep(1);
	// 잘 삽입되었는지 확인
	printk("================result=================\n");
	list_for_each_entry_safe(current_node, tmp, &(my_list.head), head){
		cnode = container_of(current_node, struct my_node, whead);
		// printk("current node-> data : %d \n", current_node->data);
	
	}
	return 0;
}

int wdelete(void* data){
	

	list_for_each_entry_safe(current_node, tmp, &(my_list.head), head){
		cnode = container_of(current_node, struct my_node, whead);
		if(cnode->data == 2){
			// printk("current node value :%d \n", current_node->data);
			list_del(&current_node->head);
			kfree(current_node);
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

/*
	int i;
	for(i = 0; i < 4; i++) {
		spin_lock(&counter_lock);
		res_time = 0;
		ktime_get_real_ts64(&spclock[0]);
		writer_thread[i] = kthread_run(winsert, NULL, "winsert");
		ktime_get_real_ts64(&spclock[1]);
		res_time = (spclock[1].tv_sec - spclock[0].tv_sec) * BILLION;
		res_time += (spclock[1].tv_nsec - spclock[0].tv_nsec);
		printk("%lld ns\n", res_time);

		ktime_get_real_ts64(&spclock[0]);
		writer_thread[i] = kthread_run(wsearch, NULL, "wsearch");
		ktime_get_real_ts64(&spclock[1]);
		res_time = (spclock[1].tv_sec - spclock[0].tv_sec) * BILLION;
		res_time += (spclock[1].tv_nsec - spclock[0].tv_nsec);
		printk("%lld ns\n", res_time);

		ktime_get_real_ts64(&spclock[0]);
		writer_thread[i] = kthread_run(wdelete, NULL, "wdelete");
		ktime_get_real_ts64(&spclock[1]);
		res_time = (spclock[1].tv_sec - spclock[0].tv_sec) * BILLION;
		res_time += (spclock[1].tv_nsec - spclock[0].tv_nsec);
		printk("%lld ns\n", res_time);
	}
	*/

}

unsigned long long calclock3(struct timespec64 *spclock, unsigned long long *total_time){

	long temp, temp_n;
	unsigned long long timedelay=0;

	if(spclock[1].tv_nsec >= spclock[0].tv_nsec){

		temp = spclock[1].tv_sec - spclock[0].tv_sec;
		temp_n = spclock[1].tv_nsec - spclock[0].tv_nsec;
		timedelay = BILLION * temp +temp_n;
	} else{

		temp = spclock[1].tv_sec - spclock[0].tv_sec -1;
		temp_n = BILLION + spclock[1].tv_nsec - spclock[0].tv_nsec;
		timedelay = BILLION * temp + temp_n;
	}

	__sync_fetch_and_add(total_time, timedelay);
	return timedelay;
}
