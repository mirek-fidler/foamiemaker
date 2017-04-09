#include "Hot4d.h"

struct CNCSetupDlg : WithCNCSetupLayout<TopWindow> {
	typedef CNCSetupDlg CLASSNAME;
	
	void Sync();
	bool CheckMaterialName(const String& txt);
	void StoreParameters();
	void LoadParameters();

	bool Perform(Settings& settings);
	
	CNCSetupDlg();
};

bool CNCSetupDlg::CheckMaterialName(const String& txt)
{
	if(material.Find(txt) >= 0) {
		Exclamation("This material name is already used!");
		return false;
	}
	return true;
}

void CNCSetupDlg::Sync()
{
	bool b = material.IsCursor();
	edit_material.Enable(b);
	remove_material.Enable(b);
	add_parameters.Enable(b);
	
	b = b && parameters.IsCursor();
	edit_parameters.Enable(b);
	remove_parameters.Enable(b);
}

const char *ParamId[] = { "left_size", "left_kerf", "right_size", "right_kerf", "speed" };

void CNCSetupDlg::LoadParameters()
{
	parameters.Clear();
	if(!material.IsCursor())
		return;
	for(const auto& l : material.Get(1))
		parameters.AddMap(l);
	parameters.GoBegin();
}

void CNCSetupDlg::StoreParameters()
{
	if(!material.IsCursor())
		return;
	ValueArray va;
	for(int i = 0; i < parameters.GetCount(); i++)
		va << parameters.GetMap(i);
	material.Set(1, (Value)va);
}

CNCSetupDlg::CNCSetupDlg()
{
	CtrlLayoutOKCancel(*this, "Settings");
	
	material.AddColumn("Material");
	
	material.WhenSel = [=] { Sync(); LoadParameters(); };
	
	add_material ^= [=] {
		String txt;
	again:
		if(EditText(txt, "New material", "Name")) {
			if(!CheckMaterialName(txt))
				goto again;
			material.Add(txt);
			material.Sort();
			material.FindSetCursor(txt);
		}
	};
	
	edit_material ^= material.WhenLeftDouble = [=] {
		if(material.IsCursor()) {
			String txt = material.GetKey();
		again:
			if(!EditText(txt, "Rename Material", "Name"))
				return;
			if(!CheckMaterialName(txt))
				goto again;
			material.Set(0, txt);
			material.Sort();
			material.FindSetCursor(txt);
		}
	};
	
	remove_material ^= [=] { material.DoRemove(); };

	parameters.AddColumn("left_length", "Left");
	parameters.AddColumn("left_kerf", "Left kerf");
	parameters.AddColumn("right_length", "Right");
	parameters.AddColumn("right_kerf", "Right kerf");
	parameters.AddColumn("speed", "Speed");
	
	parameters.WhenSel = [=] { Sync(); };
	
	add_parameters ^= [=] {
		WithTaperParametersLayout<TopWindow> dlg;
		CtrlLayoutOKCancel(dlg, "Taper parameters");
		if(dlg.Execute() != IDOK)
			return;
		parameters.Add(~dlg.left_length, ~dlg.left_kerf, ~dlg.right_length, ~dlg.right_kerf, ~dlg.speed);
		parameters.GoEnd();
		StoreParameters();
	};
	edit_parameters ^= parameters.WhenLeftDouble = [=] {
		int q = parameters.GetCursor();
		if(q < 0)
			return;
		WithTaperParametersLayout<TopWindow> dlg;
		CtrlLayoutOKCancel(dlg, "Taper parameters");
		dlg.left_length <<= parameters.Get("left_length");
		dlg.left_kerf <<= parameters.Get("left_kerf");
		dlg.right_length <<= parameters.Get("right_length");
		dlg.right_kerf <<= parameters.Get("right_kerf");
		dlg.speed <<= parameters.Get("speed");
		if(dlg.Execute() != IDOK)
			return;
		parameters.Set("left_length", ~dlg.left_length);
		parameters.Set("left_kerf", ~dlg.left_kerf);
		parameters.Set("right_length", ~dlg.right_length);
		parameters.Set("right_kerf", ~dlg.right_kerf);
		parameters.Set("speed", ~dlg.speed);
		StoreParameters();
	};
	remove_parameters ^= [=] {
		parameters.DoRemove();
	};
}

bool CNCSetupDlg::Perform(Settings& settings)
{
	CtrlRetriever r;
	r
		(left, settings.left)
		(width, settings.width)
		(right, settings.right)
		(xmin, settings.xmin)
		(xmax, settings.xmax)
		(ymin, settings.ymin)
		(ymax, settings.ymax)
		(x, settings.x)
		(y, settings.y)
		(z, settings.z)
		(a, settings.a)
	;
	for(const auto& m : settings.material) {
		ValueArray va;
		for(const auto& p : m.param) {
			va << ValueMap()
					("left_length", p.left_length)
					("left_kerf", p.left_kerf)
					("right_length", p.right_length)
					("right_kerf", p.right_kerf)
					("speed", p.speed)
			;
		}
		material.Add(m.name, va);
	}
	if(Execute() != IDOK)
		return false;
	r.Retrieve();
	settings.material.Clear();
	for(int i = 0; i < material.GetCount(); i++) {
		Material& m = settings.material.Add();
		m.name = material.Get(i, 0);
		for(const auto& p : material.Get(i, 1)) {
			TaperParameters& tp = m.param.Add();
			tp.left_length = p["left_length"];
			tp.left_kerf = p["left_kerf"];
			tp.right_length = p["right_length"];
			tp.right_kerf = p["right_kerf"];
			tp.speed = p["speed"];
		}
	}
	return true;
}

void FourAxisDlg::CncSetup()
{
	if(CNCSetupDlg().Perform(settings))
		StoreAsJsonFile(settings, ConfigFile("cnc.json"), true);
}

void TaperParameters::Jsonize(JsonIO& io)
{
	io
		("left_length", left_length)
		("left_kerf", left_kerf)
		("right_length", right_length)
		("right_kerf", right_kerf)
		("speed", speed)
	;
}

void Material::Jsonize(JsonIO& io)
{
	io("name", name)("parameters", param);
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
		("materials", material)
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
