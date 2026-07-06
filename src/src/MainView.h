#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/LineEdit.h>
#include <gui/ComboBox.h>
#include <gui/Button.h>
#include <gui/TabView.h>
#include <gui/Image.h>
#include <gui/GridLayout.h>
#include <gui/GridComposer.h>
#include <gui/FileDialog.h>
#include <gui/Thread.h>
#include <fo/FileOperations.h>
#include <mu/ScopedCLocale.h>
#include <dp/IDatabase.h>
#include <dp/IDataSet.h>
#include <dp/DSColumns.h>
#include <td/String.h>

#include <vector>
#include <fstream>
#include <cmath>
#include <cstdlib>

#include "MatpowerCase.h"
#include "MatpowerParser.h"
#include "ShortCircuitSolver.h"
#include "TablePage.h"
#include "CanvasHost.h"
#include "ChartView.h"
#include "DiagramView.h"
#include "ReportCanvas.h"
#include "StatusBar.h"

//
// Main view: input panel on top, and a TabView with the result tables below.
//
class MainView : public gui::View
{
private:
	struct Results
	{
		ScOptions                _opt;
		int                      _selBusId = 0;
		FaultResult              _sel;
		std::vector<FaultResult> _perBus;
		std::vector<BranchFault> _branches;
		std::vector<GenContribution> _genContrib;
	};

	gui::Label    _lblFile;
	gui::LineEdit _leFile;
	gui::Button   _btnOpen;

	gui::Label    _lblXd;
	gui::LineEdit _leXd;
	gui::Label    _lblZf;
	gui::LineEdit _leZf;

	gui::Label    _lblGen;
	gui::ComboBox _cmbGen;
	gui::LineEdit _leGenXd;
	gui::Button   _btnSetGen;

	gui::Label    _lblBus;
	gui::ComboBox _cmbBus;
	gui::Button   _btnCompute;
	gui::Button   _btnExport;

	// _tabIcon and _tabs are declared BEFORE the tab pages on purpose: members are
	// destroyed in reverse declaration order, so the pages are torn down first, while
	// their parent TabView (_tabs) is still alive. The reverse order lets a page's
	// teardown touch an already-freed TabView -> heap corruption on close.
	gui::Image    _tabIcon;
	gui::TabView  _tabs;

	TablePage     _pgFault;
	TablePage     _pgVolt;
	TablePage     _pgBranch;
	TablePage     _pgGen;
	CanvasHost<ChartView>    _pgChart;
	CanvasHost<DiagramView>  _pgDiagram;
	CanvasHost<ReportCanvas> _pgReport;
	gui::GridLayout _gl;

	StatusBar*          _pStatus = nullptr;
	MatpowerCase        _mpc;
	std::vector<double> _genXd;   // per-generator Xd' override (0 = use global)
	bool                _loaded = false;

	static double parseD(const td::String& s, double defVal)
	{
		mu::ScopedCLocale loc;
		if (s.isEmpty())
			return defVal;
		return std::atof(s.c_str());
	}

	double globalXd() { return parseD(_leXd.getText(), 0.0608); }

	void setStatus(const td::String& s)  { if (_pStatus) _pStatus->setMessage(s); }
	void setSummary(const td::String& s) { if (_pStatus) _pStatus->setSummary(s); }

	void buildDataSets()
	{
		_pgFault._pDS = dp::createConnectionlessDataSet(dp::IDataSet::Size::Medium);
		dp::DSColumns fc(_pgFault._pDS->allocBindColumns(6));
		fc << "bus" << td::int4 << "zre" << td::real8 << "zim" << td::real8
		   << "ifpu" << td::real8 << "ifka" << td::real8 << "mva" << td::real8;
		_pgFault._pDS->execute();

		_pgVolt._pDS = dp::createConnectionlessDataSet(dp::IDataSet::Size::Medium);
		dp::DSColumns vc(_pgVolt._pDS->allocBindColumns(3));
		vc << "bus" << td::int4 << "vpu" << td::real8 << "ang" << td::real8;
		_pgVolt._pDS->execute();

		_pgBranch._pDS = dp::createConnectionlessDataSet(dp::IDataSet::Size::Medium);
		dp::DSColumns bc(_pgBranch._pDS->allocBindColumns(4));
		bc << "fb" << td::int4 << "tb" << td::int4 << "ipu" << td::real8 << "ika" << td::real8;
		_pgBranch._pDS->execute();

		_pgGen._pDS = dp::createConnectionlessDataSet(dp::IDataSet::Size::Medium);
		dp::DSColumns gc(_pgGen._pDS->allocBindColumns(4));
		gc << "bus" << td::int4 << "ipu" << td::real8 << "ika" << td::real8 << "pct" << td::real8;
		_pgGen._pDS->execute();
	}

