#include "Hot4d.h"

struct GCode {
	Stream&  out;
	Pt       lpos, rpos;
	int      speed;
	String   nx, ny, nz, na;
	
	void Put(const String& s) { out.PutLine(s); }

	void To(Pt l, Pt r);
	void To(Pt p)             { To(p, p); }
	
	GCode(Stream& out, int speed) : out(out), lpos(0, 0), rpos(0, 0), speed(speed) {};
};

String GFormat(double x)
{
#ifdef _DEBUG
	return Format("%.4f", x);
#else
	return AsString(x);
#endif
}

void GCode::To(Pt l, Pt r)
{
	Pt ld = l - lpos;
	Pt rd = r - rpos;
	if(ld.x || ld.y || rd.x || rd.y) {
		String lx = GFormat(ld.x);
		String ly = GFormat(ld.y);
		String rx = GFormat(rd.x);
		String ry = GFormat(rd.y);
		Put(String() << "G1" << nx << lx << ny << ly << nz << rx << na << ry << "F" << speed);
	}
	lpos = l;
	rpos = r;
}

const char *begin_source_tag = ";<<<source>>>";
const char *end_source_tag = ";>>>source<<<";

void FourAxisDlg::SaveGCode(Stream& out, double inverted, bool mirrored)
{
	MakePaths(inverted, mirrored);

	GCode gcode(out, int(speed));
	
	gcode.nx = settings.x;
	gcode.ny = settings.y;
	gcode.nz = settings.z;
	gcode.na = settings.a;
	
	gcode.Put("G21");
	gcode.Put("G17");
	gcode.Put("G91");

	for(int i = 0; i < cnc[0].GetCount(); i++)
		gcode.To(cnc[0][i], cnc[1][i]);
}

String FourAxisDlg::Save0(const char *path)
{
	FileOut out(path);
	
	if(!out)
		return String().Cat() << "Unable to open \1" << path;

	SaveGCode(out, Null, false);
	out.PutLine("");
	String s = Base64Encode(MakeSave());
	out.PutLine(begin_source_tag);
	while(s.GetCount()) {
		int n = min(s.GetCount(), 78);
		out.PutLine(';' + s.Mid(0, n));
		s.Remove(0, n);
	}
	out.PutLine(end_source_tag);
	out.PutLine("");

	out.Close();
	if(out.IsError())
		return String().Cat() << "Error writing \1" << path;
		
	if(save_inverted) {
		String ipath = path;
		String ext = GetFileExt(ipath);
		ipath.Trim(ipath.GetCount() - ext.GetCount());
		ipath << ".inverted" << ext;
		FileOut iout(ipath);
		if(!iout)
			return String().Cat() << "Unable to open \1" << ipath;
		SaveGCode(iout, GetInvertY(), false);
		iout.Close();
		if(iout.IsError())
			return String().Cat() << "Error writing \1" << ipath;
	}

	if(save_mirrored) {
		String ipath = path;
		String ext = GetFileExt(ipath);
		ipath.Trim(ipath.GetCount() - ext.GetCount());
		ipath << ".mirrored" << ext;
		FileOut iout(ipath);
		if(!iout)
			return String().Cat() << "Unable to open \1" << ipath;
		SaveGCode(iout, Null, true);
		iout.Close();
		if(iout.IsError())
			return String().Cat() << "Error writing \1" << ipath;
	}
	
	return Null;
}

bool FourAxisDlg::Save(const char *path)
{
	if(!Accept())
		return false;

	String e = Save0(path);
	if(IsNull(e))
		return true;

	Exclamation(e);
	return false;
}

String FourAxisDlg::MakeSave()
{
	ValueMap m;
	m("type", CurrentShape().GetId())
	 ("data", CurrentShape().Save())
	 ("negative_kerf", (bool)~negative_kerf)
	 ("save_inverted", (bool)~save_inverted)
	 ("save_mirrored", (bool)~save_mirrored)
	 ("material", ~material)
	;
	if(save_inverted)
		m("invert_y", ~invert_y);
	if(IsTapered())
		m("taper", CurrentShape(true).Save())
		 ("panel_width", (double)~panel_width)
		;
	return AsJSON(m);
}

void FourAxisDlg::StoreRevision()
{
	revision = FastCompress(MakeSave());
}

bool FourAxisDlg::Load(const char *path)
{
	FileIn in(path);
	if(!in)
		return false;
	String src;
	while(!in.IsEof()) {
		String s = in.GetLine();
		if(s == begin_source_tag) {
			while(!in.IsEof()) {
				String l = in.GetLine();
				if(l == end_source_tag)
					break;
				src.Cat(TrimBoth(l.Mid(1)));
			}
			break;
		}
	}
	try {
		Value m = ParseJSON(Base64Decode(src));
		int q = shapes.Find(m["type"]);
		if(q < 0)
			return false;
		filepath = path;
		type <<= q;
		Type();
		CurrentShape().Load(m["data"]);
		negative_kerf <<= m["negative_kerf"];
		save_inverted <<= m["save_inverted"];
		save_mirrored <<= m["save_mirrored"];
		material <<= m["material"];
		invert_y <<= m["invert_y"];
		Value h = m["taper"];
		if(!IsNull(h) && CurrentShape().IsTaperable()) {
			tapered <<= true;
			Type();
			CurrentShape(true).Load(h);
			panel_width <<= m["panel_width"];
		}
	}
	catch(ValueTypeError) {
		return false;
	}
	lrufile.NewEntry(filepath);
	lrufile.Limit(16);
	Sync();
	StoreRevision();
	return true;
}

bool FourAxisDlg::OpenS(const String& fp)
{
	if(Load(fp))
		return true;
	Exclamation("Opening [* \1" + fp + "\1] has failed!");
	return false;
}

/*
void FourAxisDlg::OpenFile(const String& fp)
{
	if(filepath.GetCount())
		NewInstance(fp);
	else
		OpenS(fp);
}
*/

void FourAxisDlg::NewInstance(const String& path)
{
	LocalProcess p;
#ifdef PLATFORM_POSIX
	p.DoubleFork();
#endif
	p.Start(Merge(" ", GetExeFilePath(), path));
	p.Detach();
}

void FourAxisDlg::Open()
{
	String fp = SelectFileOpen("*.nc");
	if(fp.GetCount())
		OpenFile(fp);
}

void FourAxisDlg::OpenFile(const String& fp)
{
	if(filepath.GetCount()) {
	#if PLATFORM_WIN32
		String s = Merge(" ", GetExeFilePath() + " " + fp);
		Buffer<char> cmd(s.GetCount() + 1);
		memcpy(cmd, s, s.GetCount() + 1);
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		if(CreateProcess(NULL, cmd, &sa, &sa, TRUE,
			             NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE,
		                 NULL, NULL, &si, &pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	#else
		LocalProcess p;
		p.DoubleFork();
		p.Start(GetExeFilePath() + " " + fp);
		p.Detach();
	#endif
		return;
	}
	OpenS(fp);
}

bool FourAxisDlg::Save()
{
	if(IsNull(filepath))
		return SaveAs();
	return Save(filepath);
}

bool FourAxisDlg::SaveAs()
{
	String p = SelectFileSaveAs("*.nc");
	if(p.GetCount() && Save(p)) {
		filepath = p;
		lrufile.NewEntry(filepath);
		Sync();
		return true;
	}
	return false;
}
