#include "Hot4d.h"

Rod::Rod()
{
	CtrlLayout(*this);

	y <<= 23;
	x <<= 10;

	left_x <<= 10;
	left_y <<= 10;
	right_x <<= 10;
	right_y <<= 10;
	rect_x <<= 50;
	rect_y <<= 40;
}

void Qel(Path& path, double cx, double cy, double rx, double ry, double from = 0)
{
	int n4 = int(sqrt(rx * rx + ry * ry) / 0.5);
	if(n4)
		for(int a = 0; a <= n4; a++) {
			double angle = M_PI * a / n4 / 2 + from;
			path.Kerf(rx * cos(angle) + cx, ry * sin(angle) + cy);
		}
	else
		path.Kerf(cx, cy);
}

Path Rod::Get()
{
	Path path;

	int n4 = 50;

	Sizef rect = MakePoint(rect_x, rect_y);
	Sizef left = MakePoint(left_x, left_y);
	Sizef right = MakePoint(right_x, right_y);

	rect /= 2;
	left /= 2;
	right /= 2;

	Pt center = MakePoint(x, y);
	center.x += rect.cx;

	path.To(0, center.y);

	Pt begin(center.x - rect.cx, center.y);

	path.MainShape();

	path.Kerf(begin);
	path.NewSegment();

	Qel(path, center.x - rect.cx + left.cx, center.y + rect.cy - left.cy, -left.cx, left.cy);

	path.Kerf(center.x, center.y + rect.cy);
	path.NewSegment();

	Qel(path, center.x + rect.cx - right.cx, center.y + rect.cy - right.cy, -right.cx, right.cy, M_PI / 2);

	path.Kerf(center.x + rect.cx, center.y);
	path.NewSegment();

	Qel(path, center.x + rect.cx - right.cx, center.y - rect.cy + right.cy, -right.cx, right.cy, M_PI);

	path.Kerf(center.x, center.y - rect.cy);
	path.NewSegment();

	Qel(path, center.x - rect.cx + left.cx, center.y - rect.cy + left.cy, -left.cx, left.cy, 3 * M_PI / 2);

	path.Kerf(begin);
	path.NewSegment();

	path.EndMainShape();

	Vector<Spar> spars;
	double w = center.x;
	ReadSpar(spars, TOP_SPAR, w, top_spar, top_spar_width, top_spar_height, top_spar_circle);
	ReadSpar(spars, RIGHT_SPAR, center.y, right_spar, right_spar_width, right_spar_height, right_spar_circle);
	ReadSpar(spars, BOTTOM_SPAR, w, bottom_spar, bottom_spar_width, bottom_spar_height, bottom_spar_circle);

	Path r;
	r.segment_counter = 1000;
	for(int i = 0; i < path.GetCount(); i++)
		if(!(path[i].mainshape && DoSpars(r, path, i, spars)))
			r.Add(path[i]);

	r.NewSegment();
	r.Kerf(0, center.y);

	return r;
}
