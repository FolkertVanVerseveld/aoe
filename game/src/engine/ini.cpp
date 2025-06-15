#include "ini.hpp"

#include <cstdlib> // strtod
#include <cctype>  // tolower
#include <cstring> // strerror

IniParser::IniParser(const char *path) : f(path ? fopen(path, "rb") : NULL), cache() {}

IniParser::~IniParser() {
	if (f)
		fclose(f);
}

ipStatus IniParser::try_get(const char *section, const char *name, std::string &dst)
{
	if (find_cache(section, name, dst))
		return ipStatus::ok;

	char rbuf[4096];
	size_t in;

	fseek(f, SEEK_SET, 0);

	ipState state = ipState::find_data;
	std::string sectionname, key, value;

	for (bool eof = false; !eof;) {
		if ((in = fread(rbuf, 1, sizeof rbuf, f)) != sizeof rbuf) {
			if (ferror(f))
				return ipStatus::ioerr;

			eof = true;
		}

		for (size_t i = 0; i < in; ++i) {
			int ch = rbuf[i];

			switch (state) {
				case ipState::find_data:
					switch (ch) {
						case '[':
							sectionname.clear();
							state = ipState::in_section;
							break;
						case ';':
							state = ipState::in_comment;
							break;
						case ' ': case '\t': case '\r': case '\n':
							break;
						default:
							state = ipState::in_kvpair;
							key.clear();
							key += ch;
							break;
					}
					break;
				case ipState::in_comment:
					switch (ch) {
						case '\r': case '\n':
							for (size_t j = i + 1; j < in; ++j) {
								ch = rbuf[j];
								if (ch != '\r' && ch != '\n') {
									i = j - 1;
									break;
								}
							}
							state = ipState::find_data;
							break;
					}
					break;
				case ipState::in_section:
					switch (ch) {
						case '[':
							break;
						case ']':
							state = ipState::find_data;
							break;
						default:
							sectionname += ch;
							break;
					}
					break;
				case ipState::in_kvpair:
					switch (ch) {
						case '=':
							value.clear();
							if (i < in && rbuf[i + 1] == '"') {
								state = ipState::parse_qval;
								++i;
							} else {
								state = ipState::parse_value;
							}
							break;
						case ' ': case '\t': case '\r': case'\n':
							break;
						default:
							key += ch;
							break;
					}
					break;
				case ipState::parse_value:
				case ipState::parse_qval:
					switch (ch) {
						case ';':
							if (state == ipState::parse_qval) {
								value += ch;
								continue;
							}
						case '"':
							if (state != ipState::parse_qval)
								continue;
						case '\r': case '\n':
							if (state == ipState::parse_qval) {
								int nl = '\n';

								for (size_t j = i; j < in; ++j) {
									ch = rbuf[j];
									if (ch == '\r') {
										if (j + 1 < in && rbuf[j + 1] == '\n')
											continue;

										value += nl;
									} else if (ch == '\n') {
										value += nl;
									} else if (ch == '"') {
										i = j;
										break;
									} else {
										value += ch;
									}
								}
							}

							add_cache(sectionname, key, value);

							// check kv pair
							if (sectionname == section && key == name) {
								dst = value;
								return ipStatus::ok;
							}

							for (size_t j = i + 1; j < in; ++j) {
								ch = rbuf[j];
								if (ch != '\r' && ch != '\n') {
									i = j - 1;
									break;
								}
							}
							state = ch == ';' ? ipState::in_comment : ipState::find_data;
							break;
						default:
							value += ch;
							break;
					}
					break;
			}
		}

		if (state == ipState::parse_value) {
			add_cache(sectionname, key, value);

			// check kv pair
			if (sectionname == section && key == name) {
				dst = value;
				return ipStatus::ok;
			}
		}
	}

	return ipStatus::not_found;
}

double IniParser::get_or_default(const char *section, const char *key, double def)
{
	std::string value;

	switch (try_get(section, key, value)) {
		case ipStatus::ioerr:
			fprintf(stderr, "%s: failed to read: %s\n", __func__, strerror(errno));
			return def;
		case ipStatus::not_found:
			return def;
	}

	const char *str = value.c_str();
	char *end = NULL;

	double v = std::strtod(str, &end);
	if (end == str)
		return def;

	return v;
}

static int strcasecmp(const std::string &s1, const std::string &s2)
{
	size_t n1 = s1.size(), n2 = s2.size();
	if (n1 != n2)
		return n1 > n2 ? 1 : -1;

	for (size_t i = 0; i < n1; ++i) {
		int c1 = tolower(s1[i]), c2 = tolower(s2[i]);
		if (c1 != c2)
			return c1 > c2 ? 1 : -1;
	}

	return 0;
}

static bool streqcase(const std::string &s1, const std::string &s2)
{
	return strcasecmp(s1, s2) == 0;
}

bool IniParser::get_or_default(const char *section, const char *key, bool def)
{
	std::string value;

	if (try_get(section, key, value) != ipStatus::ok)
		return def;

	if (value == "1" || streqcase(value, "true") || streqcase(value, "on"))
		return true;

	if (value == "0" || streqcase(value, "false") || streqcase(value, "off"))
		return false;

	return def;
}

void IniParser::add_cache(const std::string &section, const std::string &key, const std::string &value)
{
	auto it = cache.find(section);
	if (it == cache.end()) {
		std::map<std::string, std::string> m{ { key, value } };
		cache.emplace(section, m);
	} else {
		it->second.try_emplace(key, value);
	}
}

void IniParser::add_cache(const std::string &section, const std::string &key, double value, unsigned decimals)
{
	if (decimals) {
		char fmt[8], buf[24];
		snprintf(fmt, sizeof fmt, "%%.%uf", decimals);
		snprintf(buf, sizeof buf, fmt, value);
		add_cache(section, key, buf);
	} else {
		add_cache(section, key, std::to_string(value));
	}
}

void IniParser::add_cache(const std::string &section, const std::string &key, const char *ptr)
{
	add_cache(section, key, std::string(ptr));
}

void IniParser::add_cache(const std::string &section, const std::string &key, bool value)
{
	add_cache(section, key, value ? "true" : "false");
}

bool IniParser::find_cache(const std::string &section, const std::string &key, std::string &dst)
{
	auto it = cache.find(section);
	if (it == cache.end())
		return false;

	auto it2 = it->second.find(key);
	if (it2 == it->second.end())
		return false;

	dst = it2->second;
	return true;
}

ipStatus IniParser::write_file(const char *path)
{
	FILE *f;

	if (!(f = fopen(path, "wb")))
		return ipStatus::ioerr;

	for (auto sv : cache) {
		if (!sv.first.empty())
			fprintf(f, "[%s]\n", sv.first.c_str());

		for (auto kv : sv.second) {
			const std::string &key = kv.first, &val = kv.second;
			if (val.find(';') != val.npos || val.find('\n') != val.npos)
				fprintf(f, "%s=\"%s\"\n", key.c_str(), val.c_str());
			else
				fprintf(f, "%s=%s\n", key.c_str(), val.c_str());
		}
	}

	fclose(f);

	return ipStatus::ok;
}