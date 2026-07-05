#pragma once
#include <gui/View.h>
#include <gui/TableEdit.h>
#include <gui/VerticalLayout.h>
#include <dp/IDataSet.h>

//
// A tab page that hosts a single TableEdit filling the whole view, plus the
// connectionless data set that feeds it. The owning view configures columns and
// fills rows through the public _table / _pDS members.
//
class TablePage : public gui::View
{
	gui::VerticalLayout _vl;
public:
	gui::TableEdit _table;
	dp::IDataSet*  _pDS = nullptr;

	TablePage()
		: _vl(1)
		, _table(td::Ownership::Extern, gui::TableEdit::RowNumberVisibility::VisibleOneBased)
	{
		_vl << _table;
		setLayout(&_vl);
	}

	~TablePage()
	{
		_table.cleanAndDetachDataSet();
		if (_pDS)
			_pDS->release();
	}
};
