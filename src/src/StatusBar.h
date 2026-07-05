#pragma once
#include <gui/StatusBar.h>
#include <gui/Label.h>
#include <gui/Font.h>
#include <td/String.h>

//
// Window status bar: a left-aligned message and a right-aligned bold summary
// (e.g. "Max Sk: 2061.8 MVA @ sabirnica 2").
//
class StatusBar : public gui::StatusBar
{
	gui::Label _lblMsg;
	gui::Label _lblSummary;
public:
	StatusBar()
		: gui::StatusBar(3)
		, _lblMsg(tr("welcome"))
		, _lblSummary("")
	{
		_lblSummary.setFont(gui::Font::ID::SystemBold);
		setMargins(6, 0, 6, 4);
		_layout << _lblMsg;
		_layout.appendSpacer();
		_layout << _lblSummary;
		setLayout(&_layout);
	}

	void setMessage(const td::String& s) { _lblMsg.setTitle(s); }
	void setSummary(const td::String& s) { _lblSummary.setTitle(s); }
};
