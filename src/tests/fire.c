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



static int fire(const char *path)
{
	MlDevice dev;
	
	if (!(dev = mlOpenDevice(path))) {
		fprintf(stderr, "Failed to open device '%s': %s (%d)\n", path, strerror(errno), errno);
		return 1;
	}
	
	printf("  Setting feedback...\n");
	if (mlSetUsingFeedback(dev, 1)) {
		fprintf(stderr, "Failed to set feedback '%s': %s (%d)\n", path, strerror(errno), errno);
		mlCloseDevice(dev);
		return 1;
	}
	
	printf("  Armed...\n");
	if (mlArm(dev)) {
		fprintf(stderr, "Failed to arm '%s': %s (%d)\n", path, strerror(errno), errno);
		mlCloseDevice(dev);
		return 1;
	}
	
	printf("  Ready in five seconds...\n");
	sleep(5);
	
	printf("  Fired...\n");
	if (mlFire(dev)) {
		fprintf(stderr, "Failed to fire '%s': %s (%d)\n", path, strerror(errno), errno);
		mlCloseDevice(dev);
		return 1;
	}
	
	mlCloseDevice(dev);
	
	return 0;
}


int main(int argc, char **argv)
{
	return fire("/dev/usb/hiddev0");
}
