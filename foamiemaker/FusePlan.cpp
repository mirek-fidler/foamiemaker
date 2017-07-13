#include "Hot4d.h"

Path FusePlan::Get()
{
	Path r;
	
	Pt start = MakePoint(x, y);
	
	Sizef dim = MakePoint(cx, cy);
	
	Vector<Pt> foil = GetHalfFoil(airfoil.Get());
	
	double ay = GetMaxY(foil);

	if(ay)
		ay = dim.cy / ay;
	
	Mul(foil, 1, ay);

	CutHalfFoil(foil, Nvl((double)~head_y), Nvl((double)~tail_y));
	
	Mul(foil, foil.GetCount() ? dim.cx / foil.Top().x : 0, 1);

	r.To(0, start.y);
	r.Kerf(start);
	r.Offset(start.x, start.y);

	Vector<Spar> sp = spars.Get();
	for(int i = 0; i < foil.GetCount(); i++)
		if(!DoSpars(r, foil, i, sp))
			r.Kerf(foil[i]);

	double x = dim.cx;
	for(int i = 0; i < list.GetCount(); i++) {
		double len = Nvl((double)list.Get(i, 0));
		double width = Nvl((double)list.Get(i, 1));
		if(len > 0) {
			r.Kerf(x, width);
			x -= len;
			r.Kerf(x, width);
		}
	}
	
	r.Kerf(x, 0);

	r.Identity();
	
	r.Kerf(0, start.y);

	return r;
}

void FusePlan::Load(const ValueMap& json)
{
	Shape::Load(json);
	list.Clear();
	ValueArray va = json["sectors"];
	for(int i = 0; i < va.GetCount(); i++) {
		list.Add(va[i]["length"], va[i]["width"]);
	}
}

ValueMap FusePlan::Save()
{
	ValueMap m = Shape::Save();
	ValueArray va;
	for(int i = 0; i < list.GetCount(); i++)
		va.Add(ValueMap()("length", list.Get(i, 0))("width", list.Get(i, 1)));
	m.Add("sectors", va);
	return m;
}

FusePlan::FusePlan()
{
	CtrlLayout(*this);
	
	y <<= 5;
	x <<= 10;
	
	list.AddColumn("Length").Ctrls<EditDoubleSpin>();
	list.AddColumn("Width").Ctrls<EditDoubleSpin>();
	list.SetLineCy(EditDouble::GetStdHeight() + 2);
	list.EvenRowColor();
	list.Moving();
	list.WhenCtrlsAction << [=] { WhenAction(); };
	
	add << [=] {
		list.Add();
		WhenAction();
	};
	insert << [=] {
		int ii = list.GetCursor();
		if(ii < 0)
			return;
		list.Insert(ii);
		WhenAction();
	};
	remove << [=] {
		int q = list.GetCursor();
		if(q < 0)
			return;
		list.Remove(q);
		WhenAction();
	};
}
