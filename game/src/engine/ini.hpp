#pragma once

#include <cstdio>

#include <map>
#include <string>

enum class ipStatus {
	ok,
	not_found,
	ioerr,
	invalid_value,
};

enum class ipState {
	find_data,
	in_section,
	in_comment,
	in_kvpair,
	parse_value,
	parse_qval,
};

class IniParser final {
	FILE *f;
	std::map<std::string, std::map<std::string, std::string>> cache;
public:
	IniParser(const char *path);
	~IniParser();

	bool exists() const noexcept { return !!f; }

	double get_or_default(const char *section, const char *key, double def);
	bool get_or_default(const char *section, const char *key, bool def);
	ipStatus try_get(const char *section, const char *name, std::string &dst);
	ipStatus try_get(const char *section, const char *name, double &dst);

	ipStatus try_clamp(const char *section, const char *name, double &dst, double min, double max);

	// dump cache to file. any keys that haven't been looked up and all comments will be lost
	ipStatus write_file(const char *path);
	void add_cache(const std::string &section, const std::string &key, const std::string &value);
	void add_cache(const std::string &section, const std::string &key, const char *ptr);
	void add_cache(const std::string &section, const std::string &key, double value, unsigned decimals=0);
	void add_cache(const std::string &section, const std::string &key, bool value);
private:
	bool find_cache(const std::string &section, const std::string &key, std::string &dst);
};