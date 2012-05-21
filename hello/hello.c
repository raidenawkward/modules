#include <linux/module.h>//needed by every module
#include <linux/init.h>//init&exit
MODULE_LICENSE("GPL"); 

static howmany = 1;
static char* whom = "world";

module_param(howmany, int, S_IRUGO);
module_param(whom, charp, S_IRUGO);

void print()
{
	int i;
	for (i = 0; i < howmany; ++i) {
		printk("I am the %s\n", whom);
	}

}

static int __init hello_init (void)
{
	printk("Hello module init\n");

	print();

	return 0;
}
static void __exit hello_exit (void)
{
	printk("Hello module exit\n");
}

module_init(hello_init);
module_exit(hello_exit);