	void initTables()
	{
		gui::Columns fcols(_pgFault._table.allocBindColumns(6));
		fcols << gui::Header(0, tr("hBus"),  "", 90,  td::HAlignment::Right)
		      << gui::Header(1, tr("hZre"),  "", 100, td::HAlignment::Right)
		      << gui::Header(2, tr("hZim"),  "", 100, td::HAlignment::Right)
		      << gui::Header(3, tr("hIfpu"), "", 100, td::HAlignment::Right)
		      << gui::Header(4, tr("hIfka"), "", 100, td::HAlignment::Right)
		      << gui::Header(5, tr("hSk"),   "", 110, td::HAlignment::Right);
		_pgFault._table.init(_pgFault._pDS);
		_pgFault._table.setColumnNumericFormat(1, td::FormatFloat::Decimal, 4);
		_pgFault._table.setColumnNumericFormat(2, td::FormatFloat::Decimal, 4);
		_pgFault._table.setColumnNumericFormat(3, td::FormatFloat::Decimal, 3);
		_pgFault._table.setColumnNumericFormat(4, td::FormatFloat::Decimal, 3);
		_pgFault._table.setColumnNumericFormat(5, td::FormatFloat::Decimal, 1);

		gui::Columns vcols(_pgVolt._table.allocBindColumns(3));
		vcols << gui::Header(0, tr("hBus"), "", 90,  td::HAlignment::Right)
		      << gui::Header(1, tr("hV"),   "", 100, td::HAlignment::Right)
		      << gui::Header(2, tr("hAng"), "", 100, td::HAlignment::Right);
		_pgVolt._table.init(_pgVolt._pDS);
		_pgVolt._table.setColumnNumericFormat(1, td::FormatFloat::Decimal, 4);
		_pgVolt._table.setColumnNumericFormat(2, td::FormatFloat::Decimal, 2);

		gui::Columns bcols(_pgBranch._table.allocBindColumns(4));
		bcols << gui::Header(0, tr("hFrom"), "", 110, td::HAlignment::Right)
		      << gui::Header(1, tr("hTo"),   "", 110, td::HAlignment::Right)
		      << gui::Header(2, tr("hIpu"),  "", 100, td::HAlignment::Right)
		      << gui::Header(3, tr("hIka"),  "", 100, td::HAlignment::Right);
		_pgBranch._table.init(_pgBranch._pDS);
		_pgBranch._table.setColumnNumericFormat(2, td::FormatFloat::Decimal, 4);
		_pgBranch._table.setColumnNumericFormat(3, td::FormatFloat::Decimal, 3);

		gui::Columns gcols(_pgGen._table.allocBindColumns(4));
		gcols << gui::Header(0, tr("hGenBus"), "", 120, td::HAlignment::Right)
		      << gui::Header(1, tr("hIpu"),    "", 100, td::HAlignment::Right)
		      << gui::Header(2, tr("hIka"),    "", 100, td::HAlignment::Right)
		      << gui::Header(3, tr("hPct"),    "", 120, td::HAlignment::Right);
		_pgGen._table.init(_pgGen._pDS);
		_pgGen._table.setColumnNumericFormat(1, td::FormatFloat::Decimal, 4);
		_pgGen._table.setColumnNumericFormat(2, td::FormatFloat::Decimal, 3);
		_pgGen._table.setColumnNumericFormat(3, td::FormatFloat::Decimal, 1);
	}

