#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "launcher.h"


/* --- Types --- */
struct _MlDeviceEnum {
	DIR *dir;
};



MlDeviceEnum mlEnumerateDevices(void)
{
	DIR *dir;
	MlDeviceEnum devEnum;
	
	dir = opendir("/dev/usb");
	if (!dir) return NULL;
	
	devEnum = (MlDeviceEnum) calloc(1, sizeof(*devEnum));
	if (!devEnum) {
		closedir(dir);
		return NULL;
	}
	
	devEnum->dir = dir;
	
	return devEnum;
}


void mlCloseDeviceEnum(MlDeviceEnum e)
{
	if (!e) return;
	
	closedir(e->dir);
	free(e);
}


int mlGetNextDevice(MlDeviceEnum e, MlDeviceDescr *descr, size_t size)
{
	struct dirent *de;
	
	if (!e || !descr || !size) {
		errno = EINVAL;
		return -1;
	}
	
	do {
		de = readdir(e->dir);
		
		if (!de) return -1;
	} while (de->d_type != DT_CHR);
	
	descr->type = ML_TYPE_UNKNOWN;
	descr->path = (char*) (descr + 1);
	strncpy(descr->path, "/dev/usb/", size - sizeof(*descr));
	strncat(descr->path, de->d_name, size - sizeof(*descr));
	descr->path[size - sizeof(*descr) - 1] = 0;
	
	return 0;
}
