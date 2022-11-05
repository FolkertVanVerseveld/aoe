#pragma once

#include <memory>
#include <unordered_map>
#include <deque>
#include <utility>
#include <stdexcept>
#include <string>

namespace aoe {

// https://www.boost.org/doc/libs/1_37_0/doc/html/hash/reference.html#boost.hash_combine
template <class T>
inline void hash_combine(std::size_t &seed, const T &v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

typedef unsigned RefCounter;
typedef std::pair<RefCounter, RefCounter> IdPoolRef;

static constexpr const IdPoolRef invalid_ref{ 0, 0 };

template<typename T> class IdPool final {
	struct RefHash {
		template<class T1, class T2> std::size_t operator()(const std::pair<T1, T2> &p) const {
			size_t h = std::hash<T1>{}(p.first);
			hash_combine(h, p.second);
			return h;
		}
	};
	std::unordered_map<IdPoolRef, T, RefHash> values;
	std::deque<RefCounter> next;
	RefCounter mod;
public:
	IdPool() : values(), next(), mod(0) {}

	template<class... Args>	auto emplace(Args&&... val) {
		RefCounter id = !next.empty() ? next.back() : mod;
		if (!id)
			id = 1;

		auto ins = values.emplace(std::piecewise_construct, std::forward_as_tuple(id, mod), std::forward_as_tuple(std::make_pair<>(id, mod), val...));

		++mod;
		if (!next.empty())
			next.pop_back();

		return ins;
	}

	// access
	T *try_get(const IdPoolRef &r) noexcept {
		auto it = values.find(r);
		return it != values.end() ? &it->second : nullptr;
	}

	T &at(const IdPoolRef &r) {
		return values.at(r);
	}

	bool try_invalidate(const IdPoolRef &r) noexcept {
		return erase(r) > 0;
	}

	void invalidate(const IdPoolRef &r) {
		if (!try_invalidate(r))
			throw std::runtime_error("idpool invalidate failed");
	}

	// iterators
	auto begin() noexcept { return values.begin(); }
	const auto begin() const noexcept { return values.begin(); }

	size_t erase(const IdPoolRef &r) {
		size_t n = values.erase(r);
		if (n)
			next.emplace_back(r.first);
		return n;
	}

	auto erase(typename decltype(values)::iterator it) {
		RefCounter id = it->first.first;
		auto i = values.erase(it);
		next.emplace_back(id);
		return i;
	}

	auto erase(typename decltype(values)::const_iterator it) {
		RefCounter id = it->first.first;
		auto i = values.erase(it);
		next.emplace_back(id);
		return i;
	}

	auto end() noexcept { return values.end(); }
	const auto end() const noexcept { return values.end(); }

	// state
	constexpr size_t size() const noexcept { return values.size(); }

	void clear() {
		values.clear();
		next.clear();
		// XXX should we also set mod to zero?
	}
};

}
