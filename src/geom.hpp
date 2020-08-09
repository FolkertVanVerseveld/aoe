/* Copyright 2020 Folkert van Verseveld. All rights reserved */
#include <cassert>

#include <type_traits>
#include <array>
#include <variant>
#include <vector>
#include <map>
#include <memory>

#include <SDL2/SDL_opengl.h>

/*
 * Geometric objects and geometric transformation functions
 */

namespace genie {

template<typename T>
struct Point final {
	T x, y;

	Point() : x(0), y(0) {}
	Point(T x, T y) : x(x), y(y) {}

	constexpr bool operator==(const Point &pt) const noexcept {
		return x == pt.x && y == pt.y;
	}
};

/** Squared axis aligned bounding box for collision detection. */
template<typename T>
struct Box final {
	Point<T> center;
	Point<T> hsize;

	Box() : center(), hsize() {}
	Box(T hw, T hh) : center(), hsize(hw, hh) {}
	Box(T x, T y, T hw, T hh) : center(x, y), hsize(hw, hh) {}

	constexpr bool operator==(const Box<T> &bnds) const noexcept {
		return center == bnds.center && hsize == bnds.hsize;
	}

	constexpr bool contains(const Point<T> &pos) const noexcept {
		return std::abs(center.x - pos.x) <= hsize.x && std::abs(center.y - pos.y) <= hsize.y;
	}

	constexpr bool intersects(const Box<T> &bb) const noexcept {
		T dx = center.x - bb.center.x, dy = center.y - bb.center.y;
		return std::abs(dx) <= hsize.x + bb.hsize.x && std::abs(dy) <= hsize.y + bb.hsize.y;
	}
};

template<typename Obj, typename GetBox, typename Equal, typename Coord=float>
class Quadtree final {
	static_assert(std::is_convertible_v<std::invoke_result_t<GetBox, const Obj&>, Box<Coord>>,
		"GetBox must be a callable of signature Box<Coord>(const Obj&)");
	static_assert(std::is_convertible_v<std::invoke_result_t<Equal, const Obj&, const Obj&>, bool>,
		"Equal must be a callable of signature bool(const Obj&, const Obj&)");
	static_assert(std::is_arithmetic_v<Coord>);

public:
	std::function<Box<Coord>(const Obj&)> getBox;
	std::function<bool(const Obj&, const Obj&)> equal;

	static constexpr size_t threshold = 16;
	size_t maxdepth;

	struct Node final {
		Box<Coord> bounds;
		std::variant<std::vector<Obj>, std::unique_ptr<Node[]>> data;

		Node() : bounds(), data() {}
		Node(Box<Coord> bounds) : bounds(bounds), data() {}

		constexpr size_t size(size_t threshold = (size_t)-1) const noexcept {
			if (data.index() == 0)
				return std::get<std::vector<Obj>>(data).size();

			auto *nodes = std::get<std::unique_ptr<Node[]>>(data);
			size_t count = 0;

			for (unsigned i = 0; i < 4; ++i)
				if ((count += nodes[i].size()) >= threshold)
					return count;

			return count;
		}

		/** Retrieve all objects that intersect with \a bounds and return number of retrieved objects. */
		size_t collect(std::vector<const Obj*> &lst, const Box<Coord> &bounds) const noexcept {
			if (!this->bounds.intersects(bounds))
				return 0;

			if (data.index() == 0) {
				auto &items = std::get<std::vector<Obj>>(data);

				for (size_t i = 0; i < items.size(); ++i)
					lst.emplace_back(items.data() + i);

				return items.size();
			}

			auto *nodes = std::get<std::unique_ptr<Node[]>>(data).get();
			size_t count = 0;

			for (unsigned i = 0; i < 4; ++i)
				count += nodes[i].collect(lst, bounds);

			return count;
		}

		void clear() {
			data.emplace<std::vector<Obj>>();
		}

		void resize(Box<Coord> bounds) noexcept {
			this->bounds = bounds;
		}

		void show_nodes() const {
			auto x = bounds.center.x, y = bounds.center.y, szx = bounds.hsize.x, szy = bounds.hsize.y;

			glVertex2f(x - szx, y - szy); glVertex2f(x + szx, y - szy);
			glVertex2f(x + szx, y - szy); glVertex2f(x + szx, y + szy);
			glVertex2f(x + szx, y + szy); glVertex2f(x - szx, y + szy);
			glVertex2f(x - szx, y + szy); glVertex2f(x - szx, y - szy);

			if (data.index() == 1) {
				auto *nodes = std::get<std::unique_ptr<Node[]>>(data).get();

				for (unsigned i = 0; i < 4; ++i)
					nodes[i].show_nodes();
			}
		}

		bool add(std::function<Box<Coord>(const Obj&)> getBox, Obj &obj, const Point<Coord> &pos, size_t depth, size_t threshold) {
			if (!bounds.contains(pos))
				return false;

			// if leaf
			if (data.index() == 0) {
				auto &lst = std::get<std::vector<Obj>>(data);

				if (!depth || lst.size() < threshold) {
					lst.emplace_back(std::move(obj));
					return true;
				}

				split(getBox, depth ? depth - 1 : 0, threshold);
				// readd
				return add(getBox, obj, pos, depth, threshold);
			}

			auto *nodes = std::get<std::unique_ptr<Node[]>>(data).get();

			if (depth)
				--depth;

			for (unsigned i = 0; i < 4; ++i)
				if (nodes[i].add(getBox, obj, pos, depth, threshold))
					return true;

			return false;
		}

