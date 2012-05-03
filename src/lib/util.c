#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "launcher.h"



char* mlGetDeviceTypeDisplayString(MlDeviceType type, char *buf, size_t size)
{
	switch (type) {
	case ML_TYPE_UNKNOWN:
		strncpy(buf, "Unknown Device", size);
		break;
		
	case ML_TYPE_DREAMCHEEKY_MISSILE_LAUNCHER:
		strncpy(buf, "Dream Cheeky Missile Launcher", size);
		break;
		
	default:
		*buf = 0;
		return NULL;
	}
	
	buf[size - 1] = 0;
	return buf;
}
