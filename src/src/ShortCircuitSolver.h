#pragma once
#include "MatpowerCase.h"
#include <td/String.h>
#include <td/Types.h>
#include <sparse/ISolver.h>
#include <vector>
#include <map>
#include <utility>

//
// Modeling options for the symmetrical (balanced three-phase) short circuit.
//
struct ScOptions
{
	// Default generator source reactance = transient Xd' on the SYSTEM base (100 MVA),
	// used directly (no machine-base conversion). 0.0608 matches the professor's dynamic
	// converter default (WSCC/Sauer-Pai generator 1). A per-generator value (ScGen::_xdp)
	// overrides this when > 0. MATPOWER .m files carry no generator reactance, so a value
	// must be assumed here. (No subtransient Xd'' exists anywhere in the professor's data.)
	double _genReactance    = 0.0608; // transient Xd', pu on system base
	bool   _includeGenerators = true; // model generators as a reactance to ground (required for a meaningful fault)
	bool   _includeLoads    = false;  // model loads as constant impedance to ground
	double _prefaultV       = 1.0;    // flat prefault voltage, pu
	double _faultReactance  = 0.0;    // fault reactance Zf, pu (0 = bolted fault)
};

//
// Result of a bolted (or Zf) three-phase fault at one bus.
//
struct FaultResult
{
	int       _faultBusId    = 0;
	int       _faultBusIndex = -1;
	td::cmplx _Zkk           = td::cmplx(0, 0); // driving-point impedance at the fault bus, pu
	td::cmplx _Ifpu          = td::cmplx(0, 0); // fault current, pu
	double    _IfkA          = 0.0;             // fault current, kA
	double    _faultMVA      = 0.0;             // short-circuit power, MVA
	std::vector<td::cmplx> _busV;               // post-fault bus voltages, pu (size N)
};

//
// Series current flowing through one branch during a fault.
//
struct BranchFault
{
	int       _fbus = 0;
	int       _tbus = 0;
	td::cmplx _Iij  = td::cmplx(0, 0); // series current from -> to, pu
	double    _Ipu  = 0.0;             // |Iij|, pu
	double    _IkA  = 0.0;             // |Iij|, kA (on to-bus base)
};

//
// Fault-current contribution of one generator (current it injects into the fault).
//
struct GenContribution
{
	int    _bus     = 0;
	double _Ipu     = 0.0; // |I_gen|, pu
	double _IkA     = 0.0; // |I_gen|, kA
	double _percent = 0.0; // share of the total generator current, %
};

//
// Builds Y_bus as a natID sparse complex matrix and computes short circuits via
// the Z-bus method: solve Y_bus * z = e_k for the k-th column of Z_bus, then
// I_f = V0 / (Z_kk + Zf) and V_i = V0 - z_i * I_f.
//
class ShortCircuitSolver
{
public:
	ShortCircuitSolver(const MatpowerCase& mpc, const ScOptions& opts);
	~ShortCircuitSolver();

	bool build(td::String& err);                               // build Y_bus and factorize (once)
	bool computeFault(int busIndex, FaultResult& res, td::String& err);
	// series current in every in-service branch for the given fault result
	void computeBranchCurrents(const FaultResult& res, std::vector<BranchFault>& out) const;
	// current each generator injects into the fault (E behind Xd')
	void computeGenContributions(const FaultResult& res, std::vector<GenContribution>& out) const;

	int size() const { return _mpc.size(); }
	const std::map<std::pair<int, int>, td::cmplx>& yEntries() const { return _y; }

private:
	const MatpowerCase& _mpc;
	ScOptions           _opts;
	std::map<std::pair<int, int>, td::cmplx> _y; // accumulated Y_bus, 0-based indices
	sparse::CmplxSolver* _pSolver   = nullptr;
	bool                 _factorized = false;

	void accumulate(int i, int j, const td::cmplx& v);
	void buildYMap();
};
