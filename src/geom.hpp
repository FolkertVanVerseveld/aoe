/* Copyright 2020 Folkert van Verseveld. All rights reserved */
#include <type_traits>
#include <array>
#include <variant>
#include <vector>

#include <SDL2/SDL_opengl.h>

namespace genie {

template<typename T>
class Quadtree final {
public:
	struct Point final {
		T x, y;

		Point() : x(0), y(0) {}
		Point(T x, T y) : x(x), y(y) {}

		bool operator==(const Point &pt) {
			return x == pt.x && y == pt.y;
		}
	};

	struct AABB final {
		Point center;
		T hsize;

		AABB(T hsize) : center(), hsize(hsize) {}
		AABB(T x, T y, T hsize) : center(x, y), hsize(hsize) {}

		bool operator==(const AABB &bnds) {
			return center == bnds.center && hsize == bnds.hsize;
		}
	};

	struct Leaf final {
		// TODO add data stuff
		Point pos;
		size_t id;
	};

	struct Node final {
		AABB bounds;
		std::variant<std::vector<Leaf>, std::unique_ptr<Node[]>> data;

		Node(const AABB &bounds) : bounds(bounds), data() {}

		void resize(const AABB &bounds) {
			assert(data.index() == 0);
			this->bounds = bounds;
		}

		void show() const {
			auto x = bounds.center.x, y = bounds.center.y, sz = bounds.hsize;

			glVertex2f(x - sz, y - sz); glVertex2f(x + sz, y - sz);
			glVertex2f(x + sz, y - sz); glVertex2f(x + sz, y + sz);
			glVertex2f(x + sz, y + sz); glVertex2f(x - sz, y + sz);
			glVertex2f(x - sz, y + sz); glVertex2f(x - sz, y - sz);

			if (data.index() == 1) {
				auto *nodes = std::get<std::unique_ptr<Node[]>>(data).get();

				for (unsigned i = 0; i < 4; ++i)
					nodes[i].show();
			}
		}
	};

	Node root;

	Quadtree(T hsize) : root(hsize) {}

	void reset(AABB bounds) {
		root.resize(bounds);
	}

	void show() const {
		glColor3f(1, 0, 0);

		glBegin(GL_LINES);
		root.show();
		glEnd();
	}
};

void unproject(float dst[3], const float pos[3], const float modelview[16], const float projection[16], const int viewport[4]);

}
