#include "Hot4d.h"

void Circle(Path& path, Pt c, double r, double a0)
{
	int steps = int(r / 0.05);
	for(int j = 0; j <= steps; j++) {
		double a = j * M_2PI / steps + a0;
		path.Kerf(c.x - r * sin(a), c.y + r * cos(a));
	}
}

void CutSpar(Path& r, const Vector<Pt>& foil, int& i, Pt pos, const Spar& spar, Point dir)
{
	int si = foil[i].segment;
	Pt p = LineIntersection(foil[i - 1], foil[i],
	                        Pt(Nvl(pos.x, 0.0), Nvl(pos.y, 0.0)), Pt(Nvl(pos.x, 1.0), Nvl(pos.y, 1.0)));
	int pi = i - 1;
	while(pi > 0 && foil[pi] == p)
		pi--;
	Pt prev = foil[pi];
	double depth = Nvl(spar.depth);
	Sizef dim = spar.dimension;
	double a = spar.angle;
	if(!IsNull(a)) {
		if(spar.kind == TOP_SPAR)
			a += 90;
		if(spar.kind == BOTTOM_SPAR)
			a -= 90;
		a = M_2PI * a / 360;
	}
	if(spar.shape == CIRCLE_SPAR || depth) {
		Pt u = IsNull(a) ? Orthogonal(p - prev) / Distance(p, prev) : Polar(a);
		if(!IsNull(si))
			r.SetSegment(si);
		r.Kerf(p);
		Pointf p1 = p - u * spar.depth;
		r.To(p1);
		r.NewSegment();
		if(spar.shape == CIRCLE_SPAR)
			Circle(r, p - u * (dim.cx / 2 + dim.cy), dim.cx / 2, Bearing(u) - M_PI / 2);
		else {
			r.Sharp(p1 + Orthogonal(u) * dim.cx / 2);
			r.Sharp(p1 + Orthogonal(u) * dim.cx / 2 - u * dim.cy);
			r.Sharp(p1 - Orthogonal(u) * dim.cx / 2 - u * dim.cy);
			r.Sharp(p1 - Orthogonal(u) * dim.cx / 2);
			r.Sharp(p1);
		}
		r.NewSegment();
		r.To(p1);
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
			LineCircleIntersections(p, dim.cx, foil[i - 1], foil[i], t1, t2, 999999);
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
			p -= u * dim.cy;
			r.Sharp(p);
			p -= Orthogonal(u) * dim.cx;
			r.Sharp(p);
			r.Kerf(p2);
			if(IsNull(si))
				r.NewSegment();
			else
				r.SetSegment(foil[i].segment);
			r.Kerf(foil[i]);
		}
	}
}

bool DoSpar(Path& r, const Vector<Pt>& foil, int& i, Spar& spar)
{
	if(i <= 0)
		return false;
	Pt p = foil[i];
	Pt prev = foil[i - 1];
	bool circle = spar.shape == CIRCLE_SPAR;
	if(spar.kind == TOP_SPAR && p.x > spar.pos && p.x > foil[i - 1].x) {
		CutSpar(r, foil, i, Pt(spar.pos, Null), spar, Point(1, 0));
		spar.kind = Null;
		return true;
	}
	if(spar.kind == BOTTOM_SPAR && p.x < spar.pos && p.x < foil[i - 1].x) {
		CutSpar(r, foil, i, Pt(spar.pos, Null), spar, Point(-1, 0));
		spar.kind = Null;
		return true;
	}
	if(spar.kind == RIGHT_SPAR && p.y < spar.pos && p.y < foil[i - 1].y) {
		CutSpar(r, foil, i, Pt(spar.pos, Null), spar, Point(0, 1));
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
