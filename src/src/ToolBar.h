#pragma once
#include <gui/ToolBar.h>
#include <gui/Image.h>
#include <gui/Symbol.h>

class ToolBar : public gui::ToolBar
{
protected:
	gui::Image _imgOpen;
	gui::Image _imgCompute;
	gui::Image _imgSaveCsv;
	gui::Image _imgExportImg;
public:
	ToolBar()
		: gui::ToolBar("mainTB", 4)
		, _imgOpen(":imgOpen")
		, _imgCompute(":imgCompute")
		, _imgSaveCsv(":imgSaveCsv")
		, _imgExportImg(":imgExportImg")
	{
		// addItem(label, image, tooltip, menuID, firstSubID, lastSubID, actionID)
		addItem(tr("open"),      &_imgOpen,      tr("openTT"),      20, 0, 0, 10);
		addItem(tr("compute"),   &_imgCompute,   tr("computeTT"),   20, 0, 0, 30);
		addItem(tr("exportCsv"), &_imgSaveCsv,   tr("exportCsvTT"), 20, 0, 0, 40);
		addItem(tr("exportImg"), &_imgExportImg, tr("exportImgTT"), 20, 0, 0, 50);
	}
};
