#include "Hot4d.h"

Motor::Motor()
{
	CtrlLayout(*this);

	central_y <<= 20;
	leadin_x <<= 10;
	front_cx <<= 30;
	front_cy <<= 30;
	firewall_cx <<= 3;
	firewall_cy <<= 40;
	back_cx <<= 0;
	back_cy <<= 0;
	
	angle_x <<= angle_y <<= 0;
	
	angle << [=] {
		bool b = IsNull(angle);
		angle_x.Enable(b);
		angle_y.Enable(b);
	};
	
	angle.WhenAction();
}

Path Motor::Get()
{
	Path p;
	
	Pt center = MakePoint(leadin_x, central_y);
	Sizef front = MakePoint(front_cx, front_cy);
	Sizef firewall = MakePoint(firewall_cx, firewall_cy);
	Sizef back = MakePoint(back_cx, back_cy);
	double a = (double)~angle;
	
	if(IsNull(a))
		a = Bearing(MakePoint(angle_x, angle_y));
	else
		a = M_2PI * a / 360;

	p.Kerf(0, center.y);
	p.Rotate(center.x + front.cx, center.y, a);
	p.Kerf(center.x, center.y + front.cy / 2);
	p.Kerf(center.x + front.cx, center.y + front.cy / 2);
	p.Kerf(center.x + front.cx, center.y + firewall.cy / 2);
	p.Kerf(center.x + front.cx + firewall.cx, center.y + firewall.cy / 2);
	p.Kerf(center.x + front.cx + firewall.cx, center.y + back.cy / 2);
	p.Kerf(center.x + front.cx + firewall.cx + back.cx, center.y + back.cy / 2);
	p.Kerf(center.x + front.cx + firewall.cx + back.cx, center.y - back.cy / 2);
	p.Kerf(center.x + front.cx + firewall.cx, center.y - back.cy / 2);
	p.Kerf(center.x + front.cx + firewall.cx, center.y - firewall.cy / 2);
	p.Kerf(center.x + front.cx, center.y - firewall.cy / 2);
	p.Kerf(center.x + front.cx, center.y - front.cy / 2);
	p.Kerf(center.x, center.y - front.cy / 2);
	p.Identity();
	p.Kerf(0, center.y);
	
	return p;
}
