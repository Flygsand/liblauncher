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



static int control(const char *path)
{
	MlDevice dev;
	unsigned int status;
	
	if (!(dev = mlOpenDevice(path))) {
		fprintf(stderr, "Failed to open device '%s': %s (%d)\n", path, strerror(errno), errno);
		return 1;
	}
	
#if 1
	printf("  Moving to lower left...\n");
	do {
		usleep(10000);
		
		if ((status = mlGetStatus(dev)) == MLS_UNKNOWN) {
			fprintf(stderr, "Failed to read status on '%s': %s (%d)\n", path, strerror(errno), errno);
			mlCloseDevice(dev);
			return 1;
		}
		
		if (mlSetState(dev, (status & MLS_LEFTMOST ? 0 : MLC_LEFT) | (status & MLS_LOWEST ? 0 : MLC_DOWN))) {
			fprintf(stderr, "Failed to set state on '%s': %s (%d)\n", path, strerror(errno), errno);
			mlCloseDevice(dev);
			return 1;
		}
	} while ((status & (MLS_LEFTMOST | MLS_LOWEST)) != (MLS_LEFTMOST | MLS_LOWEST));
#endif
	
#if 1
	printf("  Moving to upper right...\n");
	do {
		usleep(10000);
		
		if ((status = mlGetStatus(dev)) == MLS_UNKNOWN) {
			fprintf(stderr, "Failed to read status on '%s': %s (%d)\n", path, strerror(errno), errno);
			mlCloseDevice(dev);
			return 1;
		}
		
		if (mlSetState(dev, (status & MLS_RIGHTMOST ? 0 : MLC_RIGHT) | (status & MLS_HIGHEST ? 0 : MLC_UP))) {
			fprintf(stderr, "Failed to set state on '%s': %s (%d)\n", path, strerror(errno), errno);
			mlCloseDevice(dev);
			return 1;
		}
	} while ((status & (MLS_RIGHTMOST | MLS_HIGHEST)) != (MLS_RIGHTMOST | MLS_HIGHEST));
#endif
	
#if 1
	printf("  Arming...\n");
	if (mlSetState(dev, MLC_FIRE)) {
		fprintf(stderr, "Failed to set state on '%s': %s (%d)\n", path, strerror(errno), errno);
		mlCloseDevice(dev);
		return 1;
	}
	
	do {
		usleep(10000);
		
		if ((status = mlGetStatus(dev)) == MLS_UNKNOWN) {
			fprintf(stderr, "Failed to read status on '%s': %s (%d)\n", path, strerror(errno), errno);
			mlCloseDevice(dev);
			return 1;
		}
	} while ((status & MLS_ARMED) != MLS_ARMED);
	
	printf("  Firing...\n");
	do {
		usleep(10000);
		
		if ((status = mlGetStatus(dev)) == MLS_UNKNOWN) {
			fprintf(stderr, "Failed to read status on '%s': %s (%d)\n", path, strerror(errno), errno);
			mlCloseDevice(dev);
			return 1;
		}
	} while ((status & MLS_ARMED) == MLS_ARMED);
	
	if (mlSetState(dev, MLC_NONE)) {
		fprintf(stderr, "Failed to set state on '%s': %s (%d)\n", path, strerror(errno), errno);
		mlCloseDevice(dev);
		return 1;
	}
#endif

	mlCloseDevice(dev);
	
	return 0;
}


int main(int argc, char **argv)
{
	return control("/dev/usb/hiddev0");
}
