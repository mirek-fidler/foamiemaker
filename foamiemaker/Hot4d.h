#ifndef _FourAxis_Hot4d_h_
#define _FourAxis_Hot4d_h_

#include <CtrlLib/CtrlLib.h>
#include <Painter/Painter.h>

using namespace Upp;

#define LAYOUTFILE <FoamieMaker/dlg.lay>
#include <CtrlCore/lay.h>

#define IMAGECLASS HotImg
#define IMAGEFILE <FoamieMaker/HotImg.iml>
#include <Draw/iml_header.h>

struct Pt : Moveable<Pt, Pointf> {
	bool   kerf = 0; // line is kerf compensated
	bool   sharp = false; // kerf angle is sharp (e.g. in rectangular spar)
	int    segment = 0; // tapering segments
	bool   mainshape = false;
	
	String ToString() const;

	void SetAttr(const Pt& b){ kerf = b.kerf; segment = b.segment; mainshape = b.mainshape; sharp = b.sharp; }
	Pt Attr(const Pt& b)     { Pt p = *this; p.SetAttr(b); return p; }
	Pt Segment(int n)        { Pt p = *this; p.segment = n; return p; }
	
	Pt(double x_, double y_) { x = x_, y = y_; }
	Pt(const Pointf& a)      { x = a.x; y = a.y; }
	Pt(const Point& a)       { x = a.x; y = a.y; }
	Pt(Sizef& a)             { x = a.cx; y = a.cy; }
	Pt(const Nuller& n)      { x = y = Null; }
	Pt() {}
};

VectorMap<String, Ctrl *> GetCtrlMap(Ctrl& parent);
void SetValues(Ctrl& parent, const ValueMap& json);
ValueMap GetValues(Ctrl& parent);

Pt MakePoint(Ctrl& a, Ctrl& b);

double LineIntersectionT(Pt a1, Pt a2, Pt b1, Pt b2);
Pt LineIntersection(Pt a1, Pt a2, Pt b1, Pt b2);
int    LineCircleIntersections(Pt c, double radius, Pt p1, Pt p2, double& t1, double& t2, double def = Null);
double PathLength(const Vector<Pt>& path, int from, int count);
double PathLength(const Vector<Pt>& path);
Pt AtPath(const Vector<Pt>& path, double at, Pt *dir1 = 0, int from = 0);
bool IsOk(const Vector<Pt>& path);

inline
Pt SegmentIntersection(Pt a1, Pt a2, Pt b1, Pt b2)
{
	if(Rect(a1, a2).Intersects(Rect(b1, b2)) || 1) {
		double t = LineIntersectionT(a1, a2, b1, b2);
		if(t >= 0 && t <= 1) {
			double t2 = LineIntersectionT(b1, b2, a1, a2);
			if(t2 >= 0 && t2 <= 1)
				return t * (a2 - a1) + a1;
		}
	}
	return Null;
}

bool PathIntersection(Pointf a, Pointf b, const Vector<Pt>& path, int& i, Pt& pt);

void Mix(const Vector<Pt>& left, int li, int lcount,
         const Vector<Pt>& right, int ri, int rcount,
         Vector<Pt>& left_out, Vector<Pt>& right_out);

void MixAll(const Vector<Pt>& left, const Vector<Pt>& right,
            Vector<Pt>& left_out, Vector<Pt>& right_out);

void CncPath(Vector<Pt>& left, Vector<Pt>& right, double width, double tower_distance, double left_distance);

Rectf GetBounds(const Vector<Pt>& path);

Vector<Pt> KerfCompensation(const Vector<Pt>& in0, double kerf);

struct Path : Vector<Pt> {
	Xform2D     transform;
	int         segment = Null; // used to split path for tapering
	int         segment_counter = 0;
	bool        mainshape = false;
	
	void To(Pt p, bool kerf = false);
	void To(double x, double y, bool kerf = false)   { To(Pt(x, y), kerf); }
	void Kerf(Pt p)                                  { To(p, true); }
	void Kerf(double x, double y)                    { To(x, y, true); }
	void Sharp(Pt p)                                 { Sharp(p.x, p.y); }
	void Sharp(double x, double y);
	void NewSegment()                                { segment_counter++; segment = segment_counter; }
	void SetSegment(int si)                          { segment = si; }
	void MainShape()                                 { mainshape = true; }
	void EndMainShape()                              { mainshape = false; }
	
