// test code
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h> // gclist.h
#include <linux/slab.h>
#include <linux/ktime.h>

#define COUNT 100 // node 100 개

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
