// test code - original linked list of linux kernel
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/ktime.h>
#include <linux/kthread.h>
#include <linux/delay.h>


#define COUNT 10 // node 100 개
int test_insert_and_search(void* data);
int test_delete(void* data);

unsigned long long start_insert;
unsigned long long end_insert;
unsigned long long start_search;
unsigned long long end_search;
unsigned long long start_delete;
unsigned long long end_delete;

struct task_struct* writer_thread1, * writer_thread2, * writer_thread3, * writer_thread4;

struct my_node {
	struct list_head list;
	int data;
	 
};

struct list_head my_list; //head
	struct my_node *current_node = NULL;
	struct my_node *tmp = NULL;

int __init mod_init(void){
	INIT_LIST_HEAD(&my_list); // initialize list head
		int i;

	/*먼저 100개 노드를 insert해 둔다. */
	
	for (i = 0; i < COUNT; i++){
		struct my_node *new = kmalloc(sizeof(struct my_node),GFP_KERNEL);
		new->data = i;
		list_add(&new->list, &my_list);
	}	

	// writer_thread1 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
	// writer_thread2 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
	// writer_thread3 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
	// writer_thread4 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");

	writer_thread1 = kthread_run(test_delete, NULL, "test_delete");
	writer_thread2 = kthread_run(test_delete, NULL, "test_delete");
	writer_thread3 = kthread_run(test_delete, NULL, "test_delete");
	printk("hello module! \n");
	return 0;
}

void __exit mod_cleanup(void) {
	kthread_stop(writer_thread1);
	kthread_stop(writer_thread2);
	kthread_stop(writer_thread3);
	kthread_stop(writer_thread4);
	printk("bye module \n");
}

module_init(mod_init);
module_exit(mod_cleanup);
MODULE_LICENSE("GPL");

int test_insert_and_search(void* data)
{
	

	// search 
	start_search = ktime_get();

	int i = 0;
	for (i = 0; i < COUNT; i++){
		list_for_each_entry_safe(current_node,tmp, &my_list, list ){
			if(current_node->data == i){
				
				struct my_node *new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
				new->data = i*1000;
				list_add(&new->list, &(current_node->list));
			}
		}
	}	

	end_search = ktime_get();

	printk("search : %lld \n", end_search-start_search);
	// 잘 삽입되었는지 처음부터 확인
	printk("================result=================\n");
	list_for_each_entry_safe(current_node, tmp, &my_list, list){
		printk("current node-> data : %d \n", current_node->data);
	}
	return 0;
}

int test_delete(void* data){

	start_delete = ktime_get();
	list_for_each_entry_safe(current_node, tmp, &my_list, list){
		if (current_node->data == 2){
			printk("delete node %d \n", current_node->data);
			list_del(&current_node->list);
			kfree(current_node);
		}
	}
	end_delete = ktime_get();

	printk("delete : %lld \n",end_delete-start_delete);
	return 0;
}
