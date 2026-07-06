#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include <td/String.h>
#include <vector>
#include <algorithm>
#include <utility>

//
// Canvas that draws two bar charts:
//   - top:    short-circuit power Sk [MVA] per bus (worst bus in dark red)
//   - bottom: post-fault voltage |V| [pu] per bus, with a 1.0 pu reference line
//             and colour coding for voltage sag.
//
class ChartView : public gui::Canvas
{
	std::vector<std::pair<int, double>> _fault; // (busId, Sk MVA)
	std::vector<std::pair<int, double>> _volt;  // (busId, |V| pu)
	int _highlight  = -1;
	int _faultBusId = 0;
	gui::Size _sz; // last known on-screen size (fallback for export contexts)
public:
	ChartView() : gui::Canvas() { _sz.width = 980; _sz.height = 620; }

	void setData(const std::vector<std::pair<int, double>>& fault,
	             const std::vector<std::pair<int, double>>& volt,
	             int highlight, int faultBusId)
	{
		_fault = fault;
		_volt = volt;
		_highlight = highlight;
		_faultBusId = faultBusId;
		reDraw();
	}

protected:
	void drawBarChart(double x0, double y0, double x1, double y1,
	                  const td::String& title,
	                  const std::vector<std::pair<int, double>>& data,
	                  int highlight, bool voltageMode)
	{
		gui::DrawableString::draw(title, gui::Rect(x0, y0, x1, y0 + 18),
			gui::Font::ID::SystemBold, td::ColorID::SysText, td::TextAlignment::Center);
		if (data.empty())
			return;

		double dataMax = 1e-12;
		for (const auto& d : data)
			dataMax = std::max(dataMax, d.second);
		if (voltageMode)
			dataMax = std::max(dataMax, 1.05);
		double axisMax = dataMax * 1.15; // headroom so the tallest bar never touches the top edge

		double mL = x0 + 48, mR = x1 - 10, mT = y0 + 26, mB = y1 - 22;
		double W = mR - mL, H = mB - mT;
		if (W <= 0 || H <= 0)
			return;

		gui::Shape::drawLine(gui::Point(mL, mT), gui::Point(mL, mB), td::ColorID::SysText, 1);
		gui::Shape::drawLine(gui::Point(mL, mB), gui::Point(mR, mB), td::ColorID::SysText, 1);

		if (voltageMode)
		{
			double y1pu = mB - H * (1.0 / axisMax);
			gui::Shape::drawLine(gui::Point(mL, y1pu), gui::Point(mR, y1pu),
				td::ColorID::DarkOrange, 1, td::LinePattern::Dash);
		}

		double bw = W / (double)data.size();
		bool valueLabels = (data.size() <= 20); // show exact value on each bar for small cases
		for (size_t i = 0; i < data.size(); ++i)
		{
			double val = data[i].second;
			double h = H * (val / axisMax);
			gui::Rect r(mL + i * bw + 0.15 * bw, mB - h, mL + (i + 1) * bw - 0.15 * bw, mB);

			td::ColorID col;
			if ((int)i == highlight)
				col = td::ColorID::DarkRed;
			else if (voltageMode)
				col = (val < 0.7) ? td::ColorID::DarkRed
				    : (val < 0.9) ? td::ColorID::DarkOrange : td::ColorID::DarkGreen;
			else
				col = td::ColorID::DarkCyan;
			gui::Shape::drawRect(r, col);

			if (valueLabels)
			{
				td::String vlbl;
				vlbl.format(voltageMode ? "%.3f" : "%.0f", val);
				gui::DrawableString::draw(vlbl, gui::Rect(mL + i * bw, mB - h - 17, mL + (i + 1) * bw, mB - h - 1),
					gui::Font::ID::SystemNormal, td::ColorID::SysText,
					td::TextAlignment::Center, td::VAlignment::Center, td::TextEllipsize::None);
			}

			if (data.size() <= 40)
			{
				td::String lbl;
				lbl.format("%d", data[i].first);
				gui::DrawableString::draw(lbl, gui::Rect(mL + i * bw, mB + 2, mL + (i + 1) * bw, mB + 20),
					gui::Font::ID::SystemNormal, td::ColorID::SysText, td::TextAlignment::Center);
			}
		}

		// y-axis reference label = actual data maximum, at the top of the tallest bar
		td::String maxLbl;
		maxLbl.format(voltageMode ? "%.2f" : "%.0f", dataMax);
		double yTop = mB - H * (dataMax / axisMax);
		gui::DrawableString::draw(maxLbl, gui::Rect(x0, yTop - 9, mL - 3, yTop + 9),
			gui::Font::ID::SystemNormal, td::ColorID::SysText, td::TextAlignment::Right);
	}

	// required so Canvas::exportToPDF/SVG works (uses the cached on-screen size)
	bool getModelSize(gui::Size& sz) const override { sz = _sz; return true; }

	void onDraw(const gui::Rect&) override
	{
		gui::Size sz;
		getSize(sz);
		if (sz.width > 20 && sz.height > 20) _sz = sz; // cache live size
		else sz = _sz;                                 // export / not-yet-realized
		double midY = sz.height / 2.0;

		td::String t1("Snaga kvara Sk [MVA] po sabirnicama");
		td::String t2;
		t2.format("Napon |V| [pu] pri kvaru na sabirnici %d", _faultBusId);

		drawBarChart(8, 6, sz.width - 8, midY - 4, t1, _fault, _highlight, false);
		drawBarChart(8, midY + 4, sz.width - 8, sz.height - 6, t2, _volt, -1, true);
	}
};
