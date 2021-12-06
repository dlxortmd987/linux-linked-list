/*** test code of kernel linked list with spinlock ***
lock the list before job, and unlock after job.
so more than one thread cannot access the list. */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h> // list.h
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define BILLION 1000000000 

spinlock_t counter_lock;
struct timespec64 spclock[2];

#define COUNT 120 // the number of nodes

unsigned long long res_time;
int cnt = 1;

struct task_struct* writer_thread1, * writer_thread2, * writer_thread3;

struct my_node {
	struct list_head list;
	int data;
};

struct list_head my_list; //head
struct my_node *current_node = NULL;
struct my_node *tmp = NULL;

void test(void);
unsigned long long calclock3(struct timespec64 *spclock, unsigned long long *total_time);


int __init mod_init(void){
	spin_lock_init(&counter_lock);
	INIT_LIST_HEAD(&my_list); // initialize list head
	printk("=========original kernel data structure with spin lock=========\n");
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


int winsert(void* data) // insert nodes to list
{
	spin_lock(&counter_lock);

	int i;

	for (i = 0; i < COUNT; i++){
		struct my_node *new = kmalloc(sizeof(struct my_node),GFP_KERNEL);
		new->data = i;
		list_add(&new->list, &my_list);
		msleep(1);
	}	
	spin_unlock(&counter_lock);

	return 0;
}

int wsearch(void* data) // search & insert nodes to list (***NOT ONLY SEARCH***)
{
	spin_lock(&counter_lock);

	int i;
	for (i = 0; i < COUNT; i++){
		list_for_each_entry_safe(current_node,tmp, &my_list, list ){
			if(current_node->data == i){
				struct my_node *new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
				new->data = i+1000;
				list_add(&new->list, &(current_node->list));
			}
		}
		msleep(1);
	}	
	spin_unlock(&counter_lock);

	return 0;
}

int wdelete(void* data){ // delete 1 node
	spin_lock(&counter_lock);
	list_for_each_entry_safe(current_node, tmp, &my_list, list){
		if (current_node->data == cnt++){
			list_del(&current_node->list);
			kfree(current_node);
		}
	}
	spin_unlock(&counter_lock);
	
	return 0;
}

void test(void) // distributes jobs to 3 threads + 1 main process = 4 threads work
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
	kthread_stop(writer_thread1);
	kthread_stop(writer_thread2);
	kthread_stop(writer_thread3);
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
	kthread_stop(writer_thread1);
	kthread_stop(writer_thread2);
	kthread_stop(writer_thread3);
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
	kthread_stop(writer_thread1);
	kthread_stop(writer_thread2);
	kthread_stop(writer_thread3);
	ktime_get_real_ts64(&spclock[1]);
	res_time = (spclock[1].tv_sec - spclock[0].tv_sec) * BILLION;
	res_time += (spclock[1].tv_nsec - spclock[0].tv_nsec);
	printk("delete %lld ns\n", res_time);

}
