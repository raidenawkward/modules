#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>

#include "schar.h"

static char* schar_name = NULL;
/* forward declaration for _fops */
static ssize_t schar_read(struct file *file, char *buf, size_t count, loff_t *offset);
static ssize_t schar_write(struct file *file, const char *buf, size_t count, loff_t *offset);
static unsigned int schar_poll(struct file *file, poll_table *wait);
static long schar_ioctl(/* struct inode *inode, */struct file *file, unsigned int cmd, unsigned long arg);
static int schar_mmap(struct file *file, struct vm_area_struct *vma);
static int schar_open(struct inode *inode, struct file *file);
static int schar_release(struct inode *inode, struct file *file);

// kernel 3.2.6+ based
#if 0 // GNU
static struct file_operations schar_fops = {
	THIS_MODULE,
	NULL,// llseek
	schar_read,
	schar_write,
	NULL,// aio_read
	NULL,// aio_write
	NULL,// readdir
	schar_poll,
	NULL,// unlocked_ioctl
	schar_ioctl,
	schar_mmap,
	schar_open,
	NULL,// flush
	schar_release,
	NULL,// fsync
	NULL,// aio_fsync
	NULL,// fasync
	NULL,// lock
	NULL,// sendpage
};
#else // C99
struct file_operations schar_fops = {
.owner = THIS_MODULE,
.read = schar_read,
.write = schar_write,
.poll = schar_poll,
.compat_ioctl = schar_ioctl,
.mmap = schar_mmap,
.open = schar_open,
.release = schar_release,
};
#endif

ssize_t schar_read(struct file *file, char *buf, size_t count, loff_t *offset) {
	return 0;
}

ssize_t schar_write(struct file *file, const char *buf, size_t count, loff_t *offset) {
	return 0;
}

unsigned int schar_poll(struct file *file, poll_table *wait) {

	return 0;
}

long schar_ioctl(/* struct inode *inode, */struct file *file, unsigned int cmd, unsigned long arg) {

	return 0;
}

int schar_mmap(struct file *file, struct vm_area_struct *vma) {

	return 0;
}

int schar_open(struct inode *inode, struct file *file) {
	/* increment usage count */
    //MOD_INC_USE_COUNT; // for 2.4
	try_module_get(schar_fops.owner); // for 2.6, and module_put(owner)

	MSG("major: %d minor: %d\n", MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

int schar_release(struct inode *inode, struct file *file) {

	return 0;
}

int init_module(void) {
	int res;

	if (schar_name == NULL)
		schar_name = "schar";

	// register device with kernel
	res = register_chrdev(SCHAR_MAJOR, schar_name, &schar_fops);
	if (res) {
		MSG("can't register device with kernel\n");
	}

	return res;
}
