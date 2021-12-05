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

#define BILLION 1000000000 

spinlock_t counter_lock;
struct timespec64 spclock[2];

#define COUNT 10 // node 100 개

unsigned long long res_time;

// struct task_struct* writer_thread1, * writer_thread2, * writer_thread3, * writer_thread4;
typedef struct tast_struct* writer_thread[4];

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
		list_add(&new->list, &my_list);
	}	
}

int wsearch(void* data)
{
	// search 
	int i;
	for (i = 0; i < COUNT; i++){
		list_for_each_entry_safe(current_node,tmp, &my_list, list ){
			if(current_node->data == i){
				//printk("current node->data: %d \n", current_node->data);
				struct my_node *new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
				new->data = i*1000;
				list_add(&new->list, &(current_node->list));
			}
		}
	}	
	ssleep(1);
	// 잘 삽입되었는지 확인
	printk("================result=================\n");
	list_for_each_entry_safe(current_node, tmp, &my_list, list){
		printk("current node-> data : %d \n", current_node->data);
	
	}
	return 0;
}

int wdelete(void* data){
	
	list_for_each_entry_safe(current_node, tmp, &my_list, list){
		if (current_node->data == 2){
			printk("current node value :%d \n", current_node->data);
			list_del(&current_node->list);
			kfree(current_node);
		}
	}
	
	return 0;
}

void test(void) 
{
	res_time = 0;
	spin_lock(&counter_lock);
	

	int i;
	for(i = 0; i < 4; i++) {
		res_time = 0;
		ktime_get_real_ts64(&spclock[0]);
		writer_thread[i] = kthread_run(winsert, NULL, "test_insert_and_search");
		ktime_get_real_ts64(&spclock[1]);
		res_time = (spclock[1].tv_sec - spclock[0].tv_sec) * BILLION;
		res_time += (spclock[1].tv_nsec - spclock[0].tv_nsec);
		printk("%lld ns\n", res_time);

		ktime_get_real_ts64(&spclock[0]);
		writer_thread[i] = kthread_run(wsearch, NULL, "test_insert_and_search");
		ktime_get_real_ts64(&spclock[1]);
		res_time = (spclock[1].tv_sec - spclock[0].tv_sec) * BILLION;
		res_time += (spclock[1].tv_nsec - spclock[0].tv_nsec);
		printk("%lld ns\n", res_time);

		ktime_get_real_ts64(&spclock[0]);
		writer_thread[i] = kthread_run(wdelete, NULL, "test_insert_and_search");
		ktime_get_real_ts64(&spclock[1]);
		res_time = (spclock[1].tv_sec - spclock[0].tv_sec) * BILLION;
		res_time += (spclock[1].tv_nsec - spclock[0].tv_nsec);
		printk("%lld ns\n", res_time);
	}

	spin_unlock(&counter_lock);
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