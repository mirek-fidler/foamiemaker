#include "Hot4d.h"

void AirfoilCtrl::DoAction()
{
	SelectFileIn in("*.dat");
	if(!in)
		return;
	
	String name = TrimBoth(in.GetLine());
	
	ValueArray va;
	
	while(!in.IsEof()) {
		String ln = in.GetLine();
		CParser p(ln);
		Pt pt = Null;
		try {
			pt.x = p.ReadDouble();
			pt.y = p.ReadDouble();
		}
		catch(CParser::Error) {
			pt = Null;
		}
		if(!IsNull(pt)) {
			Value h;
			h("x") = pt.x;
			h("y") = pt.y;
			va.Add(h);
		}
	}
	
	Value h;
	h("name") = name;
	h("data") = va;
	SetData(h);
	
	Action();
}

struct CvAirfoil : Convert {
	virtual Value Format(const Value& v) const;
};

Value CvAirfoil::Format(const Value& v) const
{
	return v["name"];
}

AirfoilCtrl::AirfoilCtrl()
{
	SetConvert(Single<CvAirfoil>());
}


// TODO: https://www.particleincell.com/2012/bezier-splines/

Vector<Pt> SmoothAirfoil(const Vector<Pt>& foil)
{
	if(foil.GetCount() < 4)
		return clone(foil);

	struct Cons : LinearPathConsumer {
		Vector<Pt> r;
		
		virtual void Line(const Pointf& p) { r.Add(p); }
		virtual void Move(const Pointf& p) {}
		virtual void End() {}
	} r;

	r.r.Add(foil[0]);
	r.r.Add(foil[1]);
	int i;
	for(i = 1; i < foil.GetCount() - 2; i++) {
		Pt p2 = LineIntersection(foil[i - 1], foil[i], foil[i + 1], foil[i + 2]);
		if(!IsNull(p2)) {
		#if 1
			r.r.Add(p2);
			r.r.Add(foil[i + 1]);
		#else
			ApproximateQuadratic(r, foil[i], Mid(Mid(foil[i], foil[i + 1]), p2), foil[i + 1], 0.0001);
		#endif
		}
		else
			r.r.Add(foil[i + 1]);
	}
	r.r.Add(foil[i++]);
	r.r.Add(foil[i++]);
	return pick(r.r);
}

void AirfoilCtrl::Render(Path& path, double width, Pt p0, double te, bool smooth)
{
	Value p = GetData()["data"];

	Vector<Pt> foil;

	try {
		for(int i = 0; i < p.GetCount(); i++)
			foil.Add(Pt(p[i]["x"], p[i]["y"]));
		
		if(smooth)
			foil = SmoothAirfoil(foil);
	}
	catch(ValueTypeError) {
		return;
	}

	for(int i = 0; i < foil.GetCount(); i++) {
		Pt pt = foil[i];
		pt.x = 1 - pt.x;
		Pt a = width * pt;
		if(te && a.x < width / 4 && i < p.GetCount() / 2) // adjust te thickness in entry phase
			a.y = max(a.y, te);
		path.Kerf(a + p0);
	}
}

Vector<Pt> AirfoilCtrl::Get()
{
	Value p = GetData()["data"];

	Vector<Pt> foil;

	try {
		for(int i = 0; i < p.GetCount(); i++)
			foil.Add(Pt(p[i]["x"], p[i]["y"]));
	}
	catch(ValueTypeError) {
		return foil;
	}
	
	return foil;
}

void  InvertX(Vector<Pt>& foil)
{
	for(Pt& p : foil)
		p.x = 1 - p.x;
}

Vector<Pt> GetHalfFoil(const Vector<Pt>& foil, bool bottomhalf)
{
	Vector<Pt> r;
	double x = DBL_MAX;
	for(int i = 0; i < foil.GetCount(); i++) {
		if(foil[i].x > x) { // Get the top half of airfoil only
			if(bottomhalf) {
				r.Append(foil, i, foil.GetCount() - i);
				Reverse(r);
			}
			else
				r.Append(foil, 0, i);
			break;
		}
		x = foil[i].x;
	}
	InvertX(r);
	return r;
}

double GetMaxY(const Vector<Pt>& foil, double sgny)
{
	double ay = 0;
	for(const Pt& p : foil)
		ay = max(ay, sgny * p.y);
	return sgny * ay;
}

void  Mul(Vector<Pt>& foil, double ax, double ay)
{
	for(Pt& p : foil) {
		p.x *= ax;
		p.y *= ay;
	}
}

void CutHalfFoil(Vector<Pt>& foil, double head, double tail)
{
	if(tail > 0) {
		double tailx = 0;
		for(int i = 0; i < foil.GetCount(); i++) {
			Pt& a = foil[i];
			if(i > 0 && a.y > tail) {
				Pt& prev = foil[i - 1];
				prev = LineIntersection(prev, a, Pt(0, tail), Pt(1, tail));
				tailx = prev.x;
				foil.Remove(0, i - 1);
				break;
			}
		}
		for(Pt& a : foil)
			a.x -= tailx;
	}
	if(head > 0)
		for(int i = foil.GetCount() - 1; i > 0; i--) {
			Pt& a = foil[i];
			Pt& prev = foil[i - 1];
			if(prev.y > head) {
				a = LineIntersection(prev, a, Pt(0, head), Pt(1, head));
				foil.Trim(i + 1);
				break;
			}
		}
}
