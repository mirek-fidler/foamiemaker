#include "Hot4d.h"

Path Angle::Get()
{
	Path r;
	double y1 = Nvl((double)~this->y1);
	double x = Nvl((double)~this->x);
	double y2 = Nvl((double)~this->y2);
	double a = Nvl((double)~this->angle);
	
	r.To(0, y1);
	r.To(x, y1);
	
	r.To(x + tan(a * M_2PI / 360.0) * (y2 - y1), y2);

	r.To(0, y2);
	
	return r;
}

Angle::Angle()
{
	CtrlLayout(*this);
	y1 <<= 3;
	y2 <<= 30;
	x <<= 10;
	angle <<= 6;
}
