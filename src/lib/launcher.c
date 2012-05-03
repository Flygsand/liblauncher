#include "launcherp.h"



int mlSetState(MlDevice dev, unsigned int state)
{
	struct hiddev_usage_ref uref;
	unsigned int buf[2];
	int i;
	
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	memset(buf, 0, sizeof(buf));
	buf[0] = (unsigned char) state;
	
	uref.report_type = HID_REPORT_TYPE_OUTPUT;
	uref.report_id = 0;
	uref.field_index = 0;
	uref.usage_code = 0;
	
	for (i = 0; i < 8; ++i) {
		uref.usage_index = i;
		uref.value = ((unsigned char*) buf)[i];
		
		if (ioctl(dev->fd, HIDIOCSUSAGE, &uref) < 0) {
			fprintf(stderr, "Failed to send %d: %s (%d)\n", i, strerror(errno), errno);
			return -1;
		}
	}
	
	{
		struct hiddev_report_info info;
		
		info.report_type = HID_REPORT_TYPE_OUTPUT;
		info.report_id = uref.report_id;
		info.num_fields = 1;
		
		if (ioctl(dev->fd, HIDIOCSREPORT, &info) < 0) {
			fprintf(stderr, "Failed to send report: %s (%d)\n", strerror(errno), errno);
			return -1;
		}
	}
	
	dev->state = state;
	
	return 0;
}


double mlSetStateAndWait(MlDevice dev, unsigned int state, int inverted)
{
	unsigned int mask = 0, target;
	unsigned int status;
	struct timeval start, end;
	
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	if (state & MLC_UP) mask |= MLS_HIGHEST;
	else if (state & MLC_DOWN) mask |= MLS_LOWEST;
	else if (state & MLC_LEFT) mask |= MLS_LEFTMOST;
	else if (state & MLC_RIGHT) mask |= MLS_RIGHTMOST;
	else if (state & MLC_FIRE) mask |= MLS_ARMED;
	
	if (inverted) target = 0;
	else target = mask;
	
	if ((status = mlGetStatus(dev)) == MLS_UNKNOWN) {
		return -1;
	}
	
	if ((status & mask) == target)
		return 0;
	
	if (mlSetState(dev, state)) {
		return -1;
	}
	
	gettimeofday(&start, NULL);
	
	do {
		usleep(10000);
		
		if ((status = mlGetStatus(dev)) == MLS_UNKNOWN) {
			int tmp = errno;
			
			mlSetState(dev, MLC_NONE);
			errno = tmp;
			
			return -1;
		}
	} while ((status & mask) != target);
	
	gettimeofday(&end, NULL);
	
	if (mlSetState(dev, MLC_NONE)) {
		return -1;
	}
	
	return end.tv_sec * 1000 + end.tv_usec / 1000.0 - (start.tv_sec * 1000 + start.tv_usec / 1000.0);
}


