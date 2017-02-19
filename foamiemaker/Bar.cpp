#include "Hot4d.h"

void FourAxisDlg::SetBar()
{
	menu.Set([=](Bar& bar) {
		bar.Sub("File",
			[=](Bar& bar) {
				bar.Add("New", [=] { NewInstance(Null); }).Key(K_CTRL_N);
				bar.Add("Open..", [=] { Open(); }).Key(K_CTRL_O);
				bar.Add("Save", [=] { Save(); }).Key(K_CTRL_S);
				bar.Add("Save as..", [=] { SaveAs(); }).Key(K_CTRL_S|K_SHIFT);
				if(lrufile.GetCount())
					lrufile(bar, THISFN(OpenFile));
				bar.Separator();
				bar.Add("Exit", [=] { Exit(); }).Key(K_ALT_F4);
		});
	});
}
