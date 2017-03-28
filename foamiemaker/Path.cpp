#include "Hot4d.h"

String Pt::ToString() const
{
	String h;
	h << (Pointf)*this;
	h << "segment: " << segment;
	if(kerf)
		h << " kerf";
	if(sharp)
		h << " sharp";
	if(mainshape)
		h << " mainshape";
	return h;
}

void Path::To(Pt p, bool kerf)
{
	auto& h = Add();
	(Pointf&)h = transform.Transform(p);
	h.kerf = kerf;
	h.segment = segment;
	h.mainshape = mainshape;
}

void Path::Sharp(double x, double y)
{
	Kerf(x, y);
	Top().sharp = true;
}

void Path::Rotate(double x, double y, double angle)
{
	transform = transform * Xform2D::Translation(-x, -y) * Xform2D::Rotation(angle) * Xform2D::Translation(x, y);
}

void Path::Offset(double x, double y)
{
	transform = transform * Xform2D::Translation(x, y);
}

double FourAxisDlg::GetKerf(bool right)
{
	return (negative_kerf ? -1 : 1) * Nvl(double(right ? ~right_kerf : ~kerf), Nvl((double)~kerf));
}

Vector<Pt> GetKerfPath(const Vector<Pt>& path, double k)
{
	Vector<Pt> h;
	
	int i = 0;
	Pt pt(0, 0);
	while(i < path.GetCount())
		if(k && path[i].kerf) {
			Vector<Pt> kh;
			kh.Add(pt);
			while(i < path.GetCount() && path[i].kerf)
				kh.Add(path[i++]);
			h.Append(KerfCompensation(kh, k / 2));
		}
		else {
			pt = path[i++];
			h.Add(pt);
		}

	if(!IsOk(h))
		h.Clear();

	return h;
}

void NormalizeSegments(Vector<Pt>& path)
{
	if(path.GetCount() == 0)
		return;
	int sgi = -1;
	int sg;
	for(Pt& pt : path) {
		if(sgi < 0 || pt.segment != sg) {
			sgi++;
			sg = pt.segment;
		}
		pt.segment = sgi;
	}
}

void FourAxisDlg::MakePaths(Vector<Pt> *shape, Vector<Pt> *path, Vector<Pt> *cnc, double inverted, bool mirrored)
{
	for(int r = 0; r < 1 + IsTapered(); r++) {
		Vector<Pt>& p = shape[r];
		shape[r] = CurrentShape(r).Get();
		if(IsOk(p)) {
			if(!IsNull(inverted) && p.GetCount()) {
				for(int i = 0; i < p.GetCount(); i++) {
					Pt& pt = p[i];
					if(i + 1 < p.GetCount())
						pt.SetAttr(p[i + 1]);
					else
						pt.SetAttr(p[0]);
					pt.y = 2 * inverted - pt.y;
				}
				Reverse(p);
			}
			if(cut)
				p.Add(Pt(0, GetFoamHeight()).Segment(999));
			p.Add(Pt(0, 0).Segment(1000));
		}
		else
			p.Clear();
	}
	
	if(mirrored)
		Swap(shape[0], shape[1]);
		
	for(int r = 0; r < 1 + IsTapered(); r++) {
		Vector<Pt>& p = shape[r];
		path[r] = GetKerfPath(p, GetKerf(mirrored ? !r : r));
		NormalizeSegments(path[r]);
	}

	if(IsTapered()) {
		cnc[0].Add(Pointf(0, 0));
		cnc[1].Add(Pointf(0, 0));
		MixAll(path[0], path[1], cnc[0], cnc[1]);
		if(cnc[0].GetCount() == cnc[1].GetCount()) {
			double panel = Nvl((double)~panel_width);
			CncPath(cnc[0], cnc[1], panel,
			        settings.left + settings.width + settings.right,
			        mirrored ? settings.left + settings.width - panel : settings.left);
			if(!IsOk(cnc[0]))
				cnc[0].Clear();
			if(!IsOk(cnc[1]))
				cnc[1].Clear();
		}
		else {
			cnc[0].Clear();
			cnc[1].Clear();
		}
	}
	else {
		cnc[0] = clone(path[0]);
		cnc[1] = clone(path[0]);
	}
}

Rectf GetBounds(const Vector<Pt>& path)
{
	Rectf r = Null;
	
	for(const auto& p : path)
		if(p.mainshape)
			if(IsNull(r))
				r = Rectf(p, p);
			else
				r.Union(p);

	return r;
}
