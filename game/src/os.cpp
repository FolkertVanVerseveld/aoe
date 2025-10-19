#include "os.hpp"

#include "engine.hpp"

#include <cstdio>

#if _WIN32
#include <shellapi.h>
#elif __linux__
#include <unistd.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <optional>
#include <iostream>

extern char **environ;
#endif

namespace aoe {

#if _WIN32
void open_url(const char *url) {
	ShellExecute(NULL, NULL, url, NULL, NULL, SW_SHOW);
}
#elif __linux__
static bool is_executable_in_path(const std::string &prog) {
	const char *path = std::getenv("PATH");
	if (!path)
		return false;

	std::string p(path);

	for (size_t start = 0, end; start <= p.size(); start = end + 1) {
		end = p.find(':', start);
		if (end == std::string::npos)
			end = p.size();

		std::string dir = p.substr(start, end - start);
		if (!dir.empty()) {
			std::string candidate = dir + "/" + prog;
			if (access(candidate.c_str(), X_OK) == 0)
				return true;
		}
	}

	return false;
}

// Check if a process with exact name is running by scanning /proc/*/comm
static bool is_process_running(const std::string &name) {
	DIR *d = opendir("/proc");
	if (!d)
		return false;

	struct dirent *ent;
	while ((ent = readdir(d))) {
		if (ent->d_type != DT_DIR)
			continue;

		// skip non-numeric
		char *endptr = nullptr;
		long pid = strtol(ent->d_name, &endptr, 10);
		if (*endptr != '\0' || pid <= 0)
			continue;

		std::string commPath = std::string("/proc/") + ent->d_name + "/comm";
		int fd = open(commPath.c_str(), O_RDONLY);
		if (fd < 0)
			continue;

		char buf[256] = {0};
		ssize_t n = read(fd, buf, sizeof(buf) - 1);
		close(fd);

		if (n > 0) {
			// file ends with newline; strip it
			while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) --n;
			buf[n] = '\0';
			if (name == buf) {
				closedir(d);
				return true;
			}
		}
	}

	closedir(d);
	return false;
}

struct Browser {
	std::string exec;
	enum Kind { Firefox, ChromiumLike, OperaLike, Unknown } kind;
	std::vector<std::string> aliases; // process/executable names to check
};

static const std::vector<Browser> knownBrowsers = {
	{"firefox", Browser::Firefox, {"firefox"}},
	{"google-chrome", Browser::ChromiumLike, {"google-chrome", "chrome"}},
	{"chromium", Browser::ChromiumLike, {"chromium", "chromium-browser"}},
	{"brave-browser", Browser::ChromiumLike, {"brave", "brave-browser"}},
	{"microsoft-edge", Browser::ChromiumLike, {"microsoft-edge", "msedge"}},
	{"vivaldi", Browser::ChromiumLike, {"vivaldi"}},
	{"opera", Browser::OperaLike, {"opera"}}
};

static std::optional<Browser> resolve_from_env() {
	const char *env = std::getenv("BROWSER");
	if (!env || !*env)
		return std::nullopt;

	// $BROWSER may have args; take the first token
	std::string s(env);
	size_t sp = s.find_first_of(" \t");
	std::string candidate = sp == std::string::npos ? s : s.substr(0, sp);

	// Try to map to a known browser kind by name; otherwise unknown
	for (const auto &b : knownBrowsers) {
		if (candidate == b.exec)
			return b;

		for (const auto &al : b.aliases)
			if (candidate == al)
				return b;
	}
	// If it's executable, accept unknown kind
	if (is_executable_in_path(candidate))
		return Browser{candidate, Browser::Unknown, {candidate}};

	return std::nullopt;
}

static std::optional<Browser> detect_running_browser() {
	for (const auto &b : knownBrowsers)
		for (const auto &name : b.aliases)
			if (is_process_running(name))
				return b;

	return std::nullopt;
}

static std::optional<Browser> find_installed_browser() {
	for (const auto &b : knownBrowsers) {
		if (is_executable_in_path(b.exec))
			return b;

		// some distros only expose alias name in PATH
		for (const auto &al : b.aliases) {
			if (is_executable_in_path(al))
				return Browser{al, b.kind, b.aliases};
		}
	}
	return std::nullopt;
}

static std::vector<char*> build_argv(const Browser &b, const std::string &url, std::vector<std::string> &storage) {
	storage.clear();
	storage.push_back(b.exec);
	// sensible new-tab flags by family
	switch (b.kind) {
		case Browser::Firefox:
			storage.push_back("-new-tab");
			break;
		case Browser::ChromiumLike:
			storage.push_back("--new-tab");
			break;
		case Browser::OperaLike:
			storage.push_back("--newtab");
			break;
		default:
			// unknown: no flag; most will still open the URL
			break;
	}
	storage.push_back(url);
	storage.push_back(std::string()); // null terminator placeholder

	std::vector<char*> argv;
	argv.reserve(storage.size());
	for (auto &s : storage)
		argv.push_back(const_cast<char*>(s.c_str()));

	// ensure last is nullptr
	argv.back() = nullptr;
	return argv;
}

static bool launch_detached(const std::vector<char*> &argv) {
	// Double-fork daemonization with stdio to /dev/null
	pid_t pid = fork();
	if (pid < 0)
		return false;
	if (pid > 0)
		// parent
		return true;

	// child
	if (setsid() < 0 || (pid = fork()) < 0)
		_exit(127);
	if (pid == 0) {
		// child of child: detach stdio
		int nullfd = open("/dev/null", O_RDWR);
		if (nullfd >= 0) {
			dup2(nullfd, STDIN_FILENO);
			dup2(nullfd, STDOUT_FILENO);
			dup2(nullfd, STDERR_FILENO);
			if (nullfd > 2)
				close(nullfd);
		}
		// exec using posix_spawnp for cleaner env handling
		posix_spawn_file_actions_t actions;
		posix_spawn_file_actions_init(&actions);
		// already duped to /dev/null; no extra actions needed

		pid_t cpid;
		int rc = posix_spawnp(&cpid, argv[0], &actions, nullptr,
				const_cast<char* const*>(argv.data()), environ);
		posix_spawn_file_actions_destroy(&actions);
		_exit(rc == 0 ? 0 : 127);
	}
	_exit(0);
	return true;
}

// oh boy... why can't we have nice things on linux...
void open_url(const char *url) {
	/*
	 * try four approaches:
	 * 1. detect browser from env
	 * 2. detect browser through /proc
	 * 3. find browser in $PATH
	 * 4. pray xdg-open is installed
	 */
	std::optional<Browser> chosen;

	chosen = resolve_from_env();
	if (!chosen) chosen = detect_running_browser();
	if (!chosen) chosen = find_installed_browser();

	std::vector<std::string> arg_store;
	std::vector<char*> exec_argv;
	if (chosen) {
		exec_argv = build_argv(*chosen, url, arg_store);
	} else {
		arg_store = {"xdg-open", url, std::string()};
		exec_argv = {
			const_cast<char*>(arg_store[0].c_str()),
			const_cast<char*>(arg_store[1].c_str()),
			nullptr
		};
	}

	if (!launch_detached(exec_argv))
		eng->push_error(__func__, std::string("Cannot open url: ") + url);
}

#else
void open_url(const char *url) {
	fprintf(stderr, "%s: stub\n", __func__);
}
#endif

}
