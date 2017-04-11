#include "Hot4d.h"

#define IMAGECLASS HotImg
#define IMAGEFILE <FoamieMaker/HotImg.iml>
#include <Draw/iml_source.h>

void FourAxisDlg::AddShape(Shape *l, Shape *r)
{
	shapes.Add(l->GetId(), MakeTuple(l, r));
	type.Add(type.GetCount(), l->GetName());
	left.Add(*l);
	if(r)
		right.Add(*r);
}


FourAxisDlg::FourAxisDlg()
{
	CtrlLayout(*this, "4 axis CNC G-code generator for RC modelling");

	LoadFromJsonFile(settings, ConfigFile("cnc.json"));

	AddShape(rod, rod + 1);
	AddShape(text);
	AddShape(angle);
	wing[0].other = &wing[1];
	wing[0].right = false;
	wing[1].other = &wing[0];
	wing[1].right = true;
	wing[0].dlg = wing[1].dlg = this;
	wing[1].x_lbl = "Sweep";
	wing[1].x <<= 0;
	AddShape(wing, wing + 1);
	AddShape(motor);
	AddShape(textpath);
	AddShape(fuseplan);
	AddShape(fuseprofile);
	AddShape(tail);
	
	WhenClose = [=] { Exit(); };

	panel_width <<= 500;
	
	for(auto i : { 1, 2, 3, 5, 10, 20, 30, 50, 100, 200, 500, 1000 })
		scale.Add(i, AsString(i) + " pixels / mm");
	scale <<= 5;

	type << [=] { Type(); };
	type <<= 0;
	
	tapered << [=] { Type(); };
	
	view.fa = this;
	view.Add(home.BottomPos(0, Zy(14)).LeftPos(0, Zy(14)));
	home.NoWantFocus();
	home.SetImage(HotImg::Home());
	home << [=] { Home(); };
	
	show_shape <<= true;
	show_arrows <<= false;
	show_wire <<= true;
	show_kerf <<= false;
	show_points <<= false;
	show_left <<= true;
	show_right <<= true;
	show_cnc <<= false;
	show_planform <<= true;

	for(Ctrl *q = GetFirstChild(); q; q = q->GetNext())
		*q << [=] { Sync(); };

	for(int i = 0; i < shapes.GetCount(); i++) {
		shapes[i].a->WhenAction << [=] { Sync(); };
		for(Ctrl *q = shapes[i].a->GetFirstChild(); q; q = q->GetNext())
			*q << [=] { Sync(); };
		if(shapes[i].b) {
			shapes[i].b->WhenAction << [=] { Sync(); };
			for(Ctrl *q = shapes[i].b->GetFirstChild(); q; q = q->GetNext())
				*q << [=] { Sync(); };
		}
	}

	Type();
	
	Sizeable().Zoomable();
	
	StoreRevision();
	
	SyncMaterial();
}

Shape& FourAxisDlg::CurrentShape0(bool right) const
{
	int q = ~type;
	if(q < 0 || q >= shapes.GetCount()) return const_cast<Rod&>(rod[0]);
	auto t = shapes[~type];
	return t.a->IsTaperable() && tapered && right ? *t.b : *t.a;
}

Shape& FourAxisDlg::CurrentShape(bool right)
{
	return CurrentShape0(right);
}

const Shape& FourAxisDlg::CurrentShape(bool right) const
{
	return CurrentShape0(right);
}

void FourAxisDlg::Type()
{
	for(int i = 0; i < shapes.GetCount(); i++) {
		auto t = shapes[i];
		bool b = ~type == i;
		t.a->Show(b);
		if(t.a->IsTaperable())
			t.b->Show(b && tapered);
	}
	tapered.Show(CurrentShape().IsTaperable());
	
	bool b = tapered;

	panel_width_lbl.Show(b);

	panel_width.Show(b);
	
	show_left.Show(b);
	show_right.Show(b);
	show_cnc.Show(b);
	show_planform.Show(b);

	Sync();
}

void FourAxisDlg::SyncMaterial()
{
	material.ClearList();
	for(const auto& m : settings.material)
		material.Add(m.name);
	if(IsNull(material))
		material.GoBegin();
}

bool FourAxisDlg::Key(dword key, int count)
{
	if(key == K_HOME) {
		Home();
		return true;
	}
	return TopWindow::Key(key, count);
}

void FourAxisDlg::Exit()
{
	if(FastCompress(MakeSave()) != revision)
		switch(PromptYesNoCancel("Do you want to save the changes to the file?")) {
		case 1:
			if(!Save())
				return;
			break;
		case -1:
			return;
		}
	Close();
}

void FourAxisDlg::Serialize(Stream& s)
{
	SerializeGlobalConfigs(s);
	s % lrufile;
	SerializePlacement(s);
	s % show_shape % show_wire % show_kerf;
}

void FourAxisDlg::Layout()
{
	Sync();
}

void Shape::SyncView()
{
	Ctrl *q = this;
	while(q) {
		auto *h = dynamic_cast<FourAxisDlg *>(q);
		if(h) {
			h->Sync();
			break;
		}
		q = q->GetParent();
	}
}

GUI_APP_MAIN
{
#ifdef _DEBUG0
	TopWindow win;
	SparsCtrl ctrl;
	win.Add(ctrl.SizePos());
	win.Run();
	DDUMP(AsJSON(ctrl.GetData()));
	return;
#endif
#ifdef _DEBUG0
	void TestMix();
	TestMix();
	return;
#endif
	
#ifdef _DEBUG0
	TestKerf();
	return;
#endif

//	SetVppLogName("c:/xxx/fa/" + AsString(GetCurrentProcessId()) + ".log");
	
	DUMP("A");

	FourAxisDlg dlg;
	DUMP("B");

	LoadFromFile(dlg);

	DUMP("C");

	const Vector<String>& cmdline = CommandLine();
	int argc = cmdline.GetCount();

	if(argc == 1 && ToLower(GetFileExt(cmdline[0])) == ".nc") {
		if(!dlg.OpenS(cmdline[0]))
			return;
	}

	DUMP("D");
	
	dlg.Execute();

	Ctrl::EventLoop();
	
	StoreToFile(dlg);
}
