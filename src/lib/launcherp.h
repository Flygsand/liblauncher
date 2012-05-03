#ifndef _LAUNCHERP_H_
#define _LAUNCHERP_H_

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm/types.h>
#include <linux/hiddev.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "launcher.h"


/* --- Types --- */
struct _MlDevice {
	int fd;
	MlDeviceType type;
	int usingFB;
	unsigned int yaw, pitch;
	unsigned int state;
	unsigned int shots;
	
	double maxYawTime, maxPitchTime, armTime, fireTime;
};

typedef struct {
	unsigned int version;
	unsigned long yawTime;
	unsigned long pitchTime;
	unsigned long armTime;
	unsigned long fireTime;
} CalibrationData;


/* --- Functions --- */
static inline void worldToDevice(MlDevice dev, double yaw, double pitch, unsigned int *dyaw, unsigned int *dpitch)
{
	if (dyaw) *dyaw = (unsigned int) ((mlGetMaxYawTime(dev) * (yaw - mlGetMinYaw(dev)) / (mlGetMaxYaw(dev) - mlGetMinYaw(dev))) / mlGetTimeResolution(dev));
	if (dpitch) *dpitch = (unsigned int) ((mlGetMaxPitchTime(dev) * (pitch - mlGetMinPitch(dev)) / (mlGetMaxPitch(dev) - mlGetMinPitch(dev))) / mlGetTimeResolution(dev));
}


static inline void deviceToWorld(MlDevice dev, unsigned int dyaw, unsigned int dpitch, double *yaw, double *pitch)
{
	if (yaw) *yaw = mlGetMinYaw(dev) + dyaw * mlGetTimeResolution(dev) * (mlGetMaxYaw(dev) - mlGetMinYaw(dev)) / mlGetMaxYawTime(dev);
	if (pitch) *pitch = mlGetMinPitch(dev) + dpitch * mlGetTimeResolution(dev) * (mlGetMaxPitch(dev) - mlGetMinPitch(dev)) / mlGetMaxPitchTime(dev);
}


#endif /* _LAUNCHERP_H_ */
