#include "Hot4d.h"

struct GCode {
	Stream&  out;
	Pt       lpos, rpos;
	int      speed;
	
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
		Put(String() << "G1X" << lx << "Y" << ly << "Z" << rx << "A" << ry << "F" << speed);
	}
	lpos = l;
	rpos = r;
}

const char *begin_source_tag = ";<<<source>>>";
const char *end_source_tag = ";>>>source<<<";

void FourAxisDlg::SaveGCode(Stream& out, double inverted)
{
	GCode gcode(out, ~speed);
	
	gcode.Put("G21");
	gcode.Put("G17");
	gcode.Put("G91");

	Vector<Pt> shape[2];
	Vector<Pt> path[2];
	Vector<Pt> cnc[2];
	MakePaths(shape, path, cnc, inverted);
	for(int i = 0; i < cnc[0].GetCount(); i++)
		gcode.To(cnc[0][i], cnc[1][i]);
}

bool FourAxisDlg::Save(const char *path)
{
	if(!Accept())
		return false;

	FileOut out(path);

	if(out) {
		SaveGCode(out, Null);
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
		
		if(!out.IsError()) {
			if(save_inverted) {
				String ipath = path;
				String ext = GetFileExt(ipath);
				ipath.Trim(ipath.GetCount() - ext.GetCount());
				ipath << ".inverted" << ext;
				FileOut iout(ipath);
				SaveGCode(iout, GetInvertY());
				iout.Close();
				if(!iout.IsError())
					return true;
			}
			else
				return true;
		}
	}

	Exclamation("Error while saving the file.");
	return false;
}

String FourAxisDlg::MakeSave()
{
	ValueMap m;
	m("type", CurrentShape().GetId())
	 ("data", CurrentShape().Save())
	 ("kerf", (double)~kerf)
	 ("feed", (double)~speed)
	;
	if(IsTapered())
		m("taper", CurrentShape(true).Save())
		 ("panel_width", (double)~panel_width)
		 ("tower_distance", (double)~tower_distance)
		 ("left_gap", (double)~left_gap)
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
		int q = shape.Find(m["type"]);
		if(q < 0)
			return false;
		filepath = path;
		type <<= q;
		Type();
		CurrentShape().Load(m["data"]);
		kerf <<= m["kerf"];
		speed <<= m["feed"];
		Value h = m["taper"];
		if(!IsNull(h) && CurrentShape().IsTaperable()) {
			tapered <<= true;
			Type();
			CurrentShape(true).Load(h);
			panel_width <<= m["panel_width"];
			tower_distance <<= m["tower_distance"];
			left_gap <<= m["left_gap"];
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
