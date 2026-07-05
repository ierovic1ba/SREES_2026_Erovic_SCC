#include "ShortCircuitSolver.h"
#include <cmath>

namespace
{
	constexpr double kPi = 3.14159265358979323846;
}

ShortCircuitSolver::ShortCircuitSolver(const MatpowerCase& mpc, const ScOptions& opts)
	: _mpc(mpc)
	, _opts(opts)
{
}

ShortCircuitSolver::~ShortCircuitSolver()
{
	if (_pSolver)
		_pSolver->release();
	_pSolver = nullptr;
}

void ShortCircuitSolver::accumulate(int i, int j, const td::cmplx& v)
{
	auto key = std::make_pair(i, j);
	auto it = _y.find(key);
	if (it == _y.end())
		_y[key] = v;
	else
		it->second += v;
}

void ShortCircuitSolver::buildYMap()
{
	_y.clear();

	// --- branches: series admittance, line charging, transformer tap/phase shift ---
	for (const ScBranch& br : _mpc._branches)
	{
		if (br._status < 1)
			continue;
		int f = _mpc.indexOf(br._fbus);
		int t = _mpc.indexOf(br._tbus);
		if (f < 0 || t < 0)
			continue;

		td::cmplx ys = (br._r == 0.0 && br._x == 0.0)
			? td::cmplx(0, 0)
			: td::cmplx(1, 0) / td::cmplx(br._r, br._x); // series admittance 1/(r+jx)
		td::cmplx ysh(0, br._b / 2.0);                    // half line charging at each end

		double tap   = (br._ratio == 0.0) ? 1.0 : br._ratio;
		double shift = br._angle * kPi / 180.0;
		td::cmplx a     = std::polar(tap, shift);         // complex tap on the from side
		td::cmplx aConj = std::conj(a);

		accumulate(f, f, (ys + ysh) / (a * aConj));
		accumulate(t, t,  ys + ysh);
		accumulate(f, t, -ys / aConj);
		accumulate(t, f, -ys / a);
	}

	// --- bus shunts (Gs, Bs are in MW/MVAr at V = 1 pu) ---
	for (const ScBus& b : _mpc._buses)
	{
		if (b._Gs != 0.0 || b._Bs != 0.0)
		{
			int i = _mpc.indexOf(b._id);
			accumulate(i, i, td::cmplx(b._Gs, b._Bs) / _mpc._baseMVA);
		}
	}

	// --- generators as a reactance to ground (source impedances) ---
	// Reactance is transient Xd' on the system base, used directly. A per-generator
	// value overrides the global default when provided.
	if (_opts._includeGenerators)
	{
		for (const ScGen& g : _mpc._gens)
		{
			if (g._status < 1)
				continue;
			int i = _mpc.indexOf(g._bus);
			if (i < 0)
				continue;
			double xd = (g._xdp > 0.0) ? g._xdp : _opts._genReactance; // pu on system base
			if (xd > 0.0)
				accumulate(i, i, td::cmplx(1, 0) / td::cmplx(0, xd)); // 1/(j*Xd') = -j/Xd'
		}
	}

	// --- loads as constant impedance to ground (optional) ---
	if (_opts._includeLoads)
	{
		for (const ScBus& b : _mpc._buses)
		{
			if (b._Pd != 0.0 || b._Qd != 0.0)
			{
				int i = _mpc.indexOf(b._id);
				// y_load = conj(S)/|V|^2 with |V| = 1 pu, S in pu
				accumulate(i, i, td::cmplx(b._Pd, -b._Qd) / _mpc._baseMVA);
			}
		}
	}
}

bool ShortCircuitSolver::build(td::String& err)
{
	buildYMap();

	int n  = _mpc.size();
	if (n <= 0)
	{
		err = "Prazna mreza.";
		return false;
	}
	int nz = int(_y.size()) + n + 16; // generous nonzero estimate (safe to overestimate)

	if (_pSolver)
	{
		_pSolver->release();
		_pSolver = nullptr;
	}
	_pSolver = sparse::createCmplxSolver(
		n, nz,
		sparse::Symmetry::NonSymmetric,
		sparse::SolverType::LU,
		sparse::Pivoting::DiagonalMultiPass,
		sparse::Ordering::Own);
	if (!_pSolver)
	{
		err = "createCmplxSolver nije uspio.";
		return false;
	}

	_pSolver->populateDiagonals(td::cmplx(0, 0));
	for (const auto& kv : _y)
		_pSolver->addTriple(kv.first.first, kv.first.second, kv.second);

	if (!_pSolver->factorize())
	{
		const char* e = _pSolver->getLastError();
		err = e ? e : "Faktorizacija Y_bus nije uspjela.";
		return false;
	}
	_factorized = true;
	return true;
}

