#include "Hot4d.h"

SparsCtrl::SparsCtrl()
{
	CtrlLayout(*this);
	
	at.Add(TOP_SPAR, "Top at");
	at.Add(RIGHT_SPAR, "Right at");
	at.Add(BOTTOM_SPAR, "Bottom at");
	
	type.Add(RECTANGLE_SPAR, "Rectangle");
	type.Add(CIRCLE_SPAR, "Circle");
	
	list.AddCtrl("kind", at);
	list.AddCtrl("pos", at_pos);
	list.AddCtrl("shape", type);
	list.AddCtrl("width", width);
	list.AddCtrl("height", height);
	list.AddCtrl("angle", angle);
	list.AddCtrl("depth", depth);
	struct CvAt : Convert {
		Value Format(const Value& v) const {
			String h = String() << decode(v["kind"], TOP_SPAR, "Top", RIGHT_SPAR, "Right", "Bottom")
			                    << " at " << ~v["pos"]
			                    << decode(v["shape"], RECTANGLE_SPAR, ", □ ", CIRCLE_SPAR, ", ○ ", ", ? ")
			                    << AsString(v["width"]) << "×" << AsString(v["height"]);
			double angle = v["angle"];
			if(!IsNull(angle))
				h << ", < " << angle << "°";
			double depth = v["depth"];
			if(!IsNull(depth))
				h << ", depth " << depth;
			return h;
		}
	};
	list.AddColumnAt("kind", "Feature")
	    .Add("pos")
	    .Add("shape")
	    .Add("width")
	    .Add("height")
	    .Add("angle")
	    .Add("depth")
	    .SetConvert(Single<CvAt>());
	
	list.WhenSel = [=] {
		int c = list.GetCursor();
		remove.Enable(c >= 0);
		up.Enable(list.GetCursor() > 0);
		down.Enable(list.GetCursor() < list.GetCount() - 1);
	};
	
	for(Ctrl *q = GetFirstChild(); q; q = q->GetNext()) {
		if(q != &list && !dynamic_cast<Button *>(q)) {
			*q << [=] {
				Sync();
			};
		}
	}
	
	up.SetImage(HotImg::arrow_up());
	down.SetImage(HotImg::arrow_down());
	
	up << [=] { list.SwapUp(); };
	down << [=] { list.SwapDown(); };
	
	add << [=] {
		list.Add();
		list.GoEnd();
		at <<= TOP_SPAR;
		type <<= RECTANGLE_SPAR;
		at_pos <<= 0;
		width <<= 5;
		height <<= 3;
		Sync();
	};
	
	remove << [=] {
		list.DoRemove();
	};
}

void SparsCtrl::Sync()
{
	if(list.AcceptRow()) {
		for(Ctrl *q = GetFirstChild(); q; q = q->GetNext())
			q->ClearModify();
		WhenAction();
	}
}

Vector<Spar> SparsCtrl::Get(Pointf origin, Sizef dir) const
{
	Vector<Spar> spars;
	for(int i = 0; i < list.GetCount(); i++) {
		ValueMap m = list.GetMap(i);
		Spar& spar = spars.Add();
		spar.kind = m["kind"];
		spar.pos = m["pos"];
		spar.dimension.cx = m["width"];
		spar.dimension.cy = m["height"];
		spar.shape = m["shape"];
		spar.angle = m["angle"];
		spar.depth = m["depth"];
		if(findarg(spar.kind, TOP_SPAR, BOTTOM_SPAR) >= 0)
			spar.pos = origin.x + dir.cx * spar.pos;
		else
			spar.pos = origin.y + dir.cy * spar.pos;
		if(spar.shape == RECTANGLE_SPAR && Nvl(spar.depth) == 0)
			spar.pos += (spar.kind == TOP_SPAR ? -1 : 1) * spar.dimension.cx / 2;
	}
	return spars;
}

Value SparsCtrl::GetData() const
{
	ValueArray va;
	for(int i = 0; i < list.GetCount(); i++) {
		ValueMap m = list.GetMap(i);
		m.Set("kind", decode(m["kind"], TOP_SPAR, "top", RIGHT_SPAR, "right", "bottom"));
		m.Set("shape", decode(m["shape"], RECTANGLE_SPAR, "rectangle", "ellipse"));
		va.Add(m);
	}
	return va;
}

void SparsCtrl::SetData(const Value& data)
{
	list.Clear();
	for(int i = 0; i < data.GetCount(); i++) {
		ValueMap m = data[i];
		m.Set("kind", decode(m["kind"], "top", TOP_SPAR, "right", RIGHT_SPAR, BOTTOM_SPAR));
		m.Set("shape", decode(m["shape"], "rectangle", RECTANGLE_SPAR, CIRCLE_SPAR));
		list.AddMap(m);
	}
	list.GoBegin();
}
