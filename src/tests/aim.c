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



static int aim(const char *path)
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
	
	printf("  Aiming (0, 0)...\n");
	if (mlAim(dev, 0, 0)) {
		fprintf(stderr, "Failed to aim '%s': %s (%d)\n", path, strerror(errno), errno);
		mlCloseDevice(dev);
		return 1;
	}
	
	mlCloseDevice(dev);
	
	return 0;
}


int main(int argc, char **argv)
{
	return aim("/dev/usb/hiddev0");
}
