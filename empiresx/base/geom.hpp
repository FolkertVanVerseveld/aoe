/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

/*
 * Wrappers for basic geometry in euclidean space
 */

namespace genie {

/**
 * Simple minimal API for a point in two dimensional euclidean space.
 */
template<typename T> class Vector2 {
public:
	T x, y;

	constexpr Vector2<T>(T x=0, T y=0) noexcept : x(x), y(y) {}
	constexpr Vector2<T>(const Vector2<T> &other) noexcept : x(other.x), y(other.y) {}

	friend constexpr Vector2<T> operator+(Vector2<T> lhs, const Vector2<T> &rhs) noexcept {
		lhs += rhs;
		return lhs;
	}

	constexpr Vector2<T> &operator+=(const Vector2<T> &other) noexcept {
		x += other.x;
		y += other.y;
		return *this;
	}

	friend constexpr Vector2<T> operator-(Vector2<T> lhs, const Vector2<T> &rhs) noexcept {
		lhs -= rhs;
		return lhs;
	}

	constexpr Vector2<T> &operator-=(const Vector2<T> &other) noexcept {
		x -= other.x;
		y -= other.y;
		return *this;
	}

	/** Divide vector by scalar. It is undefined to divide by zero. */
	friend constexpr Vector2<T> operator/(Vector2<T> vec, T v) noexcept {
		vec /= v;
		return vec;
	}

	constexpr Vector2<T> &operator/=(T v) noexcept {
		x /= v;
		y /= v;
		return *this;
	}

	friend constexpr Vector2<T> operator*(Vector2<T> vec, T v) noexcept {
		vec *= v;
		return vec;
	}

	constexpr Vector2<T> &operator*=(T v) noexcept {
		x *= v;
		y *= v;
		return *this;
	}
};

}
