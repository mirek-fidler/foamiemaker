#include "Hot4d.h"

void FourAxisDlg::CncSetup()
{
	WithCNCSetupLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, "");
	CtrlRetriever r;
	r
		(dlg.left, settings.left)
		(dlg.width, settings.width)
		(dlg.right, settings.right)
		(dlg.xmin, settings.xmin)
		(dlg.xmax, settings.xmax)
		(dlg.ymin, settings.ymin)
		(dlg.ymax, settings.ymax)
		(dlg.x, settings.x)
		(dlg.y, settings.y)
		(dlg.z, settings.z)
		(dlg.a, settings.a)
	;
	if(dlg.Execute() != IDOK)
		return;
	r.Retrieve();
	StoreAsJsonFile(settings, ConfigFile("cnc.json"), true);
}

void Settings::Jsonize(JsonIO& io)
{
	io
		("left", left)
		("width", width)
		("right", right)
		("xmin", xmin)
		("xmax", xmax)
		("ymin", ymin)
		("ymax", ymax)
		("x", x)
		("y", y)
		("z", z)
		("a", a)
	;
}

Settings::Settings()
{
	left = 84;
	width = 600;
	right = 97;
	xmin = -10;
	xmax = 480;
	ymin = 0;
	ymax = 200;
	x = "X";
	y = "Y";
	z = "Z";
	a = "A";
}