	bool runCompute(Results& R, td::String& err)
	{
		if (!_loaded)
		{
			err = "Prvo ucitaj MATPOWER .m fajl.";
			return false;
		}
		R._opt._genReactance   = globalXd();
		R._opt._faultReactance = parseD(_leZf.getText(), 0.0);

		for (int k = 0; k < (int)_mpc._gens.size() && k < (int)_genXd.size(); ++k)
			_mpc._gens[k]._xdp = _genXd[k];

		ShortCircuitSolver scs(_mpc, R._opt);
		if (!scs.build(err))
			return false;

		int n = _mpc.size();
		int selIdx = _cmbBus.getSelectedIndex();
		if (selIdx < 0 || selIdx >= n)
			selIdx = 0;

		R._perBus.assign(n, FaultResult());
		for (int i = 0; i < n; ++i)
			scs.computeFault(i, R._perBus[i], err);

		R._sel      = R._perBus[selIdx];
		R._selBusId = R._sel._faultBusId;
		scs.computeBranchCurrents(R._sel, R._branches);
		scs.computeGenContributions(R._sel, R._genContrib);
		return true;
	}

	void findWorst(const Results& R, int& worstIdx, double& worstSk)
	{
		worstIdx = 0; worstSk = -1.0;
		for (int i = 0; i < (int)R._perBus.size(); ++i)
			if (R._perBus[i]._faultMVA > worstSk) { worstSk = R._perBus[i]._faultMVA; worstIdx = i; }
	}

	// Per-bus fault levels do not depend on the selected bus. Only filled on a full
	// compute -- NEVER from the fault table's own selection handler, because
	// reloading a TableEdit from inside its selection event corrupts it (adds rows).
	void fillFaultTable(const Results& R)
	{
		_pgFault._pDS->removeAll();
		for (int i = 0; i < (int)R._perBus.size(); ++i)
		{
			const FaultResult& r = R._perBus[i];
			dp::IDataSet::Row row = _pgFault._pDS->getEmptyRow();
			row[0] = (td::INT4)r._faultBusId;
			row[1] = r._Zkk.real();
			row[2] = r._Zkk.imag();
			row[3] = std::abs(r._Ifpu);
			row[4] = r._IfkA;
			row[5] = r._faultMVA;
			_pgFault._pDS->push_back();
		}
		_pgFault._table.reload();
		if (R._sel._faultBusIndex >= 0) // select the analysed bus row (silent: no message)
			_pgFault._table.selectRow(R._sel._faultBusIndex, false, true);

		int worstIdx; double worstSk; findWorst(R, worstIdx, worstSk);
		if (!R._perBus.empty())
		{
			td::String sum;
			sum.format("Najveca snaga kvara Sk = %.1f MVA  (na sabirnici %d)", worstSk, R._perBus[worstIdx]._faultBusId);
			setSummary(sum);
		}
	}

	// Results that depend on the selected fault bus. Safe to call from the fault
	// table's selection handler because it does NOT touch the fault table itself.
	void fillSelected(const Results& R)
	{
		_pgVolt._pDS->removeAll();
		for (int i = 0; i < _mpc.size(); ++i)
		{
			const td::cmplx& v = R._sel._busV[i];
			double mag = std::abs(v);
			double ang = std::atan2(v.imag(), v.real()) * 180.0 / 3.14159265358979323846;
			dp::IDataSet::Row row = _pgVolt._pDS->getEmptyRow();
			row[0] = (td::INT4)_mpc._buses[i]._id;
			row[1] = mag;
			row[2] = ang;
			_pgVolt._pDS->push_back();
		}
		_pgVolt._table.reload();

		_pgBranch._pDS->removeAll();
		for (const BranchFault& b : R._branches)
		{
			dp::IDataSet::Row row = _pgBranch._pDS->getEmptyRow();
			row[0] = (td::INT4)b._fbus;
			row[1] = (td::INT4)b._tbus;
			row[2] = b._Ipu;
			row[3] = b._IkA;
			_pgBranch._pDS->push_back();
		}
		_pgBranch._table.reload();

		_pgGen._pDS->removeAll();
		for (const GenContribution& g : R._genContrib)
		{
			dp::IDataSet::Row row = _pgGen._pDS->getEmptyRow();
			row[0] = (td::INT4)g._bus;
			row[1] = g._Ipu;
			row[2] = g._IkA;
			row[3] = g._percent;
			_pgGen._pDS->push_back();
		}
		_pgGen._table.reload();

		int worstIdx; double worstSk; findWorst(R, worstIdx, worstSk);
		std::vector<std::pair<int, double>> faultBars, voltBars;
		faultBars.reserve(R._perBus.size());
		for (const FaultResult& r : R._perBus)
			faultBars.push_back(std::make_pair(r._faultBusId, r._faultMVA));
		voltBars.reserve(_mpc.size());
		for (int i = 0; i < _mpc.size(); ++i)
			voltBars.push_back(std::make_pair(_mpc._buses[i]._id, std::abs(R._sel._busV[i])));
		_pgChart._canvas.setData(faultBars, voltBars, worstIdx, R._selBusId);
		_pgDiagram._canvas.setNetwork(_mpc, R._sel, R._branches, R._sel._faultBusIndex);
		_pgReport._canvas.setReport(_leFile.getText(), _mpc._baseMVA,
			R._opt._genReactance, R._opt._faultReactance, R._selBusId, R._sel,
			R._perBus, R._genContrib, worstSk, R._perBus[worstIdx]._faultBusId);
	}