double mlSetStateAndWaitMulti(MlDevice dev, unsigned int mask, unsigned int target)
{
	unsigned int state = 0;
	unsigned int status;
	struct timeval start, end;
	
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	if (((mask & MLS_LOWEST) && (target & MLS_LOWEST)) || ((mask & MLS_HIGHEST) && !(target & MLS_LOWEST)))
		state |= MLC_DOWN;
	
	if (((mask & MLS_LOWEST) && !(target & MLS_LOWEST)) || ((mask & MLS_HIGHEST) && (target & MLS_LOWEST)))
		state |= MLC_UP;
	
	if (((mask & MLS_LEFTMOST) && (target & MLS_LEFTMOST)) || ((mask & MLS_RIGHTMOST) && !(target & MLS_RIGHTMOST)))
		state |= MLC_LEFT;
	
	if (((mask & MLS_LEFTMOST) && !(target & MLS_LEFTMOST)) || ((mask & MLS_RIGHTMOST) && (target & MLS_RIGHTMOST)))
		state |= MLC_RIGHT;
	
	if (mask & MLS_ARMED)
		state |= MLC_FIRE;
	
	if ((status = mlGetStatus(dev)) == MLS_UNKNOWN) {
		return -1;
	}
	
	if ((status & mask) == target)
		return 0;
	
	gettimeofday(&start, NULL);
	
	do {
		if (((mask & MLS_LOWEST) && (target & MLS_LOWEST) == (status & MLS_LOWEST)) || ((mask & MLS_HIGHEST) && (target & MLS_HIGHEST) == (status & MLS_HIGHEST)))
			state &= ~(MLC_DOWN | MLC_UP);
		
		if (((mask & MLS_LEFTMOST) && (target & MLS_LEFTMOST) == (status & MLS_LEFTMOST)) || ((mask & MLS_RIGHTMOST) && (target & MLS_RIGHTMOST) == (status & MLS_RIGHTMOST)))
			state &= ~(MLC_LEFT | MLC_RIGHT);
		
		if ((mask & MLS_ARMED) && (target & MLS_ARMED) == (status & MLS_ARMED))
			state &= ~MLC_FIRE;
		
		if (mlSetState(dev, state)) {
			return -1;
		}
		
		usleep(10000);
		
		if ((status = mlGetStatus(dev)) == MLS_UNKNOWN) {
			int tmp = errno;
			
			mlSetState(dev, MLC_NONE);
			errno = tmp;
			
			return -1;
		}
	} while ((status & mask) != target);
	
	gettimeofday(&end, NULL);
	
	if (mlSetState(dev, MLC_NONE)) {
		return -1;
	}
	
	return end.tv_sec * 1000 + end.tv_usec / 1000.0 - (start.tv_sec * 1000 + start.tv_usec / 1000.0);
}


static void recalibrate(MlDevice dev, unsigned int status)
{
	if (!dev->usingFB)
		return;
	
	if (status & MLS_LEFTMOST) worldToDevice(dev, mlGetMaxYaw(dev), 0, &dev->yaw, NULL);
	else if (status & MLS_RIGHTMOST) worldToDevice(dev, mlGetMinYaw(dev), 0, &dev->yaw, NULL);
	
	if (status & MLS_LOWEST) worldToDevice(dev, 0, mlGetMinPitch(dev), NULL, &dev->pitch);
	else if (status & MLS_HIGHEST) worldToDevice(dev, 0, mlGetMaxPitch(dev), NULL, &dev->pitch);
}


unsigned int mlGetStatus(MlDevice dev)
{
	struct hiddev_usage_ref uref;
	struct hiddev_report_info info;
	unsigned char buf[8];
	int i;
	
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	info.report_type = HID_REPORT_TYPE_INPUT;
	info.report_id = 0;
	info.num_fields = 1;
	
	if (ioctl(dev->fd, HIDIOCGREPORT, &info) < 0) {
		fprintf(stderr, "Failed to get report: %s (%d)\n", strerror(errno), errno);
		return MLS_UNKNOWN;
	}
	
	uref.report_type = HID_REPORT_TYPE_INPUT;
	uref.report_id = info.report_id;
	uref.field_index = 0;
	uref.usage_code = 0;
	
	for (i = 0; i < sizeof(buf); ++i) {
		uref.usage_index = i;
		
		if (ioctl(dev->fd, HIDIOCGUSAGE, &uref) < 0) {
			fprintf(stderr, "Failed to get %d: %s (%d)\n", i, strerror(errno), errno);
			return MLS_UNKNOWN;
		}
		
		buf[i] = uref.value;
	}
	
	recalibrate(dev, *((unsigned int*) buf));
	
	return *((unsigned int*) buf);
}


int mlIsUsingFeedback(MlDevice dev)
{
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	errno = 0;
	
	return dev->usingFB;
}


int mlSetUsingFeedback(MlDevice dev, int v)
{
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	if (mlSetStateAndWaitMulti(dev, MLS_RIGHTMOST | MLS_LOWEST, MLS_RIGHTMOST | MLS_LOWEST) < 0) {
		return -1;
	}
	
	dev->usingFB = v;
	
	return 0;
}


