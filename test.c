// test code
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h> // gclist.h
#include <linux/slab.h>
#include <linux/ktime.h>

#define COUNT 100 // node 100 개
void test_insert_and_search(void)
void test_delete(void)

struct node {
	struct list_head entry;
	int data;
};

int __init mod_init(void){

	printk("hello module! \n");
	return 0;
}

int __exit mod_cleanup(void) {
	printk("bye module \n");
}

module_init(mod_init);
module_exit(mod_cleanup);
MODULE_LICENSE("GPL");

void test_insert_and_search(void)
{
	struct list_head my_list; //head
	struct my_node *current_node = NULL;
	struct my_node *tmp = NULL;

	INIT_LIST_HEAD(&my_list); // initialize list head
	/*먼저 100개 노드를 insert해 둔다. */
	
	for (int i = 0; i < COUNT; i++){
		struct my_node *new = kmalloc(sizeof(struct my_node),GFP_KERNEL);
		new->data = i;
		list_add(&new->list, &my_list);
	}	

	// search 
	
	list_for_each_entry_safe(current_node,tmp, &my_list, entry ){
		printk("current node->data: %d \n", current_node->data);
		struct my_node *new = kmalloc(sizeof(struct my_node), GFP_KERNEL);
		new->data = i*2;
		list_add(&new->list, &my_list);
	}

	// 잘 삽입되었는지 확인
	printk("================result=================\n")
	list_for_each_entry_safe(current_node, tmp, &my_list, entry){
		printk("current node-> data : %d \n", current_node->data);
	
	}

}

void test_delete(void){
	list_for_each_entry_safe(current_node, tmp, &my_list, entry){
		if (current_node->data == 2){
			printk("current node value :%d \n", current_node->data);
			list_del(&current_node->list);
			kfree(current_node);
		}
	}
	
}
