#include "Hot4d.h"

Path Wing::Get()
{
	DLOG("=============== WING");
	Path r;
	
	double t = Nvl((double)~te);
	double w = Nvl((double)~width);
	
	Pt ledim = MakePoint(le_spar_width, le_spar_height);
	Pt tedim = MakePoint(te_spar_width, te_spar_height);
	
	Pt start = MakePoint(x, y);
	
	if(right)
		start.x = Nvl((double)~other->width, 0.0) + Nvl((double)~other->x, 0.0) - start.x - w;

	Vector<Spar> spars;
	ReadSpar(spars, TOP_SPAR, w, top_spar, top_spar_width, top_spar_height, top_spar_circle);
	ReadSpar(spars, BOTTOM_SPAR, w, bottom_spar, bottom_spar_width, bottom_spar_height, bottom_spar_circle);

	Vector<Pt> foil = airfoil.Get();
	for(int i = 0; i < foil.GetCount(); i++) {
		Pt& a = foil[i];
		a.x = 1 - a.x;
		a *= width;
		if(t && a.x < width / 2 && i < foil.GetCount() / 2) // adjust te thickness
			a.y = max(a.y, t);
	}
	
	double rot = Nvl((double)~rotate);

	if(rot)
		r.Rotate(width - Nvl((double)~rotate_around) + start.x, start.y, M_2PI * rot / 360.0);
	
	r.Offset(start.x, start.y);
	if(rot)
		r.Rotate(width - Nvl((double)~rotate_around), 0, M_2PI * rot / 360.0);

	Pt pp(-start.x, te);
	r.NewSegment();
	r.To(pp);
	r.NewSegment();

	r.Kerf(-(start.x > 5 ? 5 : start.x / 3), te); // this is lead-in to avoid long stay of wire
	r.NewSegment();

	bool top = true;
	for(int i = 0; i < foil.GetCount(); i++) {
		r.MainShape();
		Pt p = foil[i];
		if(top && i > 0 && p.x < foil[i - 1].x) { // LE split to new segment
			top = false;
			r.NewSegment();
		}
		if(!DoSpars(r, foil, i, spars)) {
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
			if(ledim.x > 0 && p.y < ledim.x / 2 && i > 0 && p.y < foil[i - 1].y && !le_spar_circle) {
				CutSpar(r, foil, i, Pt(Null, ledim.x / 2), ledim, le_spar_circle, Point(0, -1));
				ledim.x = 0;
			}
			else
			if(le_spar_circle && ledim.x > 0 && i > 0 && p.y < foil[i - 1].y && p.y < 0) {
				r.NewSegment();
				Pt p1(width, 0);
				r.Kerf(p1);
				r.To(width - ledim.y, 0);
				Circle(r, Pt(width - ledim.y - ledim.x / 2, 0), ledim.x / 2, -M_PI / 2);
				r.To(p1);
				r.Kerf(p);
				ledim.x = 0;
				r.NewSegment();
			}
			else
				r.Kerf(p);
		}
		if(i == 0)
			r.NewSegment();
	}
	
//	airfoil.Render(r, Nvl((double)~width), start, t, ~smooth);

	DLOG("======== ENDING");

	r.EndMainShape();

	if(cutte) {
		r.NewSegment();
		r.Kerf(0 /*-kerf*/, te); // TODO
	}

	r.NewSegment();
	r.Kerf(-(start.x > 5 ? 5 : start.x / 3), 0); // this is lead-out to avoid long stay of wire

	r.NewSegment();
	r.Kerf(-start.x, 0);

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
