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
	DATA_UNKNOWN = 0,
	DATA_DOUBLE,
	DATA_INTEGER,
	DATA_CHAR
};

struct numlist_dev {
	unsigned int length;
	numlist_data_t type;
	union {
		float *ddata;
		int *idata;
		char *cdata;
	};
};