	void Rotate(double x, double y, double angle);
	void Offset(double x, double y);
	void Identity()                                  { transform = Xform2D::Identity(); }
};

enum { BOTTOM_SPAR, TOP_SPAR, RIGHT_SPAR };
enum { RECTANGLE_SPAR, CIRCLE_SPAR };

struct Spar : Moveable<Spar> {
	int    kind;
	double pos;
	int    shape;
	Sizef  dimension;
	double depth;
	double angle = Null;
};

struct SparsCtrl : WithSparsLayout<ParentCtrl> {
	virtual Value GetData() const;
	virtual void  SetData(const Value& data);
	
	void Sync();
	
	Vector<Spar> Get(Pointf origin = Pointf(0, 0), Sizef dir = Pointf(1, 1)) const;

	SparsCtrl();
};

void Circle(Path& path, Pt c, double r, double a0);
void CutSpar(Path& r, const Vector<Pt>& foil, int& i, Pt pos, const Spar& spar, Point dir);
// void CutSpar(Path& r, const Vector<Pt>& foil, int& i, Pt pos, Pt dim, bool circle, Point dir, double angle = Null);
bool DoSpars(Path& r, const Vector<Pt>& foil, int& i, Vector<Spar>& spars);
void ReadSpar(Vector<Spar>& spar, int kind, double le, Ctrl& pos, Ctrl& cx, Ctrl& cy, Ctrl& circle);

enum {
	TAPERABLE = 1, INVERTABLE = 2,
};

struct Shape : ParentCtrl {
	virtual Path     Get() = 0;
	virtual void     Load(const ValueMap& json);
	virtual ValueMap Save();
	virtual String   GetId() const = 0;
	virtual String   GetName() const = 0;
	virtual void     AddPoint(Pt& p) {}
	virtual dword    GetInfo() const { return 0; }
	
	bool    IsTaperable() const { return GetInfo() & TAPERABLE; }
	bool    IsInvertable() const { return GetInfo() & INVERTABLE; }
	
	void SyncView();
};

struct AirfoilCtrl : public DataPusher {
	virtual void DoAction();
	
	void Render(Path& path, double width, Pt p0, double te, bool smooth);
	
	Vector<Pt> Get();
	
	AirfoilCtrl();
};

Vector<Pt> GetHalfFoil(const Vector<Pt>& foil, bool bottomhalf = false);
void       InvertX(Vector<Pt>& foil);
void       Mul(Vector<Pt>& foil, double ax, double ay);
double     GetMaxY(const Vector<Pt>& foil, double sgny = 1);
void       CutHalfFoil(Vector<Pt>& foil, double head, double tail);

struct FourAxisDlg;

struct Rod : WithRodLayout<Shape> {
	virtual Path     Get();
	virtual String   GetId() const   { return "rod"; }
	virtual String   GetName() const { return "Rod"; }
	virtual dword    GetInfo() const { return TAPERABLE; }

	SparsCtrl   spars;

	typedef Rod CLASSNAME;

	Rod();
};

struct Text : WithTextLayout<Shape> {
	typedef Text CLASSNAME;

	virtual Path    Get();
	virtual String  GetId() const   { return "text"; }
	virtual String  GetName() const { return "Text"; }

	Text();
};

struct Angle : WithAngleLayout<Shape> {
	typedef Angle CLASSNAME;

	virtual Path    Get();
	virtual String  GetId() const   { return "angle"; }
	virtual String  GetName() const { return "Angle cut"; }

	Angle();
};

struct Wing : WithWingLayout<Shape> {
	typedef Wing CLASSNAME;
	
	AirfoilCtrl airfoil;
	SparsCtrl   spars;

	virtual Path    Get();
	virtual String  GetId() const   { return "wing"; }
	virtual String  GetName() const { return "Wing panel"; }
	virtual dword   GetInfo() const { return TAPERABLE|INVERTABLE; }
	
	FourAxisDlg *dlg;
	Wing        *other;
	bool         right;

	Wing();
};

struct FusePlan : WithFusePlanLayout<Shape> {
	typedef FusePlan CLASSNAME;
	
	AirfoilCtrl airfoil;

	virtual Path    Get();
	virtual String  GetId() const   { return "fuseplan"; }
	virtual String  GetName() const { return "Fuselage planform"; }

