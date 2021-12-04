// test code
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h> // gclist.h
#include <linux/slab.h>
#include <linux/ktime.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "gclist.h"


#define COUNT 10 // node 100 개
int test_insert_and_search(void* data);
void test_delete(void);

struct task_struct* writer_thread1, * writer_thread2, * writer_thread3, * writer_thread4;

struct my_node {
	struct gclist_head list;
	int data;
};

struct gclist_head my_list; //head
	struct my_node *current_node = NULL;
	struct my_node *tmp = NULL;

int __init mod_init(void){
	INIT_GCLIST_HEAD(&my_list); // initialize list head
	int i;

	/*먼저 100개 노드를 insert해 둔다. */
	
	for (i = 0; i < COUNT; i++){
		struct my_node *new = kmalloc(sizeof(struct my_node),GFP_KERNEL);
		new->data = i;
		gclist_add(&new->list, &my_list);
	}	
	writer_thread1 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
	writer_thread2 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
	writer_thread3 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
	writer_thread4 = kthread_run(test_insert_and_search, NULL, "test_insert_and_search");
	printk("hello module! \n");
	return 0;
}

void __exit mod_cleanup(void) {
	printk("bye module \n");
}

module_init(mod_init);
module_exit(mod_cleanup);
MODULE_LICENSE("GPL");

int test_insert_and_search(void* data)
{
	

	// search 
	
	int i;
	for (i = 0; i < COUNT; i++){
		gclist_for_each_entry_safe(current_node,tmp, &my_list, list ){
			if(current_node->data == i){
				//printk("current node->data: %d \n", current_node->data);
				struct my_node *new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
				new->data = i*1000;
				gclist_add(&new->list, &(current_node->list));
			}
		}
	}	
	ssleep(1);
	// 잘 삽입되었는지 확인
	printk("================result=================\n");
	gclist_for_each_entry_safe(current_node, tmp, &my_list, list){
		printk("current node-> data : %d \n", current_node->data);
	
	}
	return 0;
}

void test_delete(void){
	
	gclist_for_each_entry_safe(current_node, tmp, &my_list, list){
		if (current_node->data == 2){
			printk("current node value :%d \n", current_node->data);
			gclist_del(&current_node->list);
			kfree(current_node);
		}
	}
	
}