	void fillTables(const Results& R)
	{
		fillFaultTable(R);
		fillSelected(R);
	}

	void doOpen()
	{
		gui::OpenFileDialog::show(this, tr("openCase"), "*.m", 7000, [this](gui::FileDialog* pDlg)
		{
			if (pDlg->getStatus() != gui::FileDialog::Status::OK)
				return;
			td::String fileName = pDlg->getFileName();
			if (fileName.isEmpty())
				return;
			loadCase(fileName);
		});
	}

	void loadCase(const td::String& fileName)
	{
		MatpowerCase mpc;
		td::String err;
		if (!MatpowerParser::parse(fileName, mpc, err))
		{
			_loaded = false;
			setStatus(err);
			return;
		}
		_mpc = mpc;
		_loaded = true;
		_leFile = fileName;

		_cmbBus.clean();
		for (const ScBus& b : _mpc._buses)
		{
			td::String s;
			s.format("Sabirnica %d", b._id);
			_cmbBus.addItem(s);
		}
		if (!_mpc._buses.empty())
			_cmbBus.selectIndex(0);

		_genXd.assign(_mpc._gens.size(), 0.0);
		_cmbGen.clean();
		for (int k = 0; k < (int)_mpc._gens.size(); ++k)
		{
			td::String s;
			s.format("Gen %d (sab. %d)", k + 1, _mpc._gens[k]._bus);
			_cmbGen.addItem(s);
		}
		if (!_mpc._gens.empty())
			_cmbGen.selectIndex(0);
		showGenXd();

		_pgFault._pDS->removeAll();  _pgFault._table.reload();
		_pgVolt._pDS->removeAll();   _pgVolt._table.reload();
		_pgBranch._pDS->removeAll(); _pgBranch._table.reload();
		_pgGen._pDS->removeAll();    _pgGen._table.reload();
		setSummary(td::String(""));

		td::String info;
		info.format("Ucitano: baseMVA=%.1f, sabirnice=%d, generatori=%d, grane=%d. Klikni 'Izracunaj'.",
			_mpc._baseMVA, _mpc.size(), (int)_mpc._gens.size(), (int)_mpc._branches.size());
		setStatus(info);
	}

	void showGenXd()
	{
		int idx = _cmbGen.getSelectedIndex();
		double val = (idx >= 0 && idx < (int)_genXd.size() && _genXd[idx] > 0.0)
			? _genXd[idx] : globalXd();
		td::String s;
		s.format("%.4f", val);
		_leGenXd = s;
	}

	void setGenXd()
	{
		int idx = _cmbGen.getSelectedIndex();
		if (idx < 0 || idx >= (int)_genXd.size())
			return;
		_genXd[idx] = parseD(_leGenXd.getText(), globalXd());
		td::String s;
		s.format("Postavljeno Xd'=%.4f za generator na sabirnici %d.", _genXd[idx], _mpc._gens[idx]._bus);
		setStatus(s);
	}

	void compute()
	{
		Results R;
		td::String err;
		if (!runCompute(R, err))
		{
			setStatus(err);
			return;
		}
		fillTables(R);

		td::String st;
		st.format("Kvar na sabirnici %d:  struja kvara If = %.3f pu (%.3f kA),  snaga kvara Sk = %.1f MVA",
			R._selBusId, std::abs(R._sel._Ifpu), R._sel._IfkA, R._sel._faultMVA);
		setStatus(st);
	}

	void doExport()
	{
		if (!_loaded)
		{
			setStatus(td::String("Prvo ucitaj .m fajl i izracunaj."));
			return;
		}
		gui::SaveFileDialog::show(this, tr("saveResults"), "*.csv", 8000, [this](gui::FileDialog* pDlg)
		{
			if (pDlg->getStatus() != gui::FileDialog::Status::OK)
				return;
			td::String fileName = pDlg->getFileName();
			if (fileName.isEmpty())
				return;
			exportCsv(fileName);
		});
	}

