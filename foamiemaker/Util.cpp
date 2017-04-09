#include "Hot4d.h"

VectorMap<String, Ctrl *> GetCtrlMap(Ctrl& parent)
{
	VectorMap<String, Ctrl *> map;
	for(Ctrl *q = parent.GetFirstChild(); q; q = q->GetNext()) {
		String id = q->GetLayoutId();
		if(!dynamic_cast<Label *>(q))
			map.Add(id, q);
	}
	return map;
}

void SetValues(Ctrl& parent, const ValueMap& json)
{
	auto h = GetCtrlMap(parent);
	for(auto& e : ~h)
		*e.value <<= json[e.key];
}

ValueMap GetValues(Ctrl& parent)
{
	ValueMap m;
	auto h = GetCtrlMap(parent);
	for(auto& e : ~h)
		m.Add(e.key, ~*e.value);
	return m;
}


void Shape::Load(const ValueMap& json)
{
	SetValues(*this, json);
}

ValueMap Shape::Save()
{
	return GetValues(*this);
}

Pt MakePoint(Ctrl& a, Ctrl& b)
{
	return Pt(Nvl((double)~a), Nvl((double)~b));
}

Pt MakePoint(double a, Ctrl& b)
{
	return Pt(a, Nvl((double)~b));
}

double LineIntersectionT(Pt a1, Pt a2, Pt b1, Pt b2)
{
	Pt s1 = a2 - a1;
	Pt s2 = b2 - b1;
	double t = (s2.x * (a1.y - b1.y) - s2.y * (a1.x - b1.x)) / (s1.x * s2.y - s2.x * s1.y);
	return IsNaN(t) ? Null : t;
}

Pt LineIntersection(Pt a1, Pt a2, Pt b1, Pt b2)
{
	double t = LineIntersectionT(a1, a2, b1, b2);
	if(IsNaN(t))
		return Null;
	return t * (a2 - a1) + a1;
}

int LineCircleIntersections(Pt c, double radius, Pt p1, Pt p2, double& t1, double& t2, double def)
{
	Pt d = p2 - p1;
	
	double A = Squared(d);
	double B = 2 * (d.x * (p1.x - c.x) + d.y * (p1.y - c.y));
	double C = Squared(p1 - c) - radius * radius;

	t1 = t2 = def;
	
	double det = B * B - 4 * A * C;
	if(A == 0 || det < 0) // TODO: Check A==0 condition
		return 0;
	else
	if (det == 0) {
		t1 = -B / (2 * A);
		return 1;
	}
	else {
		double h = sqrt(det);
		t1 = (-B + h) / (2 * A);
		t2 = (-B - h) / (2 * A);
		return 2;
	}
}

bool PathIntersection(Pointf a, Pointf b, const Vector<Pt>& path, int& ii, Pt& pt)
{
	for(int i = ii; i < path.GetCount(); i++)
		if(i > 0) {
			Pt p = SegmentIntersection(path[i - 1], path[i], a, b);
			if(!IsNull(p)) {
				ii = i;
				pt = p;
				return true;
			}
		}
	return false;
}

double PathLength(const Vector<Pt>& path, int from, int count)
{
	double length = 0;
	for(int i = from; i < from + count - 1; i++)
		length += Distance(path[i], path[i + 1]);
	return length;
}

double PathLength(const Vector<Pt>& path)
{
	return PathLength(path, 0, path.GetCount());
}

Pt AtPath(const Vector<Pt>& path, double at, Pt *dir1, int from)
{
	double length = 0;
	for(int i = from; i < path.GetCount(); i++) {
		double d = Distance(path[i], path[i + 1]);
		double h = at - length;
		if(h < d) {
			Pt d1 = (path[i + 1] - path[i]) / d;
			if(dir1)
				*dir1 = d1;
			return Pt(path[i] + h * d1).Attr(path[i + 1]);
		}
		length += d;
	}
	return Null;
}


bool IsOk(const Vector<Pt>& path)
{
	for(auto p : path)
		if(!(p.x > -100000 && p.x < 100000 && p.y > -100000 && p.y < 100000))
			return false;
	return true;
}
