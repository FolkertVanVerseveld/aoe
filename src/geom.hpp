/* Copyright 2020 Folkert van Verseveld. All rights reserved */
#include <cassert>
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
struct AABB final {
	Point<T> center;
	T hsize;

	AABB() : center(), hsize(0) {}
	AABB(T hsize) : center(), hsize(hsize) {}
	AABB(T x, T y, T hsize) : center(x, y), hsize(hsize) {}

	constexpr bool operator==(const AABB &bnds) const noexcept {
		return center == bnds.center && hsize == bnds.hsize;
	}

#if 0
	constexpr bool intersects(const Point<T> &center, const Point<T> &size) const noexcept {
		T w = size.x / 2 + this->hsize, h = size.y / 2 + this->hsize;
		T dx = center.x - this->center.x, dy = center.y - this->center.y;
		return w <= std::abs(dx) && h <= std::abs(dy);
	}
#endif

	constexpr bool intersects(const AABB &bb) const noexcept {
		T dx = center.x - bb.center.x, dy = center.y - bb.center.y, sz = hsize + bb.hsize;
		return sz <= std::abs(dx) && sz <= std::abs(dy);
	}
};

template<typename Obj, typename GetBox, typename Float=float>
class Quadtree final {
	static_assert(std::is_convertible_v<std::invoke_result_t<GetBox, const Obj&>, AABB<Float>>,
		"GetBox must be a callable of signature AABB<Float>(const Obj&)");

public:
	GetBox getBox;

	static constexpr size_t threshold = 16;
	static constexpr size_t maxdepth = 8;

	struct Node final {
		std::variant<std::vector<std::shared_ptr<Obj>>, std::unique_ptr<Node[]>> data;

		Node() : data() {}

		void clear() {
			data.emplace<std::vector<std::shared_ptr<Obj>>();
		}

		void show_nodes(AABB<Float> bounds) const {
			auto x = bounds.center.x, y = bounds.center.y, sz = bounds.hsize;

			glVertex2f(x - sz, y - sz); glVertex2f(x + sz, y - sz);
			glVertex2f(x + sz, y - sz); glVertex2f(x + sz, y + sz);
			glVertex2f(x + sz, y + sz); glVertex2f(x - sz, y + sz);
			glVertex2f(x - sz, y + sz); glVertex2f(x - sz, y - sz);

			if (data.index() == 1) {
				auto *nodes = std::get<std::unique_ptr<Node[]>>(data).get();

				AABB<Float> bb[4];
				bb[0].hsize = bb[1].hsize = bb[2].hsize = bb[3].hsize = bounds.hsize / 2;
				auto ss = hsize / 4;
				bb[0].center.x = bounds.center.x + ss; bb[0].center.y = bounds.center.y + ss;
				bb[1].center.x = bounds.center.x - ss; bb[1].center.y = bounds.center.y + ss;
				bb[2].center.x = bounds.center.x - ss; bb[2].center.y = bounds.center.y - ss;
				bb[3].center.x = bounds.center.x + ss; bb[3].center.y = bounds.center.y - ss;

				for (unsigned i = 0; i < 4; ++i)
					nodes[i].show_nodes(bb[i]);
				break;
			}
		}
	};

	Node root;
	AABB<Float> bounds;

	Quadtree(AABB<Float> bounds) : root(), bounds(bounds) {}

	void reset(AABB<Float> bounds) {
		root.clear();
		this->bounds = bounds;
	}

	void show() const {
		glColor3f(1, 0, 0);

		glBegin(GL_LINES);
		root.show(bounds);
		glEnd();

#if 0
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
#endif
	}
};

void unproject(float dst[3], const float pos[3], const float modelview[16], const float projection[16], const int viewport[4]);

}