int mlArm(MlDevice dev)
{
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	return mlSetStateAndWait(dev, MLC_FIRE, 0) < 0;
}


int mlFire(MlDevice dev)
{
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	if (mlArm(dev)) {
		return -1;
	}
	
	if (mlSetStateAndWait(dev, MLC_FIRE, 1) < 0) {
		return -1;
	}
	
	if (dev->shots > 0)
		--dev->shots;
	
	return 0;
}


int mlIsArmed(MlDevice dev)
{
	unsigned int status;
	
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	if ((status = mlGetStatus(dev)) == MLS_UNKNOWN) {
		return -1;
	}
	
	return (status & MLS_ARMED);
}


int mlAim(MlDevice dev, double yaw, double pitch)
{
	unsigned int ny, np;
	unsigned int states[2];
	unsigned long delays[2];
	int i;
	
	if (!dev || !dev->usingFB ||
			yaw < mlGetMinYaw(dev) || yaw > mlGetMaxYaw(dev) ||
			pitch < mlGetMinPitch(dev) || pitch > mlGetMaxPitch(dev)) {
		errno = EINVAL;
		return -1;
	}
	
	worldToDevice(dev, yaw, pitch, &ny, &np);
	memset(states, 0, sizeof(states));
	
	if (ny < dev->yaw) states[0] |= MLC_RIGHT;
	else if (ny > dev->yaw) states[0] |= MLC_LEFT;
	
	if (np < dev->pitch) states[0] |= MLC_DOWN;
	else if (np > dev->pitch) states[0] |= MLC_UP;
	
	states[1] = states[0];
	
	if (abs(ny - dev->yaw) < abs(np - dev->pitch)) {
		states[1] &= ~(MLC_RIGHT | MLC_LEFT);
		delays[0] = ny - dev->yaw;
		delays[1] = np - dev->pitch - (ny - dev->yaw);
	} else if (abs(ny - dev->yaw) == abs(np - dev->pitch)) {
		states[1] = 0;
		delays[0] = ny - dev->yaw;
		delays[1] = 0;
	} else {
		states[1] &= ~(MLC_UP | MLC_DOWN);
		delays[0] = np - dev->pitch;
		delays[1] = ny - dev->yaw - (np - dev->pitch);
	}
	
	for (i = 0; i < 2; ++i) {
		delays[i] *= (unsigned long) (mlGetTimeResolution(dev) * 1e6);
	}
	
#ifndef NDEBUG
	printf("Aim (%d, %d) -> (%d, %d)\n", dev->yaw, dev->pitch, ny, np);
	printf("States");
	for (i = 0; i < 2; ++i)
		printf(" (%02Xh, %lu)", states[i], delays[i]);
	printf("\n");
#endif
	
	for (i = 0; i < 2; ++i) {
		if (mlSetState(dev, states[i])) {
			return -1;
		}
		
		usleep(abs(delays[i]));
	}
	
	if (mlSetState(dev, MLC_NONE)) {
		return -1;
	}
	
	dev->yaw = ny;
	dev->pitch = np;
	
	return 0;
}


int mlAimDelta(MlDevice dev, double yaw, double pitch)
{
	double yaw0, pitch0;
	
	if (mlGetAim(dev, &yaw0, &pitch0))
		return -1;
	
	return mlAim(dev, yaw0 + yaw, pitch0 + pitch);
}


int mlGetAim(MlDevice dev, double *yaw, double *pitch)
{
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	deviceToWorld(dev, dev->yaw, dev->pitch, yaw, pitch);
	
	return 0;
}


int mlRealign(MlDevice dev)
{
	double yaw, pitch;
	
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	if (mlGetAim(dev, &yaw, &pitch)) {
		return -1;
	}
	
	if (mlSetStateAndWaitMulti(dev, MLS_RIGHTMOST | MLS_LOWEST, MLS_RIGHTMOST | MLS_LOWEST)) {
		return -1;
	}
	
	if (mlAim(dev, yaw, pitch)) {
		return -1;
	}
	
	return 0;
}