		bool erase(std::function<bool(const Obj&, const Obj&)> equal, const Obj &obj, const Point<Coord> &pos, size_t threshold) {
			if (!bounds.contains(pos))
				return false;

			// if leaf
			if (data.index() == 0) {
				auto &lst = std::get<std::vector<Obj>>(data);
				auto it = std::find_if(std::begin(lst), std::end(lst), [this, &value](const auto &rhs) { return equal(value, rhs); });
				assert(it != std::end(lst));
				if (it == std::end(lst))
					return false;

				*it = std::move(lst.back());
				lst.pop_back();
				return true;
			}

			auto *nodes = std::get<std::unique_ptr<Node[]>>(data).get();

			for (unsigned i = 0; i < 4; ++i)
				if (nodes[i].erase(equal, obj, pos)) {
					// merge if too small
					if (size(threshold) <= threshold) {
						std::vector<Obj> items;
						collect(items, bounds);
						data.emplace<std::vector<Obj>>(std::move(items));
					}

					return true;
				}

			return false;
		}

		void split(std::function<Box<Coord>(const Obj&)> getBox, size_t depth, size_t threshold) {
			assert(data.index() == 0);

			// copy collect
			std::vector<Obj> items(std::get<std::vector<Obj>>(data));

			// compute bounds
			Box<Coord> bb[4];
			bb[0].hsize.x = bb[1].hsize.x = bb[2].hsize.x = bb[3].hsize.x = bounds.hsize.x / 2;
			bb[0].hsize.y = bb[1].hsize.y = bb[2].hsize.y = bb[3].hsize.y = bounds.hsize.y / 2;
			auto szx = bounds.hsize.x / 2, szy = bounds.hsize.y / 2;
			bb[0].center.x = bounds.center.x + szx; bb[0].center.y = bounds.center.y + szy;
			bb[1].center.x = bounds.center.x - szx; bb[1].center.y = bounds.center.y + szy;
			bb[2].center.x = bounds.center.x - szx; bb[2].center.y = bounds.center.y - szy;
			bb[3].center.x = bounds.center.x + szx; bb[3].center.y = bounds.center.y - szy;

			// do split, apply bounds
			data.emplace<std::unique_ptr<Node[]>>(new Node[4]);
			auto *nodes = std::get<std::unique_ptr<Node[]>>(data).get();

			nodes[0].resize(bb[0]);
			nodes[1].resize(bb[1]);
			nodes[2].resize(bb[2]);
			nodes[3].resize(bb[3]);

			// put items in children
			while (!items.empty()) {
				auto obj = items.back();
				Box<Coord> pos(getBox(obj));

				for (unsigned i = 0; i < 4; ++i)
					if (nodes[i].add(getBox, obj, pos.center, depth, threshold))
						break;

				items.pop_back();
			}
		}
	};

	// FIXME auto expand
	Node root;
	size_t count;

	Quadtree(const std::function<Box<Coord>(const Obj&)> f, const std::function<bool(const Obj&, const Obj&)> equal) : getBox(f), equal(equal), maxdepth(0), root(), count(0) {}

	void reset(Box<Coord> bounds, size_t maxdepth) {
		this->maxdepth = maxdepth;
		root.clear();
		root.resize(bounds);
		count = 0;
	}

	template<class... Args>
	bool try_emplace(Args&&... args) {
		Obj obj(args...);
		auto pos = getBox(obj).center;
		bool b = root.add(getBox, obj, pos, maxdepth, threshold);
		if (b)
			++count;
		return b;
	}

	bool erase(const Obj &obj) {
		auto pos = getBox(obj).center;
		bool b = root.erase(equal, obj, pos, threshold);
		if (b)
			--count;
		return b;
	}

	size_t collect(std::vector<const Obj*> &lst, const Box<Coord> &bounds) const noexcept {
		return root.collect(lst, bounds);
	}

	void show() const {
		glColor3f(1, 0, 0);

		glBegin(GL_LINES);
		root.show_nodes();
		glEnd();

		glColor4f(0, 1, 1, 0.5);
		glBegin(GL_QUADS);

		std::vector<const Obj*> items;
		root.collect(items, root.bounds);

		for (auto obj : items) {
			auto bounds = getBox(*obj);
			auto x = bounds.center.x, y = bounds.center.y, hw = bounds.hsize.x, hh = bounds.hsize.y;

			glVertex2f(x - hw, y - hh);
			glVertex2f(x + hw, y - hh);
			glVertex2f(x + hw, y + hh);
			glVertex2f(x - hw, y + hh);
		}

		glEnd();
	}
};

void unproject(float dst[3], const float pos[3], const float modelview[16], const float projection[16], const int viewport[4]);

}
