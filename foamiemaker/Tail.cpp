#include "Hot4d.h"

Path Tail::Get()
{
	Path r;
	
	Vector<Pt> shape;

	shape.Add(MakePoint(0, top_y1));
	shape.Add(MakePoint(top_x, top_y1));

	Pointf c = MakePoint(middle_x, middle_y);
	double h2 = Nvl((double)~middle_height) / 2;
	
	Pointf top = MakePoint(top_x, top_y2);
	Sizef radius = Sizef(top.x - c.x, top.y - c.y - h2);
	for(int i = 0; i <= 30; i++) {
		double angle = M_PI_2 * i / 30;
		double sina = sin(angle);
		double cosa = cos(angle);
		shape.Add(Pointf(top.x - radius.cx * sina, c.y + h2 + radius.cy * cosa));
	}
	
	Pointf bottom = MakePoint(bottom_x, bottom_y1);
	radius = Sizef(bottom.x - c.x, c.y - h2 - bottom.y);

	for(int i = 30; i >= 0; i--) {
		double angle = M_PI_2 * i / 30;
		double sina = sin(angle);
		double cosa = cos(angle);
		shape.Add(Pointf(bottom.x - radius.cx * sina, c.y - h2 - radius.cy * cosa));
	}

	shape.Add(MakePoint(bottom_x, bottom_y2));
	shape.Add(MakePoint(0, bottom_y2));
	
	if(saddle) {
		Vector<Pt> sa;
		double sch = Nvl((double)~saddle_chord);
		if(saddle_isairfoil) {
			sa = saddle_airfoil.Get();
			InvertX(sa);
			Mul(sa, sch, sch);
			if(sa.GetCount())
				sa.Add(sa[0]);
		}
		else {
			double h = Nvl((double)~saddle_height, 0.0);
			sa.Add(Pt(0, 0));
			sa.Add(Pt(0, h / 2));
			sa.Add(Pt(sch, h / 2));
			sa.Add(Pt(sch, -h / 2));
			sa.Add(Pt(0, -h / 2));
			sa.Add(Pt(0, 0));
		}
		double angle = -Nvl((double)~saddle_incidence);
		if(angle) {
			angle = M_2PI * angle / 360;
			double sina = sin(angle);
			double cosa = cos(angle);
			for(Pt& p : sa) {
				p.x = cosa * p.x + sina * p.y;
				p.y = -sina * p.x + cosa * p.y;
			}
		}
		Pt spos = MakePoint(saddle_x, saddle_y);
		for(Pt& p : sa)
			p += spos;
		
		bool dosaddle = true;
		
		Vector<Pt> shape2;
		for(int i = 0; i < shape.GetCount(); i++) {
			int ii = 0;
			Pt pt;
			if(i > 0 && dosaddle && PathIntersection(shape[i - 1], shape[i], sa, ii, pt)) {
				shape2.Add(pt);
				for(int pass = 0; pass < 2 && dosaddle; pass++) {
					while(ii < sa.GetCount()) {
						shape2.Add(sa[ii++]);
						if(ii > 0 && ii < sa.GetCount() && PathIntersection(sa[ii - 1], sa[ii], shape, i, pt)) {
							shape2.Add(pt);
							dosaddle = false;
							break;
						}
					}
					ii = 0;
				}
			}
			if(i < shape.GetCount())
				shape2.Add(shape[i]);
		}
		
		if(dosaddle) {
			Pt ms0(-c.x / 2, 0);
			Pt ms(-c.x / 2, spos.y);
			sa.Insert(0, ms0);
			sa.Insert(1, ms);
			sa.Add(ms);
			sa.Add(ms0);
			shape.Insert(0, sa);
		}
		else
			shape = pick(shape2);
	}
	
	Vector<Spar> sp = spars.Get();

	for(int i = 0; i < shape.GetCount(); i++) {
		r.MainShape();
		if(!DoSpars(r, shape, i, sp))
			r.Kerf(shape[i]);
	}

	return r;
}

Tail::Tail()
{
	CtrlLayout(*this);

	top_x <<= 30;
	top_y1 <<= 30;
	top_y2 <<= 27;
	middle_x <<= 10;
	middle_y <<= 15;
	middle_height <<= 0;
	bottom_x <<= 30;
	bottom_y1 <<= 4;
	bottom_y2 <<= 2;
}
