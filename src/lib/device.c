#include "launcherp.h"



static MlDeviceType getDeviceTypeByFd(int fd)
{
	struct hiddev_devinfo info;
	
	if (ioctl(fd, HIDIOCGDEVINFO, &info)) {
		fprintf(stderr, "Failed to get info: %s (%d)\n", strerror(errno), errno);
		return ML_TYPE_UNKNOWN;
	}
	
#ifndef NDEBUG
	printf("Bustype %d\n", info.bustype);
	printf("Busnum  %d\n", info.busnum);
	printf("Devnum  %d\n", info.devnum);
	printf("Ifnum   %d\n", info.ifnum);
	printf("Vid     %x\n", info.vendor & 0xFFFF);
	printf("Pid     %x\n", info.product & 0xFFFF);
	printf("Version %d.%d\n", (info.version >> 8), info.version & 0xFF);
	printf("Numappl %d\n", info.num_applications);
	
	{
		int i;
		
		for (i = 0; i < info.num_applications; ++i) {
			int appl = ioctl(fd, HIDIOCAPPLICATION, i);
			
			printf("  Appl  %04Xh\n", (appl >> 16) & 0xFFFF);
		}
	}
#endif
	
	if ((info.vendor & 0xFFFF) == 0x1941 && (info.product & 0xFFFF) == 0x8021)
		return ML_TYPE_DREAMCHEEKY_MISSILE_LAUNCHER;
	
	return ML_TYPE_UNKNOWN;
}


MlDevice mlOpenDevice(const char *path)
{
	int fd = open(path, O_RDWR);
	MlDevice dev;
	MlDeviceType type;
	
	if (fd < 0)
		return NULL;
	
	type = getDeviceTypeByFd(fd);
	
	if (type == ML_TYPE_UNKNOWN) {
		errno = ENXIO;
		close(fd);
		return NULL;
	}
	
	dev = (MlDevice) calloc(1, sizeof(*dev));
	
	if (!dev) {
		close(fd);
		return NULL;
	}
	
	dev->fd = fd;
	dev->type = type;
	
	dev->shots = mlGetShotsCount(dev);
	
	/* Defaults for Dream Cheeky */
	dev->maxYawTime = 17.8;
	dev->maxPitchTime = 2.28;
	dev->armTime = 4.75;
	dev->fireTime = 0.650;
	
	return dev;
}


void mlCloseDevice(MlDevice dev)
{
	if (!dev) return;
	
	mlSetState(dev, MLC_NONE);
	
	close(dev->fd);
	free(dev);
}


MlDeviceType mlGetDeviceType(MlDevice dev)
{
	if (!dev) {
		errno = EINVAL;
		return ML_TYPE_UNKNOWN;
	}
	
	return dev->type;
}
