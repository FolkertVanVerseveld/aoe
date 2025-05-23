#include "legacy.hpp"

#include "../engine.hpp"

#include <cassert>

namespace aoe {
namespace io {

static std::string load_string(PE &dll, StrId id) {
	return dll.load_string((res_id)id);
}

void LanguageData::civ_add(PE &dll, res_id &tbl, StrId civid) {
	std::string civname = load_string(dll, civid);
	auto ins = civs.emplace(std::piecewise_construct, std::forward_as_tuple(civname), std::forward_as_tuple());
	assert(ins.second);

	std::vector<std::string> &names = civs.at(civname);
	
	int count = std::stoi(dll.load_string(tbl));
	int max = 10;

	if (count >= max) // number is included, so max - 1 is the limit
		throw std::runtime_error("civ names overflow");

	for (int i = 0; i < count; ++i) {
		std::string name = dll.load_string(tbl + 1 + i);
		if (!name.empty())
			names.emplace_back(name);
	}

	this->tbl[civid] = load_string(dll, civid);

	// adjust for next group
	tbl += max;
}

void LanguageData::collect_civs(std::vector<std::string> &lst) {
	lst.clear();
	for (auto kv : civs)
		lst.emplace_back(kv.first);
}

void LanguageData::load(PE &dll) {
	civs.clear();
	tbl.clear();
	age_names.clear();

#define loadstr(id) tbl[id] = load_string(dll, id)
	// main menu
	loadstr(StrId::main_copy1);
	loadstr(StrId::main_copy2a);
	loadstr(StrId::main_copy2b);
	loadstr(StrId::main_copy3);

	loadstr(StrId::btn_cancel);

	// ages
	age_names.emplace_back(loadstr(StrId::age_nomad));
	age_names.emplace_back(loadstr(StrId::age_stone));
	age_names.emplace_back(loadstr(StrId::age_tool));
	age_names.emplace_back(loadstr(StrId::age_bronze));
	age_names.emplace_back(loadstr(StrId::age_iron));
	age_names.emplace_back(loadstr(StrId::age_postiron));

	// civilizations
	res_id tbl = (res_id)StrId::btn_civtbl;

	civ_add(dll, tbl, StrId::civ_egyptian);
	civ_add(dll, tbl, StrId::civ_greek);
	civ_add(dll, tbl, StrId::civ_babylonian);
	civ_add(dll, tbl, StrId::civ_assyrian);
	civ_add(dll, tbl, StrId::civ_minoan);
	civ_add(dll, tbl, StrId::civ_hittite);
	civ_add(dll, tbl, StrId::civ_phoenician);
	civ_add(dll, tbl, StrId::civ_sumerian);
	civ_add(dll, tbl, StrId::civ_persian);
	civ_add(dll, tbl, StrId::civ_shang);
	civ_add(dll, tbl, StrId::civ_yamato);
	civ_add(dll, tbl, StrId::civ_choson);
#undef loadstr

	collect_civs(civ_names);
}

std::string LanguageData::find(StrId id) {
	return find(id, "???");
}

std::string LanguageData::find(StrId id, const std::string &def) {
	auto it = tbl.find(id);
	return it == tbl.end() ? def : it->second;
}

}

std::string Engine::txt(StrId id) {
	return old_lang.find(id);
}

}