bool ShortCircuitSolver::computeFault(int busIndex, FaultResult& res, td::String& err)
{
	if (!_factorized || !_pSolver)
	{
		err = "Y_bus nije izgradjen.";
		return false;
	}
	int n = _mpc.size();
	if (busIndex < 0 || busIndex >= n)
	{
		err = "Neispravan indeks sabirnice.";
		return false;
	}

	// solve Y_bus * z = e_k  ->  z is the k-th column of Z_bus
	_pSolver->clearRHS();
	_pSolver->setRHS(busIndex, td::cmplx(1, 0));
	if (!_pSolver->solve())
	{
		err = "solve() nije uspio.";
		return false;
	}

	std::vector<td::cmplx> z(n);
	for (int i = 0; i < n; ++i)
		z[i] = _pSolver->x(i);

	td::cmplx Zkk = z[busIndex];
	td::cmplx Zf(0, _opts._faultReactance);
	td::cmplx V0(_opts._prefaultV, 0);
	td::cmplx If = V0 / (Zkk + Zf);

	res._faultBusIndex = busIndex;
	res._faultBusId    = _mpc._buses[busIndex]._id;
	res._Zkk           = Zkk;
	res._Ifpu          = If;
	res._busV.resize(n);
	for (int i = 0; i < n; ++i)
		res._busV[i] = V0 - z[i] * If;

	double baseKV = _mpc._buses[busIndex]._baseKV;
	double Ibase  = (baseKV > 0.0) ? _mpc._baseMVA / (std::sqrt(3.0) * baseKV) : 0.0; // kA
	res._IfkA     = std::abs(If) * Ibase;
	res._faultMVA = _mpc._baseMVA * std::abs(If) * _opts._prefaultV;
	return true;
}

void ShortCircuitSolver::computeBranchCurrents(const FaultResult& res, std::vector<BranchFault>& out) const
{
	out.clear();
	if (int(res._busV.size()) != _mpc.size())
		return;

	for (const ScBranch& br : _mpc._branches)
	{
		if (br._status < 1)
			continue;
		int f = _mpc.indexOf(br._fbus);
		int t = _mpc.indexOf(br._tbus);
		if (f < 0 || t < 0)
			continue;

		td::cmplx ys = (br._r == 0.0 && br._x == 0.0)
			? td::cmplx(0, 0)
			: td::cmplx(1, 0) / td::cmplx(br._r, br._x);
		double tap   = (br._ratio == 0.0) ? 1.0 : br._ratio;
		double shift = br._angle * kPi / 180.0;
		td::cmplx a  = std::polar(tap, shift);

		// series current through the branch impedance (to-side reference)
		td::cmplx Iij = ys * (res._busV[f] / a - res._busV[t]);

		BranchFault bf;
		bf._fbus = br._fbus;
		bf._tbus = br._tbus;
		bf._Iij  = Iij;
		bf._Ipu  = std::abs(Iij);
		double baseKV = _mpc._buses[t]._baseKV;
		double Ibase  = (baseKV > 0.0) ? _mpc._baseMVA / (std::sqrt(3.0) * baseKV) : 0.0;
		bf._IkA  = bf._Ipu * Ibase;
		out.push_back(bf);
	}
}

void ShortCircuitSolver::computeGenContributions(const FaultResult& res, std::vector<GenContribution>& out) const
{
	out.clear();
	if (int(res._busV.size()) != _mpc.size())
		return;

	td::cmplx E(_opts._prefaultV, 0); // internal EMF ~ prefault voltage (flat)
	double sumMag = 0.0;

	for (const ScGen& g : _mpc._gens)
	{
		if (g._status < 1)
			continue;
		int i = _mpc.indexOf(g._bus);
		if (i < 0)
			continue;
		double xd = (g._xdp > 0.0) ? g._xdp : _opts._genReactance;
		if (xd <= 0.0)
			continue;

		// current injected by the generator: I = (E - V_gbus) / (j*Xd')
		td::cmplx I = (E - res._busV[i]) / td::cmplx(0, xd);

		GenContribution gc;
		gc._bus = g._bus;
		gc._Ipu = std::abs(I);
		double baseKV = _mpc._buses[i]._baseKV;
		double Ibase  = (baseKV > 0.0) ? _mpc._baseMVA / (std::sqrt(3.0) * baseKV) : 0.0;
		gc._IkA = gc._Ipu * Ibase;
		out.push_back(gc);
		sumMag += gc._Ipu;
	}

	for (GenContribution& gc : out)
		gc._percent = (sumMag > 0.0) ? 100.0 * gc._Ipu / sumMag : 0.0;
}