int mlGetAvailableShotsCount(MlDevice dev)
{
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	return dev->shots;
}


int mlSetAvailableShotsCount(MlDevice dev, unsigned int shots)
{
	if (!dev || !dev->usingFB || shots > mlGetShotsCount(dev)) {
		errno = EINVAL;
		return -1;
	}
	
	dev->shots = shots;
	
	return 0;
}


double mlGetMinYaw(MlDevice dev)
{
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	return -135.0;
}


double mlGetMaxYaw(MlDevice dev)
{
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	return 135.0;
}


double mlGetMinPitch(MlDevice dev)
{
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	return -10.0;
}


double mlGetMaxPitch(MlDevice dev)
{
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	return 30.0;
}


int mlGetShotsCount(MlDevice dev)
{
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	return 3;
}


double mlGetMaxYawTime(MlDevice dev)
{
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	return dev->maxYawTime;
}


double mlGetMaxPitchTime(MlDevice dev)
{
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	return dev->maxPitchTime;
}


double mlGetArmTime(MlDevice dev)
{
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	return dev->armTime;
}


double mlGetFireTime(MlDevice dev)
{
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	return dev->fireTime;
}


double mlGetTimeResolution(MlDevice dev)
{
	if (!dev) {
		errno = EINVAL;
		return -1;
	}
	
	return 10e-3;
}


int mlCalibrate(MlDevice dev)
{
	double yaw, pitch, arm, fire;
	
	if (!dev || !dev->usingFB) {
		errno = EINVAL;
		return -1;
	}
	
	if (mlSetStateAndWait(dev, MLC_LEFT, 0) < 0) {
		return -1;
	}
	
	if ((yaw = mlSetStateAndWait(dev, MLC_RIGHT, 0)) < 0) {
		return -1;
	}
	
	if (mlSetStateAndWait(dev, MLC_UP, 0) < 0) {
		return -1;
	}
	
	if ((pitch = mlSetStateAndWait(dev, MLC_DOWN, 0)) < 0) {
		return -1;
	}
	
	if (mlSetStateAndWait(dev, MLC_FIRE, 0) < 0) {
		return -1;
	}
	
	if (mlSetStateAndWait(dev, MLC_FIRE, 1) < 0) {
		return -1;
	}
	
	if ((arm = mlSetStateAndWait(dev, MLC_FIRE, 0)) < 0) {
		return -1;
	}
	
	if ((fire = mlSetStateAndWait(dev, MLC_FIRE, 1)) < 0) {
		return -1;
	}
	
	dev->maxYawTime = yaw;
	dev->maxPitchTime = pitch;
	dev->armTime = arm;
	dev->fireTime = fire;
	
	dev->yaw = 0;
	dev->pitch = 0;
	
	return 0;
}


ssize_t mlGetCalibration(MlDevice dev, void *buf, size_t size)
{
	CalibrationData *data = (CalibrationData*) buf;
	
	if (!dev || !dev->usingFB || size < sizeof(*data)) {
		errno = EINVAL;
		return -1;
	}
	
	data->version = 1;
	data->yawTime = (unsigned long) (dev->maxYawTime * 1000);
	data->pitchTime = (unsigned long) (dev->maxPitchTime * 1000);
	data->armTime = (unsigned long) (dev->armTime * 1000);
	data->fireTime = (unsigned long) (dev->fireTime * 1000);
	
	return 0;
}


int mlSetCalibration(MlDevice dev, const void *buf, size_t size)
{
	CalibrationData *data = (CalibrationData*) buf;
	
	if (!dev || !dev->usingFB || size < sizeof(*data)) {
		errno = EINVAL;
		return -1;
	}
	
	if (data->version > 1) {
		errno = EBADRQC;
		return -1;
	}
	
	dev->maxYawTime = dev->maxYawTime / 1000.0;
	dev->maxPitchTime = dev->maxPitchTime / 1000.0;
	dev->armTime = dev->armTime / 1000.0;
	dev->fireTime = dev->fireTime / 1000.0;
	
	return 0;
}
