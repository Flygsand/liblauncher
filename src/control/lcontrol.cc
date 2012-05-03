#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif
#include <cerrno>
#include <cmath>
#include <iostream>
#include <sys/param.h>
#include <wx/wx.h>
#include <wx/propdlg.h>
#include "lcontrol.h"


using namespace std;


/* --- Data --- */
Launcher *launcher = 0;



static void arm()
{
	if (!launcher->arm()) {
		wxMessageDialog dlg(0, wxT("Failed to arm the missile."), wxT("Error"), wxOK);
		
		cerr << "Failed to arm the missile: " << strerror(errno) << " (" << errno << ")" << endl;
		dlg.ShowModal();
	}
}


static void fire()
{
	if (!launcher->fire()) {
		wxMessageDialog dlg(0, wxT("Failed to fire the missile."), wxT("Error"), wxOK);
		
		cerr << "Failed to fire the missile: " << strerror(errno) << " (" << errno << ")" << endl;
		dlg.ShowModal();
	}
}


class JoystickButton : public wxControl
{
	double x, y;
	
public:
	JoystickButton(wxWindow *parent, wxWindowID id)
		: wxControl(parent, id)
	{
	}
	
	wxSize DoGetBestSize() const
	{
		return wxSize(30, 30);
	}
	
	void OnPaint(wxPaintEvent &e)
	{
		wxPaintDC dc(this);
		wxSize size = dc.GetSize();
		int r = std::min(size.x, size.y) / 2;
		
		dc.DrawCircle(size.x / 2, size.y / 2, r);
		dc.DrawLine(0, size.y / 2, size.x, size.y / 2);
		dc.DrawLine(size.x / 2, 0, size.x / 2, size.y);
		
		dc.SetBrush(*wxBLACK_BRUSH);
		dc.DrawCircle((wxCoord) ((x * size.x) + size.x) / 2, (wxCoord) ((y * size.y) + size.y) / 2, r / 3);
	}
	
	void OnValue(double x, double y)
	{
		if (!launcher->move((int) (x * 3), (int) (y * 3))) {
			cerr << "Failed to set state: " << strerror(launcher->getLastError()) << " (" << launcher->getLastError() << ")" << endl;
			return;
		}
	}
	
	void OnMouse(wxMouseEvent &e)
	{
		if (e.LeftIsDown()) {
			wxPoint p = e.GetPosition();
			wxSize size = GetClientSize();
			double d, v, x1, y1;
			
			x1 = 2 * (double) (p.x - size.x / 2) / size.x;
			y1 = 2 * (double) (p.y - size.y / 2) / size.y;
			d = std::sqrt(x1 * x1 + y1 * y1);
			v = std::atan2(y1, x1);
			
			if (d > 2.0 / 3)
				d = 2.0 / 3;
			
			x = d * std::cos(v);
			y = d * std::sin(v);
			
			OnValue(x, -y);
			Refresh(false);
		} else if (e.LeftUp()) {
			x = 0;
			y = 0;
			
			OnValue(x, y);
			Refresh(false);
		}
	}
	
	void OnChar(wxKeyEvent &e)
	{
		switch (e.GetKeyCode()) {
		case 'a':
		case 'A':
			arm();
			break;
			
		case 'f':
		case 'F':
			fire();
			break;
		}
	}
	
	
	DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(JoystickButton, wxControl)
	EVT_PAINT(JoystickButton::OnPaint)
	EVT_MOUSE_EVENTS(JoystickButton::OnMouse)
	EVT_CHAR(JoystickButton::OnChar)
END_EVENT_TABLE()


class LowLevelPage : public wxPanel
{
	JoystickButton *dir;
	wxButton *arm, *fire;
	
public:
	LowLevelPage(wxWindow *parent)
		: wxPanel(parent),
		  dir(new JoystickButton(this, wxID_ANY)),
		  arm(new wxButton(this, 0, wxT("Arm"))),
		  fire(new wxButton(this, 1, wxT("Fire")))
	{
		wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
		
		sizer->Add(dir, 3, wxALIGN_CENTER | wxSHAPED | wxALL, 10);
		sizer->Add(arm, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
		sizer->Add(fire, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
		
		SetSizer(sizer);
		sizer->SetSizeHints(this);
	}
	
	void OnArm(wxCommandEvent &e)
	{
		::arm();
	}
	
	void OnFire(wxCommandEvent &e)
	{
		::fire();
	}
	
	DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(LowLevelPage, wxPanel)
	EVT_BUTTON(0, LowLevelPage::OnArm)
	EVT_BUTTON(1, LowLevelPage::OnFire)
END_EVENT_TABLE()


class ControlDialog : public wxDialog
{
	wxNotebook *book;
	wxStatusBar *status;
	LowLevelPage *ll;
	
public:
	ControlDialog()
		: wxDialog(0, wxID_ANY, wxT("Launcher Control " PACKAGE_VERSION)),
		  book(new wxNotebook(this, wxID_ANY)),
		  status(new wxStatusBar(this, wxID_ANY, 0)),
		  ll(new LowLevelPage(book))
	{
		wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
		
		book->AddPage(ll, wxT("Low Level"));
		book->SetSelection(0);
		
		sizer->Add(book, 1, wxEXPAND | wxALL, 0);
		sizer->Add(status, 0, wxALL, 0);
		
		status->SetStatusText(wxT("Welcome"));
		
		SetSizer(sizer);
		sizer->SetSizeHints(this);
	}
};


class ControlApp : public wxApp
{
	ControlDialog *dlg;
	
public:
	static string findDevice()
	{
		MlDeviceEnum devEnum;
		MlDeviceDescr *descr;
		unsigned char buf[sizeof(*descr) + MAXPATHLEN];
		string ret;
		
		if (!(devEnum = mlEnumerateDevices())) {
			wxMessageDialog dlg(0, wxT("Failed to enumerate devices."), wxT("Error"), wxOK);
			
			cerr << "Failed to enumerate devices: " << strerror(errno) << " (" << errno << ")" << endl;
			dlg.ShowModal();
			
			return "";
		}
		
		descr = (MlDeviceDescr*) buf;
		
		while (!mlGetNextDevice(devEnum, descr, sizeof(buf))) {
			ret = descr->path;
		}
		
		mlCloseDeviceEnum(devEnum);
		
		return ret;
	}
	
	
	bool OnInit()
	{
		string path = findDevice();
		
		if (path.empty())
			return false;
		
		launcher = new Launcher(path);
		
		if (!launcher->open()) {
			wxMessageDialog dlg(0, wxT("Failed to open device."), wxT("Error"), wxOK);
			
			cerr << "Failed to open device '" << path << "': " << strerror(launcher->getLastError()) << " (" << launcher->getLastError() << ")" << endl;
			dlg.ShowModal();
			
			return false;
		}
		
		dlg = new ControlDialog();
		
		SetTopWindow(dlg);
		dlg->ShowModal();
		
		delete launcher;
		
		return false;
	}
};


IMPLEMENT_APP(ControlApp)
