#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/string.h>


struct struct_with_work {
	struct work_struct work;
	char data[64];
};

static struct workqueue_struct *queue = NULL;
static struct struct_with_work works[4];


static void work_handler(struct work_struct *data)
{
	int i;
	struct struct_with_work *work= container_of(data, struct struct_with_work, work);

	for (i = 0; i < 10; ++i) {
		printk(KERN_ALERT "work %s: %d", work->data, i);
		mdelay(10);
	}
}

static int __init test_workqueue_init(void)
{
	int i;

	queue = create_singlethread_workqueue("testworkqueue");

	if (!queue)
		goto err;

	for (i = 0; i < ARRAY_SIZE(works); ++i) {
		memset(works[i].data, '\0', sizeof(works[i].data));
		sprintf(works[i].data, "%d", i);

		INIT_WORK(&(works[i].work), work_handler);
		queue_work(queue, &(works[i].work));
	}

	return 0;

err:
	return -1;
}

static void __exit test_workqueue_exit(void)
{
	if (queue)
		destroy_workqueue(queue);
}

MODULE_LICENSE("GPL");
module_init(test_workqueue_init);
module_exit(test_workqueue_exit);
