#include "Hot4d.h"

void Circle(Path& path, Pt c, double r, double a0)
{
	int steps = int(r / 0.05);
	for(int j = 0; j <= steps; j++) {
		double a = j * M_2PI / steps + a0;
		path.Kerf(c.x - r * sin(a), c.y + r * cos(a));
	}
}

void CutSpar(Path& r, const Vector<Pt>& foil, int& i, Pt pos, Pt dim, bool circle, Point dir)
{
	DLOG("Spar");
	int si = foil[i].segment;
	DDUMP(si);
	Pt p = LineIntersection(foil[i - 1], foil[i],
	                        Pt(Nvl(pos.x, 0.0), Nvl(pos.y, 0.0)), Pt(Nvl(pos.x, 1.0), Nvl(pos.y, 1.0)));
	int pi = i - 1;
	while(pi > 0 && foil[pi] == p)
		pi--;
	Pt prev = foil[pi];
	if(circle) {
		Pt u = Orthogonal(p - prev) / Distance(p, prev);
		if(!IsNull(si))
			r.SetSegment(si);
		r.Kerf(p);
		r.To(p - u * dim.y);
		r.NewSegment();
		Circle(r, p - u * (dim.x / 2 + dim.y), dim.x / 2, dir.x < 0 ? M_PI : 0);
		r.NewSegment();
		r.To(p - u * dim.y);
		r.To(p);
		if(IsNull(si))
			r.NewSegment();
		else
			r.SetSegment(si);
	}
	else {
		Pt p2 = Null;
		while(i < foil.GetCount()) {
			double t1, t2;
			LineCircleIntersections(p, dim.x, foil[i - 1], foil[i], t1, t2, 999999);
			if(t1 < 0 || t1 > 1)
				t1 = DBL_MAX;
			if(t2 < 0 || t2 > 1)
				t2 = DBL_MAX;
			if(t2 < t1)
				Swap(t1, t2);
			if(t1 <= 1) {
				p2 = foil[i - 1] + (foil[i] - foil[i - 1]) * t1;
				if(dir.x * (p2.x - p.x) + dir.y * (p2.y - p.y) < 0)
					if(t2 <= 1)
						p2 = foil[i - 1] + (foil[i] - foil[i - 1]) * t2;
					else
						p2 = Null;
				if(!IsNull(p2))
					break;
			}
			i++;
		}
		if(!IsNull(p2)) {
			if(!IsNull(si))
				r.SetSegment(si);
			r.Kerf(p);
			r.NewSegment();
			Pt u = Orthogonal(p2 - p) / Distance(p2, p);
			p -= u * dim.y;
			r.Sharp(p);
			p -= Orthogonal(u) * dim.x;
			r.Sharp(p);
			r.Kerf(p2);
			if(IsNull(si))
				r.NewSegment();
			else
				r.SetSegment(foil[i].segment);
			r.Kerf(foil[i]);
		}
	}
	DLOG("End of spar");
}

bool DoSpar(Path& r, const Vector<Pt>& foil, int& i, Spar& spar)
{
	if(i <= 0)
		return false;
	Pt p = foil[i];
	Pt prev = foil[i - 1];
	bool circle = spar.shape == CIRCLE_SPAR;
	if(spar.kind == TOP_SPAR && p.x > spar.pos && p.x > foil[i - 1].x) {
		CutSpar(r, foil, i, Pt(spar.pos, Null), spar.dimension, circle, Point(1, 0));
		spar.kind = Null;
		return true;
	}
	if(spar.kind == BOTTOM_SPAR && p.x < spar.pos && p.x < foil[i - 1].x) {
		CutSpar(r, foil, i, Pt(spar.pos, Null), spar.dimension, circle, Point(-1, 0));
		spar.kind = Null;
		return true;
	}
	if(spar.kind == RIGHT_SPAR && p.y < spar.pos && p.y < foil[i - 1].y) {
		CutSpar(r, foil, i, Pt(spar.pos, Null), spar.dimension, circle, Point(0, 1));
		spar.kind = Null;
		return true;
	}
	return false;
}

bool DoSpars(Path& r, const Vector<Pt>& foil, int& i, Vector<Spar>& spars)
{
	for(Spar& s : spars)
		if(DoSpar(r, foil, i, s))
			return true;
	return false;
}

void ReadSpar(Vector<Spar>& spar, int kind, double le, Ctrl& pos, Ctrl& cx, Ctrl& cy, Ctrl& circle)
{
	Sizef dim = MakePoint(cx, cy);
	if(!IsNull(pos) && dim.cx > 0 && dim.cy > 0) {
		Spar& s = spar.Add();
		s.kind = kind;
		s.dimension = dim;
		s.pos = le - (double)~pos;
		s.shape = (bool)~circle ? CIRCLE_SPAR : RECTANGLE_SPAR;
		if(!(bool)~circle) {
			if(kind == TOP_SPAR)
				s.pos -= dim.cx / 2;
			if(kind == BOTTOM_SPAR)
				s.pos += dim.cx / 2;
		}
	}
}
