#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#include <cerrno>
#include "lcontrol.h"


using namespace std;



class LauncherThread : public wxThread
{
	Launcher *l;
	unsigned int state;
	unsigned int mask, target;
	wxCriticalSection stateLock;
	
public:
	LauncherThread(Launcher *l)
		: wxThread(wxTHREAD_JOINABLE), l(l),
		  state(0), mask(0), target(0),
		  stateLock()
	{
	}
	
	unsigned int statusOfState(unsigned int c)
	{
		unsigned int ret = 0;
		
		if (c & MLC_LEFT) ret |= MLS_LEFTMOST;
		if (c & MLC_RIGHT) ret |= MLS_RIGHTMOST;
		if (c & MLC_UP) ret |= MLS_HIGHEST;
		if (c & MLC_DOWN) ret |= MLS_LOWEST;
		if (c & MLC_FIRE) ret |= MLS_ARMED;
		
		return ret;
	}
	
	void cancel(unsigned int status, unsigned int s, unsigned int c)
	{
		if ((status & s) == (target & s) || !(mask & s)) {
			state &= ~c;
			mask &= ~s;
			target &= ~s;
		}
	}
	
	
	ExitCode Entry()
	{
		while (l->opened) {
			unsigned int status;
			
			if ((status = mlGetStatus(l->dev)) == MLS_UNKNOWN) {
				l->opened = false;
				break;
			}
			
			{
				wxCriticalSectionLocker lock(stateLock);
				
				cancel(status, MLS_LEFTMOST, MLC_LEFT);
				cancel(status, MLS_RIGHTMOST, MLC_RIGHT);
				cancel(status, MLS_HIGHEST, MLC_UP);
				cancel(status, MLS_LOWEST, MLC_DOWN);
				cancel(status, MLS_ARMED, MLC_FIRE);
				
				if (mlSetState(l->dev, state)) {
					l->opened = false;
					break;
				}
			}
			
			usleep((unsigned long) (mlGetTimeResolution(l->dev) * 1000));
		}
		
		return 0;
	}
	
	unsigned int getState() const
	{
		return state;
	}
	
	bool setState(unsigned int m, unsigned int c, unsigned int t)
	{
		unsigned int sm = statusOfState(m);
		unsigned int s = statusOfState(c);
		wxCriticalSectionLocker lock(stateLock);
		
		state = (state & ~m) | c;
		mask = (mask & ~sm) | s;
		target = (target & ~sm) | (statusOfState(t) & s);
		
		return true;
	}
};


Launcher::Launcher(const std::string &path)
	: path(path), dev(0), err(0), opened(false),
	  thread(new LauncherThread(this))
{
}


Launcher::~Launcher()
{
	close();
}


void Launcher::close()
{
	opened = false;
	if (thread) thread->Wait();
	if (dev) mlCloseDevice(dev);
}


bool Launcher::open()
{
	if (!(dev = mlOpenDevice(path.c_str()))) {
		setLastError(errno);
		
		return false;
	}
	
	opened = true;
	
	if (thread->Create() != wxTHREAD_NO_ERROR ||
			thread->Run() != wxTHREAD_NO_ERROR) {
		mlCloseDevice(dev);
		setLastError(ENOMEM);
		
		return false;
	}
	
	return true;
}


bool Launcher::move(int x, int y)
{
	unsigned int state = 0;
	
	if (x) state |= (x < 0 ? MLC_LEFT : MLC_RIGHT);
	if (y) state |= (y < 0 ? MLC_DOWN : MLC_UP);
	
	return thread->setState(
		MLC_LEFT | MLC_RIGHT | MLC_DOWN | MLC_UP,
		state,
		MLC_LEFT | MLC_RIGHT | MLC_DOWN | MLC_UP);
}


bool Launcher::arm()
{
	return thread->setState(MLC_FIRE, MLC_FIRE, MLC_FIRE);
}


bool Launcher::fire()
{
	return thread->setState(MLC_FIRE, MLC_FIRE, 0);
}


int Launcher::getLastError() const
{
	return err;
}


void Launcher::setLastError(int err)
{
	this->err = err;
}
