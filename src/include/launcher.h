#ifndef _LAUNCHER_H_
#define _LAUNCHER_H_

#include <sys/types.h>


/* --- Defines --- */
/* .: Control States :. */
#define MLC_NONE		0
#define MLC_UP			0x0001
#define MLC_DOWN		0x0002
#define MLC_LEFT		0x0004
#define MLC_RIGHT		0x0008
#define MLC_FIRE		0x0010

/* .: Status Codes :. */
#define MLS_UNKNOWN		0xFFFF
#define MLS_LOWEST		0x0040
#define MLS_HIGHEST		0x0080
#define MLS_LEFTMOST	0x0400
#define MLS_RIGHTMOST	0x0800
#define MLS_ARMED		0x8000


/* --- Types --- */
typedef struct _MlDevice *MlDevice;
typedef struct _MlDeviceEnum *MlDeviceEnum;

/**
 * A list of all known device types.
**/
typedef enum {
	ML_TYPE_UNKNOWN,
	ML_TYPE_DREAMCHEEKY_MISSILE_LAUNCHER
} MlDeviceType;

/**
 * This type is used in the enumeration of available devices.
**/
typedef struct {
	MlDeviceType type;
	char *path;
} MlDeviceDescr;


/* --- Functions --- */
#ifdef __cplusplus
extern "C" {
#endif

/* .: Utility :. */
/**
 * Get a pretty string from the device type.
 *
 * @param type the type to lookup.
 * @param buf  an output buffer.
 * @param size the size of the output buffer.
 * @return the output buffer on success, NULL on failure.
**/
extern char* mlGetDeviceTypeDisplayString(MlDeviceType type, char *buf, size_t size);

/* .: Enumeration :. */
/**
 * Create an enumeration context on all available devices.
 *
 * On error, <i>errno</i> is set.
 *
 * The context should be closed with <i>mlCloseDeviceEnum</i>.
 *
 * @return the context or NULL on error.
**/
extern MlDeviceEnum mlEnumerateDevices(void);

/**
 * Close a previously created enumeration context.
 *
 * @param e the context.
**/
extern void mlCloseDeviceEnum(MlDeviceEnum e);

/**
 * Get the next device from an enumeration context.
 *
 * On error, <i>errno</i> is set.
 *
 * @param e     the context.
 * @param descr an output buffer.
 * @param size  the size of the buffer.
 * @return zero on success, non-zero otherwise.
**/
extern int mlGetNextDevice(MlDeviceEnum e, MlDeviceDescr *descr, size_t size);

/* .: Device :. */
/**
 * Open a device.
 *
 * On error, <i>errno</i> is set.
 *
 * @param dev the path to the device node.
 * @return a device handle on success, NULL on error.
**/
extern MlDevice mlOpenDevice(const char *dev);

/**
 * Close a prevously opened device.
 *
 * On error, <i>errno</i> is set.
 *
 * @param dev a device handle.
**/
extern void mlCloseDevice(MlDevice dev);

/**
 * Get the type of device.
 *
 * @param dev a device handle.
 * @return the type, or ML_TYPE_UNKNOWN on error.
**/
extern MlDeviceType mlGetDeviceType(MlDevice dev);

/* .: Low-level Control :. */
/**
 * Control the turret.
 *
 * This function outputs control commands to the device.
 * It is asynchronous by nature. Once a command is sent,
 * the device will continue until you explictly stop it.
 *
 * On error, <i>errno</i> is set.
 *
 * @param dev a device handle.
 * @param state a combination of the <i>MLC_</i> bitmasks.
 * @return zero on success, non-zero on error.
**/
extern int mlSetState(MlDevice dev, unsigned int state);

/**
 * Control the turret.
 *
 * This function outputs control commands to the device
 * and waits until it cannot be performed any more.
 *
 * In essence, it waits for an appropriate limit switch to be closed.
 *
 * On error, <i>errno</i> is set.
 *
 * @param dev a device handle.
 * @param state a combination of the <i>MLC_</i> bitmasks.
 * @param inverted a flag indicating that the limit switched should be opened.
 * @return the time occupied.
**/
extern double mlSetStateAndWait(MlDevice dev, unsigned int state, int inverted);

/**
 * Get the current status of a device.
 *
 * Thus function returns the current status of the device.
 *
 * On error, <i>errno</i> is set.
 *
 * @param dev a device handle.
 * @return a combination of the <i>MLS_</i> bitmasks.
**/
extern unsigned int mlGetStatus(MlDevice dev);

/* .: High-level Control :. */
extern int mlIsUsingFeedback(MlDevice dev);
extern int mlSetUsingFeedback(MlDevice dev, int v);
extern int mlArm(MlDevice dev);
extern int mlFire(MlDevice dev);
extern int mlIsArmed(MlDevice dev);
extern int mlAim(MlDevice dev, double yaw, double pitch);
extern int mlAimDelta(MlDevice dev, double yaw, double pitch);
extern int mlGetAim(MlDevice dev, double *yaw, double *pitch);
extern int mlRealign(MlDevice dev);
extern int mlGetAvailableShotsCount(MlDevice dev);
extern int mlSetAvailableShotsCount(MlDevice dev, unsigned int shots);

/* .: Static Spatial Device Properties :. */
extern double mlGetMinYaw(MlDevice dev);
extern double mlGetMaxYaw(MlDevice dev);
extern double mlGetMinPitch(MlDevice dev);
extern double mlGetMaxPitch(MlDevice dev);
extern int mlGetShotsCount(MlDevice dev);

/* .: Temporal (Static or Calibrated) Device Properties :. */
extern double mlGetMaxYawTime(MlDevice dev);
extern double mlGetMaxPitchTime(MlDevice dev);
extern double mlGetArmTime(MlDevice dev);
extern double mlGetFireTime(MlDevice dev);
extern double mlGetTimeResolution(MlDevice dev);

/* .: High-level Calibration :. */
extern int mlCalibrate(MlDevice dev);
extern ssize_t mlGetCalibration(MlDevice dev, void *buf, size_t size);
extern int mlSetCalibration(MlDevice dev, const void *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _LAUNCHER_H_ */
