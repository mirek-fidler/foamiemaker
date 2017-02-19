#include "Hot4d.h"

View::View()
{
	SetFrame(ViewFrame());
}

void View::Paint(Draw& w)
{
	w.DrawImage(0, 0, img);
}

void View::LeftDown(Point p, dword)
{
	if(fa) fa->ViewDown(p);
}

void View::LeftUp(Point p, dword)
{
	if(fa) fa->ViewUp(p);
}

void View::MouseMove(Point p, dword)
{
	if(fa) fa->ViewMove(p);
}

void View::MouseWheel(Point p, int zdelta, dword)
{
	if(fa) fa->ViewWheel(p, zdelta);
}

void View::LeftDrag(Point p, dword)
{
	if(fa) fa->ViewDrag(p);
}

void View::LeftHold(Point p, dword)
{
	if(fa) fa->ViewDrag(p);
}

void FourAxisDlg::ViewDown(Point p)
{
	drag = false;
	click = p;
	click_org = org;
}

void FourAxisDlg::ViewUp(Point p)
{
	if(!drag)
		CurrentShape().AddPoint(GetViewPos(p));
	drag = false;
}

void FourAxisDlg::ViewMove(Point p)
{
	if(drag) {
		org = p - click + click_org;
		Sync();
		view.Sync();
	}
	Pointf h = GetViewPos(p);
	info_x = Format("X: %.3f", h.x);
	info_y = Format("Y: %.3f", h.y);
}

void FourAxisDlg::ViewWheel(Point p, int zdelta)
{
	Zoom(sgn(zdelta), p);
}

void FourAxisDlg::ViewDrag(Point p)
{
	view.SetCapture();
	drag = true;
}

Pt FourAxisDlg::GetViewPos(Point p)
{
	Point origin = ViewOrigin();
	p -= org;
	return Pt(p.x - origin.x, origin.y - p.y) / (double)~scale;
}

void FourAxisDlg::Zoom(int dir, Point p)
{
	Pt pos = GetViewPos(p);
	scale.SetIndex(minmax(scale.GetIndex() + dir, 0, scale.GetCount() - 1));

	Point origin = ViewOrigin();
	
	double z = (double)~scale;
	Point p1;
	p1.x = int(origin.x + z * pos.x);
	p1.y = int(origin.y - z * pos.y);
	org = p - p1;

	Sync();
}

void FourAxisDlg::ViewPars(Font& fnt, int& w, Point& origin) const
{
	Size isz = view.GetSize();
	fnt = Arial(Zy(12));
	w = Zx(4);
	origin = Pt(GetTextSize("9999", fnt).cx + w, isz.cy - fnt.GetLineHeight() - w);
}

Point FourAxisDlg::ViewOrigin() const
{
	Font fnt;
	int w;
	Point origin;
	ViewPars(fnt, w, origin);
	return origin;
}

