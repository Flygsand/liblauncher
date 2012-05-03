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



int main(int argc, char **argv)
{
	MlDeviceEnum devEnum;
	MlDeviceDescr *descr;
	unsigned char buf[sizeof(*descr) + MAXPATHLEN];
	
	if (!(devEnum = mlEnumerateDevices())) {
		fprintf(stderr, "Failed to enumerate devices: %s (%d)\n", strerror(errno), errno);
		return 1;
	}
	
	descr = (MlDeviceDescr*) buf;
	
	while (!mlGetNextDevice(devEnum, descr, sizeof(buf))) {
		char type[64];
		
		printf("  %s: %s\n", descr->path, mlGetDeviceTypeDisplayString(descr->type, type, sizeof(type)));
	}
	
	mlCloseDeviceEnum(devEnum);
	
	return 0;
}
