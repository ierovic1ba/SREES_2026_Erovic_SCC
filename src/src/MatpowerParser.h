#pragma once
#include "MatpowerCase.h"
#include <td/String.h>

//
// Minimal parser for MATPOWER Case Format v2 files (.m).
// Reads the mpc.baseMVA scalar and the mpc.bus / mpc.gen / mpc.branch matrices.
// Ignores everything else (version, gencost, comments).
//
class MatpowerParser
{
public:
	// Returns true on success. On failure, err holds a human-readable message.
	static bool parse(const td::String& filePath, MatpowerCase& mpc, td::String& err);
};
