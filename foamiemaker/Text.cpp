#include "Hot4d.h"

struct HGlyphPainter : NilPainter, LinearPathConsumer {
	Vector<float>       glyph;
	double              tolerance;
	Pointf              pos, move;
	
	virtual void LineOp(const Pointf& p, bool);
	virtual void MoveOp(const Pointf& p, bool);
	virtual void QuadraticOp(const Pointf& p1, const Pointf& p, bool);
	virtual void CubicOp(const Pointf& p1, const Pointf& p2, const Pointf& p, bool);
	virtual void CloseOp();

	virtual void Line(const Pointf& p);
	virtual void Move(const Pointf& p);
};

void HGlyphPainter::Move(const Pointf& p)
{
	glyph.Add((float)1e31);
	Line(p);
	move = pos;
}

void HGlyphPainter::Line(const Pointf& p)
{
	glyph.Add((float)p.x);
	glyph.Add((float)p.y);
	pos = p;
}

void HGlyphPainter::MoveOp(const Pointf& p, bool)
{
	Move(p);
}

void HGlyphPainter::LineOp(const Pointf& p, bool)
{
	Line(p);
}

void HGlyphPainter::QuadraticOp(const Pointf& p1, const Pointf& p, bool)
{
	ApproximateQuadratic(*this, pos, p1, p, tolerance);
	pos = p;
}

void HGlyphPainter::CubicOp(const Pointf& p1, const Pointf& p2, const Pointf& p, bool)
{
	ApproximateCubic(*this, pos, p1, p2, p, tolerance);
	pos = p;
}

void HGlyphPainter::CloseOp()
{
	if(move != pos && !IsNull(move))
		Line(move);
}

struct GlyphKey {
	Font   fnt;
	int    chr;
	double tolerance;
	
	bool operator==(const GlyphKey& b) const {
		return fnt == b.fnt && chr == b.chr && tolerance == b.tolerance;
	}
	unsigned GetHashValue() const {
		return CombineHash(fnt, chr, tolerance);
	}
};

struct sHMakeGlyph : LRUCache<Value, GlyphKey>::Maker {
	GlyphKey gk;

	GlyphKey Key() const     { return gk; }
	int      Make(Value& v) const {
		HGlyphPainter gp;
		gp.move = gp.pos = Null;
		gp.tolerance = gk.tolerance;
		PaintCharacter(gp, Pointf(0, 0), gk.chr, gk.fnt);
		int sz = gp.glyph.GetCount() * 4;
		v = RawPickToValue(pick(gp.glyph));
		return sz;
	}
};

void DoApproximateChar(Vector<Pointf>& path, Pointf pos, int ch, Font fnt, double tolerance)
{
	PAINTER_TIMING("ApproximateChar");
	Value v;
	INTERLOCKED {
		PAINTER_TIMING("ApproximateChar::Fetch");
		static LRUCache<Value, GlyphKey> cache;
		cache.Shrink(500000);
		sHMakeGlyph h;
		h.gk.fnt = fnt.Height(100 * fnt.GetHeight());
		h.gk.chr = ch;
		h.gk.tolerance = tolerance;
		v = cache.Get(h);
	}
	const Vector<float>& g = ValueTo< Vector<float> >(v);
	int i = 0;
	while(i < g.GetCount()) {
		Pointf p;
		p.x = g[i++];
		if(p.x > 1e30) {
			p.x = g[i++];
			p.y = -g[i++];
			path.Add(Null); // next point is move
			path.Add(p / 100.0 + pos);
		}
		else {
			PAINTER_TIMING("ApproximateChar::Line");
			p.y = -g[i++];
			path.Add(p / 100.0 + pos);
		}
	}
}

Path Text::Get()
{
	Path r;
	Font fnt = Font(~font, (int)~height).Bold(~bold).Italic(~italic);
	String h = ~text;
	Pointf pos = Pointf(~leadin, ~cline);
	double ly = ~line;
	r.To(0, pos.y);
	r.To(pos.x, pos.y);
	for(int i = 0; i < h.GetCount(); i++) {
		Vector<Pointf> ch;
		Pointf npos = pos;
		npos.x += fnt[h[i]] + (int)~spacing;
		r.To(pos);
		DoApproximateChar(ch, Pointf(pos.x, ly), h[i], fnt, 10);
		if(ch.GetCount() > 2) {
			int ei = FindBest(ch, [=](Pointf p1, Pointf p2) {
				return SquaredDistance(Nvl(p1, Pointf(INT_MAX, INT_MAX)), pos) <
				       SquaredDistance(Nvl(p2, Pointf(INT_MAX, INT_MAX)), pos);
			});
			bool kerf = false;
			for(int q = 0; q < ch.GetCount() + 1; q++) {
				Pointf pt = ch[(q + ei) % ch.GetCount()];
				if(IsNull(pt))
					kerf = false;
				else {
					if(kerf)
						r.Kerf(pt);
					else
						r.To(pt);
					kerf = true;
				}
			}
		}
		r.To(pos);
		pos = npos;
	}
	r.To(pos);
	r.To(0, pos.y);
	r.To(0, 0);
	return r;
}

Text::Text()
{
	CtrlLayout(*this);
	font.Add(Font::SANSSERIF, "SansSerif");
	font.Add(Font::SERIF, "Serif");
	font.Add(Font::MONOSPACE, "Monospace");
	
	font <<= Font::SERIF;
	height <<= 150;
	line <<= 180;
	cline <<= 8;
	leadin <<= 10;
	spacing <<= 0;
}
