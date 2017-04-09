#include "Hot4d.h"

int GetKerfAndSpeed(const Material& m, double taper, double& left_kerf, double& right_kerf, double& speed)
{
	double l_taper = -DBL_MAX;
	double l_left_kerf = 0;
	double l_right_kerf = 0;
	double l_speed = 0;
	
	double h_taper = DBL_MAX;
	double h_left_kerf = 0;
	double h_right_kerf = 0;
	double h_speed = 0;
	
	for(TaperParameters p : m.param) {
		for(int pass = 0; pass < 2; pass++) {
			double ptaper = p.left_length / p.right_length;
			if(ptaper <= taper && ptaper > l_taper) {
				l_taper = ptaper;
				l_left_kerf = p.left_kerf;
				l_right_kerf = p.right_kerf;
				l_speed = p.speed;
			}
			if(ptaper >= taper && ptaper < h_taper) {
				h_taper = ptaper;
				h_left_kerf = p.left_kerf;
				h_right_kerf = p.right_kerf;
				h_speed = p.speed;
			}
			Swap(p.left_length, p.right_length);
			Swap(p.left_kerf, p.right_kerf);
		}
	}
	
	if(l_taper == -DBL_MAX) {
		if(h_taper == DBL_MAX) {
			left_kerf = right_kerf = 0;
			speed = 140;
			return 2;
		}
		
		left_kerf = h_left_kerf;
		right_kerf = h_right_kerf;
		speed = h_speed;
		return 1;
	}
	
	if(h_taper == DBL_MAX) {
		left_kerf = l_left_kerf;
		right_kerf = l_right_kerf;
		speed = l_speed;
		return 1;
	}

	double k = abs(h_taper - l_taper) <= 0.001 ? 0 : (taper - l_taper) / (h_taper - l_taper);
	left_kerf = (1 - k) * l_left_kerf + k * h_left_kerf;
	right_kerf = (1 - k) * l_right_kerf + k * h_right_kerf;
	speed = (1 - k) * l_speed + k * h_speed;

	return 0;
}

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
		if(SquaredDistance(p1, c) > 5 * kerf * kerf)
			path << k1.Attr(p1)
			     << Pt(k1 + Unit(k1 - k0) * 2 * kerf).Attr(p1)
			     << Pt(k2 + Unit(k2 - k3) * 2 * kerf).Attr(p1)
			     << k2.Attr(p1);
		else
			path << c.Attr(p1);
	}
	if(k3 != in.Top())
		path << k3.Attr(p2);
	path << in.Top();
	
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
