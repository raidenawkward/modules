#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

struct file_operations numlist_fops;

static unsigned int numlist_major = 1234;
static unsigned int numlist_minor = 1;
struct cdev numlist_cdev;

enum numlist_data_t {
	NL_DATA_UNKNOWN = 0,
	NL_DATA_INTEGER,
	NL_DATA_CHAR
};

static enum numlist_data_t current_type = NL_DATA_CHAR;

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
	size_t pos;
	enum numlist_data_t type;
	union {
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
static char num_spliter = ',';


static size_t itoa(int i, char** ret)
{
	size_t count = 0;
	int sign = i < 0 ? -1 : 0;
	int elem = i;

	while(elem) {
		++count;
		elem = elem / 10;
	}

	if (sign)
		++count;

	*ret = (char*)kmalloc(count * sizeof(char), GFP_KERNEL);
	if (!*ret)
		return -1;

	sprintf(*ret, "%d", i);

	return count;
}

static int atoi(const char* a, size_t size, int *ret)
{
	int i, n, base = 1;
	int sign = 0;
	*ret = 0;

	for (i = size - 1; i >= 0; --i) {
		n = a[i] - '0';
		if (a[i] == '-') {
			sign = 1;
			if (i != 0)
				return -1;
			continue;
		}
		*ret += n * base;
		base *= 10;
	}
	if (sign)
		*ret = -*ret;

	return 0;
}

struct numlist_dev* numlist_dev_create(enum numlist_data_t type)
{
	struct numlist_dev* dev = (struct numlist_dev*)kmalloc(sizeof(struct numlist_dev), GFP_KERNEL);
	dev->size = 0;
	dev->pos = 0;
	dev->type = type;

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

	if (numlist_dev_count <= 0) {
		struct numlist_dev *dev = numlist_dev_create(current_type);
		if (!dev)
			return -1;
		retval = numlist_node_add(dev);
		numlist_dev_current = devices;

		printk(KERN_NOTICE "new device(%s) created", get_data_type_str(dev->type));
	}

	return retval;
}

int numlist_release(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t numlist_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t retval = 0;
	struct numlist_dev *dev;
	int count_to_read;
	char* data;
	size_t size;

	if (!numlist_dev_current)
		return -1;

	dev = numlist_dev_current->dev;
	if (!dev)
		return -1;

	down_interruptible(&dev->sem);

	count_to_read = dev->pos + count >= dev->size? dev->size - dev->pos : count;

	switch (dev->type) {
	case NL_DATA_INTEGER:
		count_to_read = 1;
		size = itoa(dev->idata[dev->pos], &data);
		if (copy_to_user(buf, data, size)) {
			retval = -EFAULT;
			goto out;
		}
		break;
	case NL_DATA_CHAR:
		size = count_to_read;
		if (copy_to_user(buf, dev->cdata + dev->pos, size)) {
			retval = -EFAULT;
			goto out;
		}
		break;
	default:
		retval = 0;
		goto out;
	}

	*f_pos += count_to_read;
	dev->pos += count_to_read;
	retval = size;

out:
	up(&dev->sem);
	return retval;
}

static int is_char_a_num(char c)
{
	if (c >= '0' && c <= '9')
		return 1;

	if (c >= 'a' && c <= 'z')
		return 1;

	if (c >= 'A' && c <= 'Z')
		return 1;

	return 0;
}

static size_t get_numlist_size_from_str(const char *buf, size_t size)
{
	size_t i;

	if (!buf)
		return 0;

	for (i = 0; i < size; ++i) {
		if (!is_char_a_num(buf[i]))
			return i;
	}

	return 0;
}

ssize_t numlist_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t retval = 0;
	ssize_t count_to_write;
	struct numlist_dev *dev;
	int *new_idata = NULL;
	char *new_cdata = NULL;
	char *data = NULL;

	if (!numlist_dev_current)
		return -1;

	dev = numlist_dev_current->dev;
	if (!dev)
		return -1;

	down_interruptible(&dev->sem);

	switch (dev->type) {
	case NL_DATA_INTEGER:
		count_to_write = 1;
		if (dev->size == 0)
			new_idata = (int*)kmalloc(sizeof(int) * count_to_write, GFP_KERNEL);
		else
			new_idata = (int*)krealloc(dev->idata, sizeof(int) * (dev->size + count_to_write), GFP_KERNEL);
		if (!new_idata) {
			retval = -EFAULT;
			goto out;
		}
		dev->idata = new_idata;
		dev->size += count_to_write;

		data = (char*)kmalloc(sizeof(char) * count, GFP_KERNEL);
		if (!data) {
			retval = -EFAULT;
			goto out;
		}

		if (copy_from_user(data, buf, count)) {
			retval = -EFAULT;
			goto out;
		}
		dev->pos += count_to_write;

		if (atoi(data, count, dev->idata + dev->pos)) {
			retval = -EFAULT;
			goto out;
		}
		retval = count;

		break;
	case NL_DATA_CHAR:
		data = (char*)kmalloc(sizeof(char) * count, GFP_KERNEL);
		if (copy_from_user(data, buf, count)) {
			retval = -EFAULT;
			goto out;
		}

		count_to_write = get_numlist_size_from_str(data, count);
		if(dev->size == 0)
			new_cdata = (char*)kmalloc(sizeof(char) * count_to_write, GFP_KERNEL);
		else
			new_cdata = (char*)krealloc(dev->cdata, sizeof(char) * (dev->size + count_to_write), GFP_KERNEL);
		if (!new_cdata) {
			retval = -EFAULT;
			goto out;
		}
		dev->cdata = new_cdata;
		dev->size += count_to_write;

		strncpy(dev->cdata + dev->pos, data, count_to_write);
		dev->pos += count_to_write;

		retval = count_to_write;

		break;

	default:
		retval = 0;
		goto out;
	}

out:
	if (data)
		kfree(data);
	up(&dev->sem);
	return retval;
}

int numlist_read_procmem(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
	int retval = 0;
	int current_index = get_node_index(numlist_dev_current);
	struct numlist_device_node* node = devices;
	size_t index = 0, i;

	retval += sprintf(page + retval, "count:%d, current:%d\n\n", numlist_dev_count, current_index);

	while (node) {
		struct numlist_dev *dev = node->dev;
		if (!dev)
			continue;

		down_interruptible(&dev->sem);

		retval += sprintf(page + retval, "%d type: %s(%d) size: %d\n", index, get_data_type_str(dev->type), dev->type, dev->size);

		for (i = 0; i < dev->size; ++i) {
			switch (dev->type) {
			case NL_DATA_INTEGER:
				retval += sprintf(page + retval, "%d", dev->idata[i]);
				break;
			case NL_DATA_CHAR:
				retval += sprintf(page + retval, "%c", dev->cdata[i]);
				break;
			default:
				break;
			}
			if (i != dev->size - 1)
				retval += sprintf(page + retval, "%c", num_spliter);
		}

		retval += sprintf(page + retval, "\n");
		node = node->next;
		++index;
		up(&dev->sem);
	} // while

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
