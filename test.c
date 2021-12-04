// test code
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h> // gclist.h
#include <linux/slab.h>
#include <linux/ktime.h>

#define COUNT 10 // node 100 개
void test_insert_and_search(void);
void test_delete(void);

struct my_node {
	struct list_head list;
	int data;
};

struct list_head my_list; //head
	struct my_node *current_node = NULL;
	struct my_node *tmp = NULL;

int __init mod_init(void){
	test_insert_and_search();
	test_delete();
	printk("hello module! \n");
	return 0;
}

void __exit mod_cleanup(void) {
	printk("bye module \n");
}

module_init(mod_init);
module_exit(mod_cleanup);
MODULE_LICENSE("GPL");

void test_insert_and_search(void)
{
	
	int i;

	INIT_LIST_HEAD(&my_list); // initialize list head
	/*먼저 100개 노드를 insert해 둔다. */
	
	for (i = 0; i < COUNT; i++){
		struct my_node *new = kmalloc(sizeof(struct my_node),GFP_KERNEL);
		new->data = i;
		list_add(&new->list, &my_list);
	}	

	// search 
	
	i = 0;
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
	// 잘 삽입되었는지 확인
	printk("================result=================\n");
	list_for_each_entry_safe(current_node, tmp, &my_list, list){
		printk("current node-> data : %d \n", current_node->data);
	
	}
}

void test_delete(void){
	
	list_for_each_entry_safe(current_node, tmp, &my_list, list){
		if (current_node->data == 2){
			printk("current node value :%d \n", current_node->data);
			list_del(&current_node->list);
			kfree(current_node);
		}
	}
	
}
