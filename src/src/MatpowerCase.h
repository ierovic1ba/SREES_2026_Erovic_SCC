#pragma once
#include <vector>
#include <unordered_map>
#include <td/Types.h>

//
// Parsed content of a MATPOWER case (positive-sequence power-flow data).
// Only the fields relevant for building Y_bus and the short-circuit calc are kept.
// Column meaning follows the MATPOWER Case Format v2.
//

struct ScBus
{
	int    _id     = 0;   // external bus number (bus_i)
	int    _type   = 1;   // 1=PQ, 2=PV, 3=ref/slack
	double _Pd     = 0.0; // MW
	double _Qd     = 0.0; // MVAr
	double _Gs     = 0.0; // MW  shunt conductance at V=1pu
	double _Bs     = 0.0; // MVAr shunt susceptance at V=1pu
	double _Vm     = 1.0; // pu
	double _Va     = 0.0; // deg
	double _baseKV = 0.0; // kV (line-to-line)
};

struct ScBranch
{
	int    _fbus   = 0;   // external from-bus id
	int    _tbus   = 0;   // external to-bus id
	double _r      = 0.0; // pu series resistance
	double _x      = 0.0; // pu series reactance
	double _b      = 0.0; // pu total line charging
	double _ratio  = 0.0; // tap ratio (0 means 1.0)
	double _angle  = 0.0; // deg phase shift
	int    _status = 1;   // 1=in service
};

struct ScGen
{
	int    _bus    = 0;   // external bus id
	double _Pg     = 0.0; // MW
	double _Qg     = 0.0; // MVAr
	double _Vg     = 1.0; // pu setpoint
	double _mBase  = 0.0; // machine MVA base (0 -> use system baseMVA)
	int    _status = 1;   // 1=in service
	double _xdp    = 0.0; // generator reactance Xd' on system base (0 -> use ScOptions default)
};

class MatpowerCase
{
public:
	double                    _baseMVA = 100.0;
	std::vector<ScBus>        _buses;
	std::vector<ScBranch>     _branches;
	std::vector<ScGen>        _gens;
	std::unordered_map<int, int> _busIndex; // external id -> 0-based index

	int size() const { return int(_buses.size()); }

	// external bus id -> 0-based index, or -1 if unknown
	int indexOf(int busId) const
	{
		auto it = _busIndex.find(busId);
		return (it == _busIndex.end()) ? -1 : it->second;
	}

	void rebuildIndex()
	{
		_busIndex.clear();
		for (int i = 0; i < int(_buses.size()); ++i)
			_busIndex[_buses[i]._id] = i;
	}
};
