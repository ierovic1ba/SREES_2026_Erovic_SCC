#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include <td/String.h>
#include <vector>
#include <cmath>

#include "ShortCircuitSolver.h"

//
// A printable one-page (tall) short-circuit report drawn on a Canvas. Shows the
// case/parameters, the analysed-fault summary, the per-bus fault-level table and
// the generator-contribution table. Exported to PDF/SVG via the "Izvezi sliku"
// action (Canvas::exportToPDF/SVG over the full model).
//
class ReportCanvas : public gui::Canvas
{
	td::String _caseFile;
	double _baseMVA = 100.0, _xd = 0.0608, _zf = 0.0;
	int    _selBusId = 0;
	FaultResult _sel;
	std::vector<FaultResult>     _perBus;
	std::vector<GenContribution> _gens;
	double _maxSk = 0.0;
	int    _maxSkBus = 0;
	double _modelHeight = 600.0;
	bool   _hasData = false;

	static void cellL(const td::String& s, double xl, double y,
	                  gui::Font::ID f = gui::Font::ID::SystemNormal)
	{
		gui::DrawableString::draw(s, gui::Rect(xl, y, xl + 740, y + 17), f, td::ColorID::SysText,
			td::TextAlignment::Left, td::VAlignment::Center, td::TextEllipsize::None);
	}
	static void cellR(const td::String& s, double xr, double y,
	                  gui::Font::ID f = gui::Font::ID::SystemNormal)
	{
		gui::DrawableString::draw(s, gui::Rect(xr - 150, y, xr, y + 17), f, td::ColorID::SysText,
			td::TextAlignment::Right, td::VAlignment::Center, td::TextEllipsize::None);
	}

public:
	ReportCanvas() : gui::Canvas() {}

	void setReport(const td::String& caseFile, double baseMVA, double xd, double zf,
	               int selBusId, const FaultResult& sel,
	               const std::vector<FaultResult>& perBus,
	               const std::vector<GenContribution>& gens,
	               double maxSk, int maxSkBus)
	{
		_caseFile = caseFile; _baseMVA = baseMVA; _xd = xd; _zf = zf;
		_selBusId = selBusId; _sel = sel; _perBus = perBus; _gens = gens;
		_maxSk = maxSk; _maxSkBus = maxSkBus;
		_modelHeight = 250.0 + (double)(perBus.size() + 2) * 18.0
		             + 70.0 + (double)(gens.size() + 2) * 18.0 + 60.0;
		_hasData = true;
		handleModelSizeChanged();
		reDraw();
	}

protected:
	bool getModelSize(gui::Size& modelSize) const override
	{
		modelSize.width = 820;
		modelSize.height = (gui::CoordType)_modelHeight;
		return true;
	}

	void onDraw(const gui::Rect&) override
	{
		if (!_hasData)
		{
			gui::DrawableString::draw(td::String("Izvjestaj: ucitaj MATPOWER .m i klikni 'Izracunaj'."),
				gui::Rect(20, 20, 780, 44), gui::Font::ID::SystemBold, td::ColorID::SysText,
				td::TextAlignment::Left);
			return;
		}

		double x = 40.0, y = 24.0;
		td::String s;

		gui::DrawableString::draw(td::String("Proracun kratkih spojeva - simetricni tropolni kvar"),
			gui::Rect(x, y, 780, y + 26), gui::Font::ID::SystemLargestBold, td::ColorID::SysText,
			td::TextAlignment::Left, td::VAlignment::Center, td::TextEllipsize::None);
		y += 38.0;

		s.format("MATPOWER slucaj: %s", _caseFile.c_str());                     cellL(s, x, y); y += 18.0;
		s.format("baseMVA = %.1f     Xd' = %.4f pu     Zf = %.4f pu", _baseMVA, _xd, _zf); cellL(s, x, y); y += 18.0;
		s.format("Analizirana sabirnica kvara: %d", _selBusId);                 cellL(s, x, y, gui::Font::ID::SystemBold); y += 24.0;

		gui::Shape::drawLine(gui::Point(x, y), gui::Point(780, y), td::ColorID::SysText, 1); y += 12.0;

		s.format("Struja kvara  If = %.3f pu = %.3f kA", std::abs(_sel._Ifpu), _sel._IfkA);
		cellL(s, x, y, gui::Font::ID::SystemBold); y += 18.0;
		s.format("Snaga kvara  Sk = %.1f MVA      Zkk = %.4f %+.4fj pu", _sel._faultMVA, _sel._Zkk.real(), _sel._Zkk.imag());
		cellL(s, x, y); y += 18.0;
		s.format("Najveca snaga kvara u mrezi: Sk = %.1f MVA (sabirnica %d)", _maxSk, _maxSkBus);
		cellL(s, x, y); y += 30.0;

		// --- per-bus fault levels ---
		cellL(td::String("Nivoi kvara po sabirnicama"), x, y, gui::Font::ID::SystemBold); y += 22.0;
		cellL(td::String("Sabirnica"), x + 8, y, gui::Font::ID::SystemBold);
		cellR(td::String("|If| (pu)"), 330, y, gui::Font::ID::SystemBold);
		cellR(td::String("If (kA)"),   450, y, gui::Font::ID::SystemBold);
		cellR(td::String("Sk (MVA)"),  600, y, gui::Font::ID::SystemBold); y += 17.0;
		gui::Shape::drawLine(gui::Point(x, y), gui::Point(600, y), td::ColorID::SysText, 1); y += 4.0;
		for (const FaultResult& r : _perBus)
		{
			s.format("%d", r._faultBusId);         cellL(s, x + 8, y);
			s.format("%.3f", std::abs(r._Ifpu));   cellR(s, 330, y);
			s.format("%.3f", r._IfkA);             cellR(s, 450, y);
			s.format("%.1f", r._faultMVA);         cellR(s, 600, y);
			y += 18.0;
		}
		y += 24.0;

		// --- generator contributions ---
		cellL(td::String("Doprinos generatora"), x, y, gui::Font::ID::SystemBold); y += 22.0;
		cellL(td::String("Generator (sab.)"), x + 8, y, gui::Font::ID::SystemBold);
		cellR(td::String("|I| (pu)"),    330, y, gui::Font::ID::SystemBold);
		cellR(td::String("|I| (kA)"),    450, y, gui::Font::ID::SystemBold);
		cellR(td::String("% doprinosa"), 600, y, gui::Font::ID::SystemBold); y += 17.0;
		gui::Shape::drawLine(gui::Point(x, y), gui::Point(600, y), td::ColorID::SysText, 1); y += 4.0;
		for (const GenContribution& g : _gens)
		{
			s.format("%d", g._bus);        cellL(s, x + 8, y);
			s.format("%.3f", g._Ipu);      cellR(s, 330, y);
			s.format("%.3f", g._IkA);      cellR(s, 450, y);
			s.format("%.1f %%", g._percent); cellR(s, 600, y);
			y += 18.0;
		}
		y += 30.0;

		cellL(td::String("SREES 2026  -  Ilhan Erovic  -  Proracun kratkih spojeva (simetricne mreze)"),
			x, y, gui::Font::ID::SystemNormal);
	}
};
