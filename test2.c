// test code
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h> // gclist.h
#include <linux/slab.h>
#include <linux/ktime.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "wlist.h"


#define COUNT 1000 // node 100 개
int test_insert_and_search(void* data);
void test_delete(void);

struct task_struct* writer_thread1, * writer_thread2, * writer_thread3, * writer_thread4;

struct my_node {
	struct wlist_head whead;
	int data;
};

struct wlist_head my_list; //head
struct my_node * cnode = NULL;
struct wlist_head *current_node = NULL;
struct wlist_head *tmp = NULL;

int __init mod_init(void){
	INIT_WLIST_HEAD(&my_list); // initialize list head
	int i;

	/*먼저 100개 노드를 insert해 둔다. */
	
	for (i = 0; i < COUNT; i++){
		struct my_node *new = kmalloc(sizeof(struct my_node),GFP_KERNEL);
		new->data = i;
		wlist_add(&new->whead, &my_list);
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
		list_for_each_entry_safe(current_node,tmp, &(my_list.head), head ){
			cnode = container_of(current_node, struct my_node, whead);
			if(cnode->data == i){
				printk("current node->data: %d \n", cnode->data);
				struct my_node *new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
				new->data = i+1000;
				wlist_add(&new->whead, &(cnode->whead));
			}
		}
	}	
	ssleep(1);
	// 잘 삽입되었는지 확인
	
	list_for_each_entry_safe(current_node, tmp, &(my_list.head), head){
		cnode = container_of(current_node, struct my_node, whead);
		printk("current node->data: %d \n", cnode->data);
	}
	return 0;
}

