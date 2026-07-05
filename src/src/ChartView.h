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
public:
	ChartView() : gui::Canvas() {}

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

		double maxV = 1e-12;
		for (const auto& d : data)
			maxV = std::max(maxV, d.second);
		if (voltageMode)
			maxV = std::max(maxV, 1.05);

		double mL = x0 + 48, mR = x1 - 10, mT = y0 + 26, mB = y1 - 22;
		double W = mR - mL, H = mB - mT;
		if (W <= 0 || H <= 0)
			return;

		gui::Shape::drawLine(gui::Point(mL, mT), gui::Point(mL, mB), td::ColorID::SysText, 1);
		gui::Shape::drawLine(gui::Point(mL, mB), gui::Point(mR, mB), td::ColorID::SysText, 1);

		if (voltageMode)
		{
			double y1pu = mB - H * (1.0 / maxV);
			gui::Shape::drawLine(gui::Point(mL, y1pu), gui::Point(mR, y1pu),
				td::ColorID::DarkOrange, 1, td::LinePattern::Dash);
		}

		double bw = W / (double)data.size();
		for (size_t i = 0; i < data.size(); ++i)
		{
			double val = data[i].second;
			double h = H * (val / maxV);
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

			if (data.size() <= 40)
			{
				td::String lbl;
				lbl.format("%d", data[i].first);
				gui::DrawableString::draw(lbl, gui::Rect(mL + i * bw, mB + 2, mL + (i + 1) * bw, mB + 20),
					gui::Font::ID::SystemNormal, td::ColorID::SysText, td::TextAlignment::Center);
			}
		}

		td::String maxLbl;
		maxLbl.format(voltageMode ? "%.2f" : "%.0f", maxV);
		gui::DrawableString::draw(maxLbl, gui::Rect(x0, mT - 2, mL - 3, mT + 16),
			gui::Font::ID::SystemNormal, td::ColorID::SysText, td::TextAlignment::Right);
	}

	void onDraw(const gui::Rect&) override
	{
		gui::Size sz;
		getSize(sz);
		double midY = sz.height / 2.0;

		td::String t1("Snaga kvara Sk [MVA] po sabirnicama");
		td::String t2;
		t2.format("Napon |V| [pu] pri kvaru na sabirnici %d", _faultBusId);

		drawBarChart(8, 6, sz.width - 8, midY - 4, t1, _fault, _highlight, false);
		drawBarChart(8, midY + 4, sz.width - 8, sz.height - 6, t2, _volt, -1, true);
	}
};