	virtual void     Load(const ValueMap& json);
	virtual ValueMap Save();

	FusePlan();
};

struct FuseProfile : WithFuseProfileLayout<Shape> {
	typedef FuseProfile CLASSNAME;
	
	AirfoilCtrl airfoil, saddle_airfoil;
	
	SparsCtrl spars;
	
	virtual Path    Get();
	virtual String  GetId() const   { return "fuseprofile"; }
	virtual String  GetName() const { return "Fuselage profile"; }

	FuseProfile();
};

struct Motor : WithMotorLayout<Shape> {
	typedef Motor CLASSNAME;

	virtual Path    Get();
	virtual String  GetId() const   { return "motor_mount"; }
	virtual String  GetName() const { return "Motor mount"; }

	Motor();
};

struct TextPath : WithTextPath<Shape> {
	typedef TextPath CLASSNAME;

	virtual Path    Get();
	virtual String  GetId() const   { return "free_text"; }
	virtual String  GetName() const { return "Free text path"; }
	virtual void    AddPoint(Pt& p);

	TextPath();
};

#ifdef _DEBUG
void TestKerf();
#endif

struct View : Ctrl {
	FourAxisDlg *fa = NULL;
	Image img;
	
	virtual void Paint(Draw& w);
	virtual void LeftDown(Point p, dword);
	virtual void LeftUp(Point p, dword);
	virtual void MouseMove(Point p, dword);
	virtual void MouseWheel(Point p, int zdelta, dword keyflags);
	virtual void LeftDrag(Point p, dword keyflags);
	virtual void LeftHold(Point p, dword keyflags);
	
	View();
};

struct FourAxisDlg : WithFourAxisLayout<TopWindow> {
	typedef FourAxisDlg CLASSNAME;
	
	String filepath;
	String revision;
	
	Rod      rod[2];
	Text     text[1];
	Angle    angle[1];
	Wing     wing[2];
	Motor    motor[1];
	TextPath textpath[1];
	FusePlan fuseplan[1];
	FuseProfile fuseprofile[1];
	
	View     view;
	Button   home;
	Point    org = Point(0, 0);
	bool     drag = false;
	Point    click;
	Point    click_org;
	
	LRUList lrufile;

	VectorMap<String, Tuple<Shape *, Shape *>> shape;

private:
	Shape& CurrentShape0(bool right) const;

public:
	Shape& CurrentShape(bool right = false);
	const Shape& CurrentShape(bool right = false) const;
	
	virtual void Layout();
	virtual bool Key(dword key, int count);

	void ViewDown(Point p);
	void ViewUp(Point p);
	void ViewMove(Point p);
	void ViewWheel(Point p, int zdelta);
	void ViewDrag(Point p);
	
	void   Zoom(int dir, Point p);
	void   Home()                       { org = Pt(0, 0); Sync(); }
	void   Type();
	void   Sync();
	void   PaintPath(Painter& p, const Vector<Pt>& path, double scale, Color color,
	                 bool dashed = false, Color kerf_color = Null, double kerf = 0);
	void   PaintArrows(Painter& p, const Vector<Pt>& path, double scale);
	void   ViewPars(Font& fnt, int& w, Point& origin) const;
	Point  ViewOrigin() const;
	Pt GetViewPos(Point p);
	
	bool   IsTapered() const             { return CurrentShape().IsTaperable() && tapered; }
	
	double GetFoamHeight() const         { return Nvl((double)~height, 30.0); }
	double GetInvertY() const            { return Nvl((double)~invert_y, GetFoamHeight() / 2); }
	
	void   SetBar();
	void   StoreRevision();
	void   SaveGCode(Stream& out, double inverted);
	bool   Save(const char *path);
	bool   Load(const char *path);
	bool   OpenS(const String& fp);
	void   OpenFile(const String& p);
	void   Open();
	bool   SaveAs();
	void   NewInstance(const String& path);
	bool   Save();
	void   Exit();
	
	double     GetKerf(bool right);
	void       MakePaths(Vector<Pt> *shape, Vector<Pt> *path, Vector<Pt> *cnc, double inverted = Null);
	
	String MakeSave();

	void   AddShape(Shape *l, Shape *r = NULL);
	
	void   Serialize(Stream& s);

	FourAxisDlg();
};

#endif
