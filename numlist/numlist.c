#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");

struct file_operations numlist_fops;

static unsigned int numlist_major = 128;
static unsigned int numlist_minor = 0;
struct cdev numlist_cdev;

enum numlist_data_t {
	NL_DATA_UNKNOWN = 0,
	NL_DATA_INTEGER,
	NL_DATA_CHAR
};

static char* get_data_type_str(enum numlist_data_t type)
{
	switch(type) {
	case NL_DATA_INTEGER:
		return "INTEGER";
	case NL_DATA_CHAR:
		return "CHAR";
	default:
		return "UNKNOWN";
	}
}

struct numlist_dev {
	size_t size;
	enum numlist_data_t type;
	union {
		float *ddata;
		int *idata;
		char *cdata;
	};
	struct semaphore sem;
};

struct numlist_device_node {
	struct numlist_dev *dev;
	struct numlist_device_node *next;
	struct numlist_device_node *prev;

};
static unsigned int numlist_dev_count = 0;
static struct numlist_device_node* numlist_dev_current = NULL;
static struct numlist_device_node* devices = NULL;


struct numlist_dev* numlist_dev_create(enum numlist_data_t type)
{
	struct numlist_dev* dev = (struct numlist_dev*)kmalloc(sizeof(struct numlist_dev), GFP_KERNEL);
	dev->size = 0;
	switch(dev->type) {
	case NL_DATA_INTEGER:
		dev->idata = NULL;
		break;
	case NL_DATA_CHAR:
		dev->cdata = NULL;
		break;
	case NL_DATA_UNKNOWN:
	default:
		break;
	}

	sema_init(&dev->sem, 1);

	return dev;
}

void numlist_dev_destory(struct numlist_dev* dev)
{
	if (dev) {
		if (dev->size > 0) {
			switch(dev->type) {
			case NL_DATA_INTEGER: kfree(dev->idata);
				break;
			case NL_DATA_CHAR:
				kfree(dev->cdata);
				break;
			case NL_DATA_UNKNOWN:
			default:
				break;
			}
		}
		kfree(dev);
	}
}

struct numlist_device_node* numlist_device_tail(void)
{
	struct numlist_device_node *retval = NULL;

	if (!devices || numlist_dev_count <= 0)
		return retval;

	retval = devices->next;
	while (retval->next) {
		retval = retval->next;
	}

	return retval;
}

int numlist_node_add(struct numlist_dev* dev)
{
	int retval = 0;
	struct numlist_device_node *newnode;

	if (!dev)
		return -1;

	newnode = (struct numlist_device_node*)kmalloc(sizeof(struct numlist_device_node), GFP_KERNEL);

	if (!newnode)
		return -1;

	newnode->next = NULL;
	newnode->dev = dev;

	if (numlist_dev_count <= 0) {
		devices = newnode;
		devices->prev = NULL;

	} else {
		struct numlist_device_node *tail = numlist_device_tail();
		if (!tail)
			return -1;
		tail->next = newnode;
		newnode->prev = tail;
	}

	++numlist_dev_count;
	return retval;
}

static struct numlist_device_node* get_node_by_dev(struct numlist_dev *dev)
{
	struct numlist_device_node *node;

	if (!dev)
		return NULL;

	node = devices;
	for (; node; node = node->next) {
		if (node->dev == dev)
			return node;
	}

	return NULL;
}

static int get_node_index(struct numlist_device_node* node)
{
	struct numlist_device_node* ptr = devices;
	int retval = 0;

	if (!node)
		return -1;

	while(ptr) {
		if (ptr == node)
			return retval;

		ptr = ptr->next;
		++retval;
	}

	return -1;
}

int numlist_node_remove(struct numlist_dev *dev_to_remove)
{
	struct numlist_device_node *node;

	if (!dev_to_remove || numlist_dev_count <= 0)
		return -1;

	node = get_node_by_dev(dev_to_remove);
	if (node->prev)
		node->prev->next = node->next;

	numlist_dev_destory(node->dev);
	kfree(node);
	--numlist_dev_count;

	return 0;
}

int numlist_open(struct inode *inode, struct file *filp)
{
	int retval = 0;

	return retval;
}

int numlist_release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t numlist_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t retval = 0;

	return retval;
}

ssize_t numlist_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t retval = 0;

	return retval;
}

int numlist_read_procmem(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int retval = 0;
	int current_index = get_node_index(numlist_dev_current);
	struct numlist_device_node* node = devices;
	size_t index = 0, i;

	retval += sprintf(buf, "count:%d, current:%d\n\n", numlist_dev_count, current_index);

	while (node) {
		struct numlist_dev *dev = node->dev;
		if (!dev)
			continue;

		retval += sprintf(buf, "%d type: %s\n", index, get_data_type_str(dev->type));

		for (i = 0; i < dev->size; ++i) {
			switch (dev->type) {
			case NL_DATA_INTEGER:
				retval += sprintf(buf, "%d", dev->idata[i]);
				break;
			case NL_DATA_CHAR:
				retval += sprintf(buf, "%c", dev->cdata[i]);
				break;
			default:
				break;
			}
			if (i != dev->size - 1 && i)
				retval += sprintf(buf, ",");
		}

		node = node->next;
		++index;
	}

	return retval;
}


struct file_operations numlist_fops = {
	.owner = THIS_MODULE,
	.read = numlist_read,
	.write = numlist_write,
	.open = numlist_open,
	.release = numlist_release,
};

static int __init numlist_init(void)
{
	int retval = 0;
	int devno = MKDEV(numlist_major, numlist_minor);

	cdev_init(&numlist_cdev, &numlist_fops);
	numlist_cdev.owner = THIS_MODULE;
	numlist_cdev.ops = &numlist_fops;
	retval = cdev_add(&numlist_cdev, devno, 1);

	if (retval)
		printk(KERN_NOTICE "device numlist init failed");
	else
		printk(KERN_NOTICE "device numlist init succeed");

	create_proc_read_entry("numlist",
						0, /* default mode */
						NULL, /* parent dir */
						numlist_read_procmem,
						NULL /* client data */);

	return retval;
}

static void __exit numlist_exit(void)
{
	cdev_del(&numlist_cdev);
	remove_proc_entry("numlist", NULL /* parent dir */);

	printk(KERN_NOTICE "device numlist exit");
}

module_init(numlist_init);
module_exit(numlist_exit);