void FourAxisDlg::Sync()
{
	Title(filepath);
	
	org.x = min(org.x, 0);
	org.y = max(org.y, 0);

	Size isz = view.GetSize();

	double scale = ~this->scale;
	
	if(isz.cx > 0 && isz.cy > 0 && scale > 0) {
		
		bool ok = true;
		for(auto q = GetFirstChild(); q; q = q->GetNext())
			if(q->GetData().IsError())
				ok = false;
	
		Font fnt;
		int w;
		Point origin;
		ViewPars(fnt, w, origin);
		
		ImagePainter p(isz);

// TODO: bug in preclip
//		p.PreClip();
	
		p.Translate(0.5, 0.5);
		p.Clear(White());
	
		double ny = INT_MAX;
		double nx = INT_MIN;
		p.Begin();
		for(int i = 0;; i += 5) {
			double si = i * scale;

			double px = origin.x + si + org.x;
			double py = origin.y - si + org.y;
			
			if(px > isz.cx && py < 0 || i > 2000)
				break;

			String s = AsString(i);
			Size tsz = GetTextSize(s, fnt);
			bool ten = i % 10 == 0;
			int w1 = Zx(ten ? 4 : 2);
			Color c = ten ? LtGray() : WhiteGray();

			if(px >= origin.x)
				p.Move(px, origin.y + w1).Line(origin.x + si + org.x, 0).Stroke(1, c);
			double x = px - tsz.cx / 2;
			if(x > nx + Zx(2)) {
				if(px >= origin.x)
					p.Text(x, origin.y + w, s, fnt).Fill(Black());
				nx = x + tsz.cx;
			}
			
			if(py <= origin.y)
				p.Move(origin.x - w, py).Line(isz.cx, py).Stroke(1, c);
			double y = py - tsz.cy / 2;
			if(y < ny - Zy(2)) {
				if(py <= origin.y)
					p.Text(origin.x - tsz.cx - Zx(6), y, s, fnt).Fill(Black());
				ny = y - tsz.cy;
			}
		}
		p.End();

		p.Move(origin).Line(isz.cx, origin.y).Stroke(1, Black());
		p.Move(origin).Line(origin.x, 0).Stroke(1, Black());
	
		p.Begin();
		p.RectPath(origin.x, 0, isz.cx - origin.x, origin.y);
		p.Clip();
	
		if(ok) {
			double k = Nvl((double)~kerf);
			
			Vector<Pt> shape[2];
			Vector<Pt> path[2];
			Vector<Pt> cnc[2];
			
			MakePaths(shape, path, cnc, show_inverted ? GetInvertY() : (double)Null);

			bool show[2];
			
			if(IsTapered()) {
				show[0] = show_left;
				show[1] = show_right;

				if(show_planform) {
					Rectf l = GetBounds(shape[0]);
					Rectf r = GetBounds(shape[1]);
					double width = Nvl((double)~panel_width);
					
					if(!IsNull(l) && !IsNull(r) && width > 0) {
						Font fnt = Arial(13);
						Size tsz = GetTextSize("99999 dm", fnt);
						double x = isz.cx - tsz.cx - width;
						double h = max(l.left, l.right, r.left, r.right);
						double y = 3 * tsz.cy + h;
						p.Move(x, y - l.left)
				         .Line(x + width, y - r.left)
				         .Line(x + width, y - r.right)
				         .Line(x, y - l.right)
				         .Close().Stroke(2, Gray());
				        double yy = y - h / 2 - tsz.cy;
				        p.Text(x - tsz.cx, yy, Format("%.1f", l.right - l.left), fnt)
				         .Text(x + width + 10, yy, Format("%.1f", r.right - r.left), fnt)
				         .Text(x, y, Format("%.1f degrees",
				               atan2((l.right + l.left) / 2 - (r.right + r.left) / 2, width) * 360 / M_2PI),
				                   fnt)
				          .Fill(Blue());

						h = max(l.top, l.bottom, r.top, r.bottom);
						y += 3 * tsz.cy + h;
						p.Move(x, y - l.top)
				         .Line(x + width, y - r.top)
				         .Line(x + width, y - r.bottom)
				         .Line(x, y - l.bottom)
				         .Close().Stroke(2, Gray());
				        yy = y - h / 2 - tsz.cy;
				        p.Text(x - tsz.cx, yy, Format("%.1f", l.bottom - l.top), fnt)
				         .Text(x + width + 10, yy, Format("%.1f", r.bottom - r.top), fnt)
				         .Text(x, y, Format("%.1f degrees",
				               atan2((l.bottom + l.top) / 2 - (r.bottom + r.top) / 2, width) * 360 / M_2PI),
				               fnt)
				         .Fill(Blue());
					}
				}
			}
			else {
				show[0] = true;
				show[1] = false;
			}
			
			for(int r = 0; r < 1 + IsTapered(); r++) {
				if(show[r]) {
					p.Begin();
					p.Translate(org.x, org.y);
					p.Translate(origin);
					p.Scale(scale, -scale);
		
					PaintPath(p, path[r], scale,
					          show_wire ? r ? Magenta() : Red() : Null, false,
					          show_kerf ? Blend(White(), r ? LtMagenta() : LtRed(), 120) : Null, GetKerf(r));
			
					if(show_shape)
						PaintPath(p, shape[r], scale, r ? Magenta() : Red());
					
					if(IsTapered() && IsOk(cnc[r]) && show_cnc)
						PaintPath(p, cnc[r], scale, r ? LtMagenta() : LtRed(), true);
					
					p.End();
				}
				if(!IsTapered())
					break;
			}
		}
		p.End();

		view.img = p;
		view.Refresh();
	}
	
	invert_y.NullText(AsString(GetFoamHeight() / 2));
	
	SetBar();
}

void FourAxisDlg::PaintPath(Painter& p, const Vector<Pt>& path, double scale,
                            Color color, bool dashed, Color kerf_color, double kerf)
{
	p.Move(0, 0);

	for(auto pt : path)
		p.Line(pt);

	if(!IsNull(kerf_color))
		p.LineCap(LINECAP_ROUND).LineJoin(LINEJOIN_ROUND)
		 .Stroke(kerf, kerf_color);

	if(!IsNull(color)) {
		if(dashed) {
			Vector<double> dash;
			dash.Add(1 / scale);
			p.Dash(dash, 0);
		}
		p.Stroke(1 / scale, color);
		
		if(show_points) {
			for(auto pt : path)
				p.Move(pt + Pt(-3, -3) / scale).Line(pt + Pt(3, 3) / scale)
				 .Move(pt + Pt(-3, 3) / scale).Line(pt + Pt(3, -3) / scale)
				 .Stroke(1.0 / scale, Black());
		}

		PaintArrows(p, path, scale);
	}
	
	p.EndPath();
}

void FourAxisDlg::PaintArrows(Painter& p, const Vector<Pt>& path, double scale)
{
	if(!show_arrows)
		return;
	
	double len = PathLength(path);
	
	int n = int(len / 30 * scale);

	int segment = -1;
	int sgi = 0;
	static Color c[] = { Blue(), Green(), Magenta(), Gray() };
	for(int i = 0; i < n; i++) {
		Pt dir;
		Pt f = AtPath(path, i * len / n, &dir);
		p.Begin();
		p.Translate(f);
		p.Rotate(Bearing(dir));
		p.Move(0, -4.0 / scale).Line(4.0 / scale, 0).Line(0, 4.0 / scale).Fill(c[f.segment % __countof(c)]);
		p.End();
	}
}
