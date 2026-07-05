#pragma once
#include <gui/MenuBar.h>

class MenuBar : public gui::MenuBar
{
private:
	gui::SubMenu _subApp;
	gui::SubMenu _subModel;
protected:
	void populateSubAppMenu()
	{
		auto& items = _subApp.getItems();
		items[0].initAsQuitAppActionItem(tr("Quit"), "q"); // translated inside natGUI
	}

	void populateSubModelMenu()
	{
		auto& items = _subModel.getItems();
		items[0].initAsActionItem(tr("open"), 10, "o");
	}
public:
	MenuBar()
		: gui::MenuBar(2)          // two menus
		, _subApp(10, "App", 1)    // App menu, 1 item
		, _subModel(20, "Model", 1) // Model menu, 1 item
	{
		populateSubAppMenu();
		populateSubModelMenu();
		setMenu(0, &_subApp);
		setMenu(1, &_subModel);
	}
};
