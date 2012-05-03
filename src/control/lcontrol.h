#ifndef _LCONTROL_H_
#define _LCONTROL_H_


#include <string>
#include <wx/wx.h>
#include "launcher.h"


class LauncherThread;


/**
 * The Launcher class encapsulates the controlling thread needed for
 * asynchronous stuff.
**/
class Launcher
{
	friend class LauncherThread;
	
	std::string path;
	MlDevice dev;
	int err;
	int opened;
	LauncherThread *thread;
	
public:
	/**
	 * Create a new Launcher object for the specified device, but do
	 * not open it.
	 *
	 * @param path path to the device.
	**/
	Launcher(const std::string &path);
	
	/**
	 * Destroy the Launcher object, and close the device.
	**/
	~Launcher();
	
	/**
	 * Close the device.
	**/
	void close();
	
	/**
	 * Open the device.
	 *
	 * On failure, use <i>getLastError()</i> to get the cause.
	 *
	 * @returns true on success, false on failure.
	**/
	bool open();
	
	/**
	 * Read the status of the device, as defined in <i>mlGetStatus()</i>.
	 *
	 * @returns a combination of <i>MLS_</i> bitmasks.
	**/
	unsigned int getStatus();
	
	/**
	 * Move the turret
	 *
	 * Move left when <i>yaw</i> is positive, and move right when negative.
	 * Move up when <i>pitch</i> is positive, and move down when negative.
	 *
	 * On failure, use <i>getLastError()</i> to get the cause.
	 *
	 * @param yaw some value.
	 * @param pitch some value.
	 * @returns true on success, false on failure.
	**/
	bool move(int yaw, int pitch);
	
	/**
	 * Arm one missile.
	 *
	 * On failure, use <i>getLastError()</i> to get the cause.
	 *
	 * @returns true on success, false on failure.
	**/
	bool arm();
	
	/**
	 * Fire a previously armed missile.
	 *
	 * On failure, use <i>getLastError()</i> to get the cause.
	 *
	 * @returns true on success, false on failure.
	**/
	bool fire();
	
	/**
	 * Return the last error code.
	 *
	 * @returns an <i>errno</i> compatible code.
	**/
	int getLastError() const;
	
	/**
	 * Set the last error code.
	 *
	 * @param err an <i>errno</i> compatible code.
	**/
	void setLastError(int err);
};


#endif /* _LCONTROL_H_ */
