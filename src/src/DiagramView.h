#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include <td/String.h>
#include <vector>
#include <cmath>
#include <algorithm>

#include "MatpowerCase.h"
#include "ShortCircuitSolver.h"

//
// In-app single-line network diagram (circular auto-layout, works for any case).
// Buses = bars coloured by post-fault voltage (green/orange/red), faulted bus red
// with a ring; branches coloured and thickened by their fault current; numeric
// voltage/current labels for small cases; generators = blue circle; loads = stub.
//
class DiagramView : public gui::Canvas
{
	struct Node { int _busId = 0; double _v = 1.0; bool _gen = false; bool _load = false; };
	struct Edge { int _f = 0; int _t = 0; double _i = 0.0; };

	std::vector<Node> _nodes;
	std::vector<Edge> _edges;
	double _maxI      = 1.0;
	int    _faultNode = -1;
	gui::Shape _circle;
	gui::Size  _sz; // last known on-screen size (fallback for export contexts)

	static constexpr double kPi = 3.14159265358979323846;

	static td::ColorID vColor(double v)
	{
		return (v < 0.7) ? td::ColorID::DarkRed
		     : (v < 0.9) ? td::ColorID::DarkOrange : td::ColorID::DarkGreen;
	}

public:
	DiagramView() : gui::Canvas()
	{
		_circle.createCircle(gui::Circle(0, 0, 1), 1.5f);
		_sz.width = 1000; _sz.height = 680;
	}

	void setNetwork(const MatpowerCase& mpc, const FaultResult& sel,
	                const std::vector<BranchFault>& branches, int faultIdx)
	{
		_nodes.clear();
		_edges.clear();
		int n = mpc.size();

		std::vector<bool> isGen(n, false);
		for (const ScGen& g : mpc._gens)
			if (g._status >= 1) { int i = mpc.indexOf(g._bus); if (i >= 0) isGen[i] = true; }

		for (int i = 0; i < n; ++i)
		{
			Node nd;
			nd._busId = mpc._buses[i]._id;
			nd._v     = (i < (int)sel._busV.size()) ? std::abs(sel._busV[i]) : 1.0;
			nd._gen   = isGen[i];
			nd._load  = (mpc._buses[i]._Pd != 0.0 || mpc._buses[i]._Qd != 0.0);
			_nodes.push_back(nd);
		}

		double maxI = 1e-12;
		for (const BranchFault& b : branches)
		{
			int f = mpc.indexOf(b._fbus), t = mpc.indexOf(b._tbus);
			if (f < 0 || t < 0) continue;
			Edge e; e._f = f; e._t = t; e._i = b._Ipu;
			_edges.push_back(e);
			maxI = std::max(maxI, b._Ipu);
		}
		_maxI = maxI;
		_faultNode = faultIdx;
		reDraw();
	}

protected:
	void legendRow(double lx, double& ly, td::ColorID c, bool circle, const char* txt)
	{
		if (circle)
			_circle.drawWireAtPointWithScale(c, gui::Point(lx + 11, ly + 11), 6.0);
		else
			gui::Shape::drawRect(gui::Rect(lx + 4, ly + 5, lx + 20, ly + 17), c, td::ColorID::SysText, 1.0f);
		gui::DrawableString::draw(td::String(txt), gui::Rect(lx + 28, ly, lx + 236, ly + 22),
			gui::Font::ID::SystemNormal, td::ColorID::SysText,
			td::TextAlignment::Left, td::VAlignment::Center);
		ly += 22.0;
	}

	void drawLegend()
	{
		double lx = 8.0, ly = 34.0;
		double boxW = 250.0;
		double boxH = 6 * 22.0 + 26.0;
		// opaque background so the legend stays readable over branch lines
		gui::Shape::drawRect(gui::Rect(lx - 4, ly - 26, lx - 4 + boxW, ly - 26 + boxH),
			td::ColorID::SysCtrlBack, td::ColorID::SysText, 1.0f);
		gui::DrawableString::draw(td::String("Legenda"), gui::Rect(lx + 2, ly - 24, lx + boxW - 8, ly - 4),
			gui::Font::ID::SystemBold, td::ColorID::SysText, td::TextAlignment::Left, td::VAlignment::Center);
		legendRow(lx, ly, td::ColorID::Red,        false, "sabirnica kvara");
		legendRow(lx, ly, td::ColorID::DarkGreen,  false, "napon >= 0.9 pu");
		legendRow(lx, ly, td::ColorID::DarkOrange, false, "napon 0.7 - 0.9 pu");
		legendRow(lx, ly, td::ColorID::DarkRed,    false, "napon < 0.7 pu");
		legendRow(lx, ly, td::ColorID::Blue,       true,  "generator");
		gui::DrawableString::draw(td::String("debljina grane = struja (pu)"),
			gui::Rect(lx + 4, ly + 1, lx + boxW - 8, ly + 21), gui::Font::ID::SystemNormal,
			td::ColorID::SysText, td::TextAlignment::Left, td::VAlignment::Center);
	}

