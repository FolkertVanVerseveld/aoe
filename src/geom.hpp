/* Copyright 2020 Folkert van Verseveld. All rights reserved */
#include <cassert>
#include <array>
#include <variant>
#include <vector>
#include <map>

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

template<typename T>
class Quadtree final {
public:
	/**
	 * Item placeholder in quadtree with reference counting and a unique identifier.
	 * Note that multiple nodes may contain the same leaf, as leaves can be larger than a node.
	 */
	struct Leaf final {
		// leaf may be rectangular, nodes must be square
		Point<T> pos, size;
		unsigned ref;
		/** Unique identifier that is externally defined. */
		size_t id;

		Leaf(size_t id, const Point<T> &pos, const Point<T> &size) : pos(pos), size(size), ref(0), id(id) {}
	};

	/**
	 * Squared axis aligned bounding box for collision detection.
	 * Only for internal use!
	 */
	struct AABB final {
		Point<T> center;
		T hsize;

		AABB(T hsize) : center(), hsize(hsize) {}
		AABB(T x, T y, T hsize) : center(x, y), hsize(hsize) {}

		constexpr bool operator==(const AABB &bnds) const noexcept {
			return center == bnds.center && hsize == bnds.hsize;
		}

		constexpr bool intersects(const Point<T> &center, const Point<T> &size) const noexcept {
			T w = size.x / 2 + this->hsize, h = size.y / 2 + this->hsize;
			T dx = center.x - this->center.x, dy = center.y - this->center.y;
			return w <= std::abs(dx) && h <= std::abs(dy);
		}

		constexpr bool intersects(const Leaf &l) const noexcept {
			return intersects(l.pos, l.size);
		}
	};

	struct Node final {
		AABB bounds;
		std::variant<std::vector<std::size_t>, std::unique_ptr<Node[]>> data;
		unsigned quadrant;
		Node *parent;
		Quadtree *qt;

		Node(const AABB &bounds, Node *parent, Quadtree *qt) : bounds(bounds), data(), quadrant(4), parent(parent), qt(qt) {}

		bool add(Leaf &l) {
			if (!bounds.intersects(l))
				return false;

			switch (data.index()) {
				case 0:
					std::get<std::vector<std::size_t>>(data).emplace_back(l.id);
					++l.ref;
					return true;
				case 1:
				{
					Node *nodes = std::get<std::unique_ptr<Node[]>>(data).get();
					bool added = false;

					for (unsigned i = 0; i < 4; ++i)
						if (nodes[i].add(l))
							added = true; // DON'T BREAK: other nodes may need to add leaf as well

					return added;
				}
				default:
					assert("bad index" == 0);
					break;
			}
			return false;
		}

		void resize(const AABB &bounds) {
			assert(parent == nullptr);

			if (data.index() == 0)
				std::get<std::vector<std::size_t>>(data).clear();
			else
				data.emplace<std::vector<std::size_t>>();

			this->bounds = bounds;
		}

		void show() const {
			auto x = bounds.center.x, y = bounds.center.y, sz = bounds.hsize;

			glVertex2f(x - sz, y - sz); glVertex2f(x + sz, y - sz);
			glVertex2f(x + sz, y - sz); glVertex2f(x + sz, y + sz);
			glVertex2f(x + sz, y + sz); glVertex2f(x - sz, y + sz);
			glVertex2f(x - sz, y + sz); glVertex2f(x - sz, y - sz);

			switch (data.index()) {
				case 0:
					break;
				case 1:
				{
					auto *nodes = std::get<std::unique_ptr<Node[]>>(data).get();

					for (unsigned i = 0; i < 4; ++i)
						nodes[i].show();
					break;
				}
				default:
					assert("bad index" == 0);
					break;
			}
		}
	};

	/** Quick lookup-table and container for reference counting. */
	std::map<size_t, Leaf> items;

	Node root;

	Quadtree(T hsize) : root(hsize, nullptr, this) {}

	void reset(AABB bounds) {
		root.resize(bounds);
	}

	bool add(size_t id, const Point<T> &pos, const Point<T> &size) {
		Leaf l(id, pos, size);

		if (!l.ref)
			return false;

		items.emplace(id, std::move(l));
		return true;
	}

	int try_erase(size_t id) {
		auto search = items.find(id);
		if (search == items.end())
			return -1;

		if (--search->ref)
			return 0;

		items.erase(search);
		return 1;
	}

	bool erase(size_t id) {
		int ret = try_erase(id);
		assert(ret != -1);
		return ret != 0;
	}

	void show() const {
		glColor3f(1, 0, 0);

		glBegin(GL_LINES);
		root.show();
		glEnd();

		glColor4f(0, 1, 1, 0.5);
		glBegin(GL_QUADS);

		for (auto it : items) {
			const Leaf &l = it.second;
			auto x = l.pos.x, y = l.pos.y, w = l.size.x, h = l.size.y;

			glVertex2f(x - w * .5f, y - h * .5f);
			glVertex2f(x + w * .5f, y - h * .5f);
			glVertex2f(x + w * .5f, y + h * .5f);
			glVertex2f(x - w * .5f, y + h * .5f);
		}

		glEnd();
	}
};

void unproject(float dst[3], const float pos[3], const float modelview[16], const float projection[16], const int viewport[4]);

}
