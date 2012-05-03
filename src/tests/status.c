#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/time.h>
#include "launcher.h"



static int status(const char *path)
{
	MlDevice dev;
	unsigned int status;
	
	if (!(dev = mlOpenDevice(path))) {
		fprintf(stderr, "Failed to open device '%s': %s (%d)\n", path, strerror(errno), errno);
		return 1;
	}
	
	printf("Type: %d\n", mlGetDeviceType(dev));
	
	if ((status = mlGetStatus(dev)) == MLS_UNKNOWN) {
		fprintf(stderr, "Failed to read status on '%s': %s (%d)\n", path, strerror(errno), errno);
		mlCloseDevice(dev);
		return 1;
	}
	
	printf("Status:");
	
	if (status & MLS_LEFTMOST)	printf(" leftmost");
	if (status & MLS_RIGHTMOST)	printf(" rightmost");
	if (status & MLS_LOWEST)	printf(" lowest");
	if (status & MLS_HIGHEST)	printf(" highest");
	if (status & MLS_ARMED)		printf(" armed");
	
	printf("\n");
	
	mlCloseDevice(dev);
	
	return 0;
}


int main(int argc, char **argv)
{
	return status("/dev/usb/hiddev0");
}
