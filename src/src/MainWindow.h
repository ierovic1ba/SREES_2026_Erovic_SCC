#pragma once
#include <gui/Window.h>
#include "MenuBar.h"
#include "ToolBar.h"
#include "StatusBar.h"
#include "MainView.h"

class MainWindow : public gui::Window
{
protected:
	MenuBar _mainMenuBar;
	ToolBar _toolBar;
	StatusBar _statusBar;
	MainView _mainView;
public:
	MainWindow()
		: gui::Window(gui::Size(1100, 820))
	{
		setTitle(tr("appTitle"));
		_mainMenuBar.setAsMain(this);
		setToolBar(_toolBar);
		setCentralView(&_mainView);
		setStatusBar(_statusBar);
		_mainView.setStatusBar(&_statusBar);
		forwardMessagesTo(&_mainView); // route menu/toolbar messages to the view
	}

protected:
	// without this, the window will not close (default shouldClose() returns false)
	bool shouldClose() override { return true; }
	void onClose() override { gui::Window::onClose(); }
};
