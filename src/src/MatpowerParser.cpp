#include "MatpowerParser.h"
#include <mu/ScopedCLocale.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

namespace
{
	enum class Block { None, Bus, Gen, Branch };

	std::string stripComment(const std::string& line)
	{
		auto pos = line.find('%');
		return (pos == std::string::npos) ? line : line.substr(0, pos);
	}

	std::string trim(const std::string& s)
	{
		size_t a = s.find_first_not_of(" \t\r\n");
		if (a == std::string::npos)
			return std::string();
		size_t b = s.find_last_not_of(" \t\r\n");
		return s.substr(a, b - a + 1);
	}

	// parse a single numeric token; supports simple "a/b" expressions found in some cases
	double parseNum(const std::string& t)
	{
		auto pos = t.find('/');
		if (pos != std::string::npos)
		{
			double a = std::atof(t.substr(0, pos).c_str());
			double b = std::atof(t.substr(pos + 1).c_str());
			return (b != 0.0) ? a / b : 0.0;
		}
		return std::atof(t.c_str());
	}

	std::vector<std::string> tokenize(const std::string& s)
	{
		std::vector<std::string> out;
		std::istringstream iss(s);
		std::string tok;
		while (iss >> tok)
			out.push_back(tok);
		return out;
	}

	// left-hand side variable name of an assignment line ("mpc.bus" from "mpc.bus = [")
	std::string lhsName(const std::string& s)
	{
		auto pos = s.find('=');
		if (pos == std::string::npos)
			return std::string();
		return trim(s.substr(0, pos));
	}

	void addBusRow(const std::vector<std::string>& t, MatpowerCase& mpc)
	{
		if (t.size() < 10) return;
		ScBus b;
		b._id     = int(parseNum(t[0]));
		b._type   = int(parseNum(t[1]));
		b._Pd     = parseNum(t[2]);
		b._Qd     = parseNum(t[3]);
		b._Gs     = parseNum(t[4]);
		b._Bs     = parseNum(t[5]);
		b._Vm     = parseNum(t[7]);
		b._Va     = parseNum(t[8]);
		b._baseKV = parseNum(t[9]);
		mpc._buses.push_back(b);
	}

	void addGenRow(const std::vector<std::string>& t, MatpowerCase& mpc)
	{
		if (t.size() < 8) return;
		ScGen g;
		g._bus    = int(parseNum(t[0]));
		g._Pg     = parseNum(t[1]);
		g._Qg     = parseNum(t[2]);
		g._Vg     = parseNum(t[5]);
		g._mBase  = parseNum(t[6]);
		g._status = int(parseNum(t[7]));
		mpc._gens.push_back(g);
	}

	void addBranchRow(const std::vector<std::string>& t, MatpowerCase& mpc)
	{
		if (t.size() < 11) return;
		ScBranch br;
		br._fbus   = int(parseNum(t[0]));
		br._tbus   = int(parseNum(t[1]));
		br._r      = parseNum(t[2]);
		br._x      = parseNum(t[3]);
		br._b      = parseNum(t[4]);
		br._ratio  = parseNum(t[8]);
		br._angle  = parseNum(t[9]);
		br._status = int(parseNum(t[10]));
		mpc._branches.push_back(br);
	}

	void addRow(Block block, const std::vector<std::string>& t, MatpowerCase& mpc)
	{
		switch (block)
		{
			case Block::Bus:    addBusRow(t, mpc);    break;
			case Block::Gen:    addGenRow(t, mpc);    break;
			case Block::Branch: addBranchRow(t, mpc); break;
			default: break;
		}
	}
}

bool MatpowerParser::parse(const td::String& filePath, MatpowerCase& mpc, td::String& err)
{
	mu::ScopedCLocale scopedLocale; // force '.' decimal separator regardless of system locale

	std::ifstream f(filePath.c_str());
	if (!f.is_open())
	{
		err = "Ne mogu otvoriti fajl.";
		return false;
	}

	mpc = MatpowerCase();
	Block block = Block::None;
	std::string line;
	bool haveBase = false;

	while (std::getline(f, line))
	{
		std::string s = stripComment(line);

		if (block == Block::None)
		{
			std::string lhs = lhsName(s);
			if (lhs == "mpc.baseMVA")
			{
				auto pos = s.find('=');
				std::string rhs = trim(s.substr(pos + 1));
				// strip trailing ';'
				if (!rhs.empty() && rhs.back() == ';')
					rhs.pop_back();
				mpc._baseMVA = parseNum(trim(rhs));
				haveBase = true;
			}
			else if (lhs == "mpc.bus" || lhs == "mpc.gen" || lhs == "mpc.branch")
			{
				if (s.find('[') != std::string::npos)
				{
					block = (lhs == "mpc.bus") ? Block::Bus
						  : (lhs == "mpc.gen") ? Block::Gen : Block::Branch;
					// handle possible data after '[' on the same line
					std::string rest = s.substr(s.find('[') + 1);
					bool ends = rest.find(']') != std::string::npos;
					for (char& c : rest) if (c == ']' || c == ';') c = ' ';
					auto toks = tokenize(rest);
					if (!toks.empty())
						addRow(block, toks, mpc);
					if (ends)
						block = Block::None;
				}
			}
		}
		else
		{
			bool ends = s.find(']') != std::string::npos;
			std::string row = s;
			for (char& c : row) if (c == ']' || c == ';') c = ' ';
			auto toks = tokenize(row);
			if (!toks.empty())
				addRow(block, toks, mpc);
			if (ends)
				block = Block::None;
		}
	}

	if (mpc._buses.empty())
	{
		err = "U fajlu nema mpc.bus podataka.";
		return false;
	}
	if (!haveBase)
		mpc._baseMVA = 100.0; // MATPOWER default

	mpc.rebuildIndex();
	return true;
}
