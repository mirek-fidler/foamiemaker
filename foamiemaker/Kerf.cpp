#include "Hot4d.h"

bool IsNaN(Pt p)
{
	return IsNaN(p.x) || IsNaN(p.y);
}

Pt Unit(Pt p)
{
	return p / Length(p);
}

void KerfLine(Pt p0, Pt p1, Pt& k0, Pt& k1, double kerf)
{
	Pt o = Orthogonal(p1 - p0);
	o = Unit(o) * kerf;
	k0 = p0 + o;
	k1 = p1 + o;
}

Vector<Pt> KerfCompensation(const Vector<Pt>& in0, double kerf)
{
	Vector<Pt> path;

	if(in0.GetCount() == 0)
		return path;

	Vector<Pt> in;
	in.Add(in0[0]);
	for(int i = 1; i < in0.GetCount(); i++)
		if(in.Top() != in0[i])
			in.Add(in0[i]);
	
	Pt k3, p2;
	
	for(int i = 0; i < in.GetCount() - 2; i++) {
		Pt p0 = in[i];
		Pt p1 = in[i + 1];
		p2 = in[i + 2];
		
		Pt k0, k1;
		KerfLine(p0, p1, k0, k1, kerf);
		
		if(i == 0)
			path << k0.Attr(p0);

		double angle = Bearing(p2 - p1) - Bearing(p0 - p1);
		if(angle < 0)
			angle += M_2PI;

		Pt axis = -(1 - 2 * (angle < M_PI)) * Unit(Unit(p1 - p0) + Unit(p1 - p2)); // angle axis unit vector
		
		Pt k2;
		KerfLine(p1, p2, k2, k3, kerf);

		Pt s1 = k1 - k0;
		Pt s2 = k3 - k2;
		double t = (s2.x * (k0.y - k2.y) - s2.y * (k0.x - k2.x)) / (s1.x * s2.y - s2.x * s1.y);
		Pt c = t * (k1 - k0) + k0;
		if(angle > M_PI - 0.001 && angle < M_PI + 0.001) // Almost straight line, just use k1
			path << k1.Attr(p1);
		else
		if(angle < M_PI) {
			if(SquaredDistance(p1, c) > 5 * kerf * kerf)
				path << k1.Attr(p1)
				     << Pt(k1 + Unit(k1 - k0) * 2 * kerf).Attr(p1)
				     << Pt(k2 + Unit(k2 - k3) * 2 * kerf).Attr(p1)
				     << k2.Attr(p1);
			else
				path << c.Attr(p1);
		}
		else {
			if(sin(angle / 2) * min(Distance(p0, p1), Distance(p1, p2)) < kerf)
				path << Pt(kerf * axis + p1).Attr(p1);
			else
			if(SquaredDistance(p1, c) > 1.3 * 1.3 * kerf * kerf) {
				path << c.Attr(p1);
				if(p1.sharp)
					path << Pt(kerf * axis + p1).Attr(p1);
				path << c.Attr(p1);
			}
			else
				path << c.Attr(p1);
		}
	}
	path << k3.Attr(p2) << in.Top();
	
	return path;
}

#ifdef _DEBUG
struct KerfTestWindow : TopWindow {
	Pt p0 = Pt(200, 200);
	Pt p1 = Pt(300, 100);
	Pt p2 = Pt(400, 100);
	Pt p3 = Pt(500, 200);
	
	virtual void Paint(Draw& w) {
		double kerf = 10;

		ImagePainter p(GetSize());
		p.Clear(White());
		
		Vector<Pt> in, path;
		Pt b = Pt(50, 150);
		Pt n = Pt(600, 400);
		in << p0 << p1 << p2 << p3;
		
		path.Add(b);
		path.Add(in[0]);
		path.Append(KerfCompensation(in, kerf));
		path.Add(n);

		bool first = true;
		for(auto x : path) {
			if(!IsNaN(x))
				if(first) {
					p.Move(x);
					first = false;
				}
				else
					p.Line(x);
		}
		if(!first)
			p.LineCap(LINECAP_ROUND).LineJoin(LINEJOIN_ROUND).Stroke(2 * kerf, Blend(White(), LtRed(), 20)).Stroke(1, Red());

		p.Move(b);
		for(auto pt : in)
			p.Line(pt);
		p.Line(n);
		p.Stroke(1, Black());

		w.DrawImage(0, 0, p);
	}
	
	virtual void LeftDown(Point p, dword keyflags) {
		p1 = p;
		Refresh();
	}

	virtual void MouseMove(Point p, dword keyflags) {
		p2 = p;
		Refresh();
	}
	
	virtual void RightDown(Point p, dword keyflags) {
		p3 = p;
		Refresh();
	}
	
	KerfTestWindow() {
		Sizeable().Zoomable();
	}
};

void TestKerf()
{
	KerfTestWindow().Run();
}
#endif