	// required so Canvas::exportToPDF/SVG works (uses the cached on-screen size)
	bool getModelSize(gui::Size& sz) const override { sz = _sz; return true; }

	void onDraw(const gui::Rect&) override
	{
		gui::Size sz;
		getSize(sz);
		if (sz.width > 20 && sz.height > 20) _sz = sz; // cache live size
		else sz = _sz;                                 // export / not-yet-realized
		int n = (int)_nodes.size();

		if (n == 0)
		{
			gui::DrawableString::draw(td::String("Ucitaj MATPOWER .m i klikni 'Izracunaj'."),
				gui::Rect(10, 10, sz.width - 10, 34), gui::Font::ID::SystemBold,
				td::ColorID::SysText, td::TextAlignment::Center);
			return;
		}

		double W = sz.width, H = sz.height;
		double cx = W / 2.0, cy = H / 2.0 + 10.0;
		double R = 0.38 * std::min(W, H - 60.0);
		if (R < 20.0) R = 20.0;

		std::vector<double> X(n), Y(n);
		for (int i = 0; i < n; ++i)
		{
			double a = -kPi / 2.0 + 2.0 * kPi * i / n;
			X[i] = cx + R * std::cos(a);
			Y[i] = cy + R * std::sin(a);
		}

		gui::DrawableString::draw(td::String("Jednopolna shema mreze"),
			gui::Rect(6, 2, W - 6, 20), gui::Font::ID::SystemBold, td::ColorID::SysText, td::TextAlignment::Center);

		bool vLabels = (n <= 18);
		bool iLabels = (n <= 12);

		// branches: single colour, thickness proportional to current (colour is reserved
		// for bus voltage, so it must NOT also encode current). Draw all lines first, then
		// the current labels on top with an opaque background so they stay readable.
		for (const Edge& e : _edges)
		{
			double rel = e._i / _maxI;
			float w = (float)(1.5 + 5.0 * rel);
			gui::Shape::drawLine(gui::Point(X[e._f], Y[e._f]), gui::Point(X[e._t], Y[e._t]),
				td::ColorID::SysText, w);
		}
		if (iLabels)
		{
			for (const Edge& e : _edges)
			{
				double mx = (X[e._f] + X[e._t]) / 2.0, my = (Y[e._f] + Y[e._t]) / 2.0;
				td::String s;
				s.format("%.2f pu", e._i);
				gui::Rect lr(mx - 38, my - 10, mx + 38, my + 10);
				gui::Shape::drawRect(lr, td::ColorID::SysCtrlBack);
				gui::DrawableString::draw(s, lr, gui::Font::ID::SystemNormal, td::ColorID::SysText,
					td::TextAlignment::Center, td::VAlignment::Center, td::TextEllipsize::None);
			}
		}

		// nodes
		for (int i = 0; i < n; ++i)
		{
			double bx = X[i], by = Y[i];
			double a = std::atan2(by - cy, bx - cx);

			if (_nodes[i]._gen)
			{
				double gx = bx + 22.0 * std::cos(a), gy = by + 22.0 * std::sin(a);
				gui::Shape::drawLine(gui::Point(bx, by), gui::Point(gx, gy), td::ColorID::SysText, 1);
				_circle.drawWireAtPointWithScale(td::ColorID::Blue, gui::Point(gx, gy), 8.0);
			}
			if (_nodes[i]._load)
			{
				double lx = bx - 18.0 * std::cos(a), ly = by - 18.0 * std::sin(a);
				gui::Shape::drawLine(gui::Point(bx, by), gui::Point(lx, ly), td::ColorID::DarkRed, 2);
			}

			gui::Rect br(bx - 15, by - 5, bx + 15, by + 5);
			td::ColorID fill = (i == _faultNode) ? td::ColorID::Red : vColor(_nodes[i]._v);
			gui::Shape::drawRect(br, fill, td::ColorID::SysText, 1.0f);
			if (i == _faultNode)
				_circle.drawWireAtPointWithScale(td::ColorID::Red, gui::Point(bx, by), 16.0);

			td::String num;
			num.format("%d", _nodes[i]._busId);
			gui::DrawableString::draw(num, gui::Rect(bx - 30, by - 28, bx + 30, by - 8),
				gui::Font::ID::SystemBold, td::ColorID::SysText,
				td::TextAlignment::Center, td::VAlignment::Center, td::TextEllipsize::None);

			if (vLabels)
			{
				td::String vs;
				vs.format("%.2f pu", _nodes[i]._v);
				gui::DrawableString::draw(vs, gui::Rect(bx - 46, by + 6, bx + 46, by + 26),
					gui::Font::ID::SystemNormal, td::ColorID::SysText,
					td::TextAlignment::Center, td::VAlignment::Center, td::TextEllipsize::None);
			}
		}

		drawLegend();
	}
};
