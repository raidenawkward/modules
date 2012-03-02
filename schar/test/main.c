#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#define SCHAR_DEVICE "/dev/schar"
char msg_buf[1024];

void write_msg_to_dev(int dev, const char* msg) {
	if (dev <= 0 || !msg)
		return;

	int i = write(dev,msg,sizeof(msg));
	printf("%d bytes wrote\n",i);
	perror("");
}

int main() {
	int devp = open(SCHAR_DEVICE, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
	if (devp <= 0) {
		printf("could not open dev\n");
		return 0;
	}

	memset(msg_buf,'\0',sizeof(msg_buf));

	int pid = fork();
	if (pid > 0) {
		int i;
		for (i = 0; i < sizeof(msg_buf); ++i) {
			msg_buf[i] = 'a';
		}
		printf("sending %s\n",msg_buf);
		write_msg_to_dev(devp,msg_buf);
		sleep(1);
		close(devp);
	} else if (pid == 0) {
		int i;
		for (i = 0; i < sizeof(msg_buf); ++i) {
			msg_buf[i] = 'b';
		}
		printf("sending %s\n",msg_buf);
		write_msg_to_dev(devp,msg_buf);
		sleep(1);
		close(devp);
	} else {

	}

	return 0;
}
