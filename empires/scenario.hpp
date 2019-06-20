/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include "cpn.h"
#include "scn.h"

#include <string>
#include <memory>

class Campaign final {
public:
	const std::string path;
	const char *name;
private:
	struct cpn cpn;
	void *data;
	size_t size;
public:
	Campaign(const char *path);
	~Campaign();
	const struct cpn_scn *scenarios(size_t &count) const;
};

struct ScenarioSettings final {
	bool pos_fixed, reveal, full_tech;

	ScenarioSettings() : pos_fixed(true), reveal(false), full_tech(false) {}
};
