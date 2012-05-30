#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>

struct file_operations numlist_fops;

static unsigned int numlist_major = 128;
static unsigned int numlist_minor = 0;

enum numlist_data_t {
	NL_DATA_UNKNOWN = 0,
	NL_DATA_DOUBLE,
	NL_DATA_INTEGER,
	NL_DATA_CHAR
};

struct numlist_dev {
	unsigned long size;
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
static unsigned int numlist_dev_current = 0;
static struct numlist_device_node* devices;


struct numlist_dev* numlist_dev_create(enum numlist_data_t type)
{
	struct numlist_dev* dev = (struct numlist_dev*)kmalloc(sizeof(struct numlist_dev), GFP_KERNEL);
	dev->size = 0;
	switch(dev->type) {
	case NL_DATA_DOUBLE:
		dev->ddata = NULL;
		break;
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
	return dev;
}

void numlist_dev_destory(struct numlist_dev* dev)
{
	if (dev) {
		if (dev->size > 0) {
			switch(dev->type) {
			case NL_DATA_DOUBLE:
				kfree(dev->ddata);
				break;
			case NL_DATA_INTEGER:
				kfree(dev->idata);
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
