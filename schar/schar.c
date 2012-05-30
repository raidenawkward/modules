#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/semaphore.h>

MODULE_LICENSE("GPL");

#include "schar.h"

static char* schar_name = NULL;

#define SCHAR_BUF_SIZE (1024)
static char schar_buf[SCHAR_BUF_SIZE];
static int buf_cur = 0;

#define SCHAR_CONCURRENT_ON 0
static struct semaphore sem;

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
	ssize_t res;

#if SCHAR_CONCURRENT_ON
	if (down_interruptible(&sem)) {
		return -ERESTARTSYS;
	}
#endif

	res = count < SCHAR_BUF_SIZE? count : SCHAR_BUF_SIZE;

	if (copy_to_user(buf, schar_buf, res)) {
		buf_cur = 0;
		up(&sem);
		return -EFAULT;
	}

	MSG("reading: %s",schar_buf);

#if SCHAR_CONCURRENT_ON
	up(&sem);
#endif
	return res;
}

ssize_t schar_write(struct file *file, const char *buf, size_t count, loff_t *offset) {

	ssize_t res;

#if SCHAR_CONCURRENT_ON
	if (down_interruptible(&sem)) {
		return -ERESTARTSYS;
	}
#endif

	res = count < SCHAR_BUF_SIZE? count : SCHAR_BUF_SIZE;

	if (copy_from_user(schar_buf, buf, res)) {
		up(&sem);
		return -EFAULT;
	}

	MSG("writting: %s",schar_buf);

#if SCHAR_CONCURRENT_ON
	up(&sem);
#endif
	return res;
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
#if ! SCHAR_CONCURRENT_ON
	if (down_interruptible(&sem)) {
		return -ERESTARTSYS;
	}
#endif
	/* increment usage count */
    //MOD_INC_USE_COUNT; // for 2.4
	try_module_get(schar_fops.owner); // for 2.6, and module_put(owner)

	MSG("openned: major: %d minor: %d\n", MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

int schar_release(struct inode *inode, struct file *file) {
#if ! SCHAR_CONCURRENT_ON
	up(&sem);
#endif
	module_put(schar_fops.owner);
	MSG("released: major: %d minor: %d\n", MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static int __init schar_init(void) {
	int res;

	if (schar_name == NULL)
		schar_name = "schar";

	// register device with kernel
	res = register_chrdev(SCHAR_MAJOR, schar_name, &schar_fops);
	if (res) {
		MSG("can't register device with kernel\n");
		return res;
	} else {
		MSG("schar inited\n");
	}

	sema_init(&sem,1);

	memset(schar_buf,0x00,SCHAR_BUF_SIZE);
	buf_cur = 0;

	return res;
}

static void __exit schar_exit(void) {
	unregister_chrdev(SCHAR_MAJOR, schar_name);

	memset(schar_buf,0x00,SCHAR_BUF_SIZE);
	buf_cur = 0;
	MSG("schar exited\n");
}

module_init(schar_init);
module_exit(schar_exit);