	void exportCsv(const td::String& fileName)
	{
		Results R;
		td::String err;
		if (!runCompute(R, err))
		{
			setStatus(err);
			return;
		}

		mu::ScopedCLocale loc;
		std::ofstream f(fileName.c_str());
		if (!f.is_open())
		{
			setStatus(td::String("Ne mogu otvoriti izlazni fajl."));
			return;
		}

		f << "# SREES 2026 - Proracun kratkih spojeva (simetricne mreze)\n";
		f << "# baseMVA=" << _mpc._baseMVA << ", Xd'=" << R._opt._genReactance
		  << ", Zf=" << R._opt._faultReactance << ", faultBus=" << R._selBusId << "\n\n";

		f << "# Nivoi kvara po sabirnicama\n";
		f << "Bus,Zkk_re,Zkk_im,If_pu,If_kA,Sk_MVA\n";
		for (const FaultResult& r : R._perBus)
			f << r._faultBusId << ',' << r._Zkk.real() << ',' << r._Zkk.imag() << ','
			  << std::abs(r._Ifpu) << ',' << r._IfkA << ',' << r._faultMVA << '\n';

		f << "\n# Naponi poslije kvara na sabirnici " << R._selBusId << "\n";
		f << "Bus,V_pu,Angle_deg\n";
		for (int i = 0; i < _mpc.size(); ++i)
		{
			const td::cmplx& v = R._sel._busV[i];
			double ang = std::atan2(v.imag(), v.real()) * 180.0 / 3.14159265358979323846;
			f << _mpc._buses[i]._id << ',' << std::abs(v) << ',' << ang << '\n';
		}

		f << "\n# Struje grana pri kvaru na sabirnici " << R._selBusId << "\n";
		f << "From,To,I_pu,I_kA\n";
		for (const BranchFault& b : R._branches)
			f << b._fbus << ',' << b._tbus << ',' << b._Ipu << ',' << b._IkA << '\n';

		f << "\n# Doprinos generatora pri kvaru na sabirnici " << R._selBusId << "\n";
		f << "GenBus,I_pu,I_kA,Percent\n";
		for (const GenContribution& g : R._genContrib)
			f << g._bus << ',' << g._Ipu << ',' << g._IkA << ',' << g._percent << '\n';

		f.close();

		td::String st;
		st.format("Rezultati snimljeni u: %s", fileName.c_str());
		setStatus(st);
	}

	// export the currently visible canvas tab (chart / diagram / report) to PDF or SVG
	void doExportImage()
	{
		// tab order: 0 fault, 1 volt, 2 branch, 3 gen, 4 chart, 5 diagram, 6 report
		int pos = _tabs.getCurrentViewPos();
		gui::Canvas* pCanvas = nullptr;
		if (pos == 4)      pCanvas = &_pgChart._canvas;
		else if (pos == 5) pCanvas = &_pgDiagram._canvas;
		else if (pos == 6) pCanvas = &_pgReport._canvas;
		if (!pCanvas)
		{
			setStatus(td::String("Za izvoz slike prvo otvori tab 'Grafici', 'Shema mreze' ili 'Izvjestaj', pa klikni Izvezi sliku."));
			return;
		}
		gui::SaveFileDialog::show(this, tr("saveImage"), "*.pdf", 9000, [this, pCanvas](gui::FileDialog* pDlg)
		{
			if (pDlg->getStatus() != gui::FileDialog::Status::OK)
				return;
			td::String fn = pDlg->getFileName();
			if (fn.isEmpty())
				return;
			std::string s = fn.c_str();
			bool svg = (s.size() >= 4 && s.compare(s.size() - 4, 4, ".svg") == 0);
			// export the whole model (each canvas provides getModelSize)
			bool ok = svg ? pCanvas->exportToSVG(fn, false) : pCanvas->exportToPDF(fn, false);
			td::String st;
			if (ok) st.format("Slika snimljena: %s", fn.c_str());
			else    st = "Greska pri izvozu slike.";
			setStatus(st);
		});
	}

