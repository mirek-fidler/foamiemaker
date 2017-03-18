#include "Hot4d.h"

Path Wing::Get()
{
	Path r;
	
	double t = Nvl((double)~te);
	double w = Nvl((double)~width);
	
	Pt tedim = MakePoint(te_spar_width, te_spar_height);
	
	Pt start = MakePoint(x, y);
	
	double sw = w;
	if(right) { // when tappered, start.x is sweep distance
		sw = Nvl((double)~other->width, 0.0) + Nvl((double)~other->x, 0.0);
		start.x = sw - start.x - w;
	}

	Vector<Spar> sp = spars.Get(Pointf(w, start.y), Sizef(-1, 1));
	
	if(te_sym)
		t /= 2;

	Vector<Pt> foil = airfoil.Get();
	int halfi = GetMaxIs(foil).left;
	for(int i = 0; i < foil.GetCount(); i++) {
		Pt& a = foil[i];
		a.x = 1 - a.x;
		a *= width;
		if(t && a.x < width / 2 && (te_sym || i < halfi)) // adjust te thickness
			a.y = (i < halfi ? 1 : -1) * max(abs(a.y), t);
	}
	
	double rot = Nvl((double)~rotate);

//	if(rot)
//		r.Rotate(width - Nvl((double)~rotate_around) + start.x, start.y, M_2PI * rot / 360.0);
	
	r.Offset(start.x, start.y);
	if(rot)
		r.Rotate(width - Nvl((double)~rotate_around), 0, M_2PI * rot / 360.0);

	Pt pp(-start.x, t);
	r.NewSegment();
	r.To(pp);
	r.NewSegment();

	r.Kerf(-(start.x > 5 ? 5 : start.x / 3), t); // this is lead-in to avoid long stay of wire
	r.NewSegment();

	bool top = true;
	for(int i = 0; i < foil.GetCount(); i++) {
		r.MainShape();
		Pt p = foil[i];
		if(top && i > 0 && p.x < foil[i - 1].x) { // LE split to new segment
			top = false;
			r.NewSegment();
		}
		if(!DoSpars(r, foil, i, sp)) {
			if(tedim.x > 0 && i > 0 && p.x < foil[i - 1].x && Distance(p, Pt(0, 0)) <= tedim.x) {
				double t, t2;
				Pt p0 = foil[i - 1];
				LineCircleIntersections(Pt(0, 0), tedim.x, p0, p, t, t2);
				if(t < 0 || t > 1)
					t = t2;
				if(t >= 0 && t <= 1) {
					p = (p - p0) * t + p0;
					Pt u = (p - p0) / Distance(p, foil[i - 1]);
					r.Kerf(p);
					r.NewSegment();
					r.Kerf(p - Orthogonal(u) * tedim.y);
					r.NewSegment();
					r.Kerf(p - Orthogonal(u) * tedim.y + u * tedim.x);
					break;
				}
			}
			else
				r.Kerf(p);
		}
		if(i == 0)
			r.NewSegment();
	}
	
	r.EndMainShape();

	if(cutte) {
		r.NewSegment();
		r.Kerf(0 /*-kerf*/, t); // TODO
	}

	t = te_sym ? -t : 0;

	r.NewSegment();
	r.Kerf(-(start.x > 5 ? 5 : start.x / 3), t); // this is lead-out to avoid long stay of wire

	r.NewSegment();
	r.Kerf(-start.x, t);

	r.NewSegment();

	r.Identity();

	return r;
}

Wing::Wing()
{
	CtrlLayout(*this);
	
	width <<= 180;
	y <<= 15;
	x <<= 10;
	te <<= 0.3;
	
	other = NULL;
	dlg = NULL;
	right = false;
}