	void handleUserActions()
	{
		_btnOpen.onClick([this] { doOpen(); });
		_btnCompute.onClick([this] { compute(); });
		_btnExport.onClick([this] { doExport(); });
		_btnSetGen.onClick([this] { setGenXd(); });
		_cmbGen.onChangedSelection([this] { showGenXd(); });
		// (the fault bus is chosen via the combo box + 'Izracunaj'; clicking a table
		//  row is intentionally not wired -- reloading a TableEdit from inside its own
		//  selection event corrupts it)
	}

public:
	MainView()
		: _lblFile(tr("caseFile"))
		, _btnOpen("…")
		, _lblXd(tr("genReact"))
		, _lblZf(tr("faultReact"))
		, _lblGen(tr("genSel"))
		, _btnSetGen(tr("setGen"))
		, _lblBus(tr("faultBus"))
		, _btnCompute(tr("compute"))
		, _btnExport(tr("exportCsv"))
		, _tabIcon(":imgOpen")
		, _tabs(gui::TabHeader::Type::FitToText, 8, 64)
		, _gl(5, 4)
	{
		_leFile.setAsReadOnly();
		_leXd = td::String("0.0608");
		_leZf = td::String("0.0");
		_leGenXd = td::String("0.0608");
		_leXd.setToolTip(td::String("Podrazumijevana reaktansa Xd' za SVE generatore (pu, sistemska baza)."));
		_leZf.setToolTip(td::String("Reaktansa na MJESTU KVARA Zf (pu). 0 = metalni (bolted) kvar."));
		_leGenXd.setToolTip(td::String("Xd' za odabrani generator; dugme 'Postavi' primjenjuje samo na njega."));

		buildDataSets();
		initTables();

		int p;
		p = _tabs.addView(&_pgFault,   tr("tabFaults"),  &_tabIcon); _tabs.setViewOwnership(p, td::Ownership::Extern); _tabs.setNonRemovable(p, true);
		p = _tabs.addView(&_pgVolt,    tr("tabVolt"),    &_tabIcon); _tabs.setViewOwnership(p, td::Ownership::Extern); _tabs.setNonRemovable(p, true);
		p = _tabs.addView(&_pgBranch,  tr("tabBranch"),  &_tabIcon); _tabs.setViewOwnership(p, td::Ownership::Extern); _tabs.setNonRemovable(p, true);
		p = _tabs.addView(&_pgGen,     tr("tabGen"),     &_tabIcon); _tabs.setViewOwnership(p, td::Ownership::Extern); _tabs.setNonRemovable(p, true);
		p = _tabs.addView(&_pgChart,   tr("tabChart"),   &_tabIcon); _tabs.setViewOwnership(p, td::Ownership::Extern); _tabs.setNonRemovable(p, true);
		p = _tabs.addView(&_pgDiagram, tr("tabDiagram"), &_tabIcon); _tabs.setViewOwnership(p, td::Ownership::Extern); _tabs.setNonRemovable(p, true);
		p = _tabs.addView(&_pgReport,  tr("tabReport"),  &_tabIcon); _tabs.setViewOwnership(p, td::Ownership::Extern); _tabs.setNonRemovable(p, true);

		gui::GridComposer gc(_gl);
		gc.appendRow(_lblFile) << _leFile << _btnOpen;
		gc.appendRow(_lblXd)   << _leXd << _lblZf << _leZf;
		gc.appendRow(_lblGen)  << _cmbGen << _leGenXd << _btnSetGen;
		gc.appendRow(_lblBus)  << _cmbBus << _btnCompute << _btnExport;
		gc.appendRow(_tabs, 0);
		setLayout(&_gl);

		handleUserActions();
	}

	void setStatusBar(StatusBar* p) { _pStatus = p; }

	// Called just before the window closes: activate a non-canvas tab so the canvas
	// widgets are not the visible view during teardown (a canvas being torn down
	// while active trips a debug-only assertion in natID on some platforms).
	void prepareForClose() { _tabs.showView(0); }

	// menu and toolbar actions
	bool onActionItem(gui::ActionItemDescriptor& aiDesc) override
	{
		auto [menuID, firstSubMenuID, lastSubMenuID, actionID] = aiDesc.getIDs();
		if (menuID == 20)
		{
			switch (actionID)
			{
				case 10: doOpen();        return true; // Open
				case 30: compute();       return true; // Compute
				case 40: doExport();      return true; // Export CSV
				case 50: doExportImage(); return true; // Export image
				default: break;
			}
		}
		return gui::View::onActionItem(aiDesc);
	}
};
