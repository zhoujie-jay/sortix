/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * vector.h
 * Vector class.
 */

#ifndef VECTOR_H
#define VECTOR_H

class Vector
{
public:
	float x;
	float y;
	float z;

public:
	Vector(float x = 0.f, float y = 0.f, float z = 0.f) : x(x), y(y), z(z) { }

	Vector& operator=(const Vector& rhs)
	{
		if ( this != &rhs ) { x = rhs.x; y = rhs.y; z = rhs.z; }
		return *this;
	}

	Vector& operator+=(const Vector& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	Vector& operator-=(const Vector& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

	Vector& operator*=(float scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	Vector& operator/=(float scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
		return *this;
	}

	const Vector operator+(const Vector& other) const
	{
		Vector ret(*this); ret += other; return ret;
	}

	const Vector operator-(const Vector& other) const
	{
		Vector ret(*this); ret -= other; return ret;
	}

	const Vector operator*(float scalar) const
	{
		Vector ret(*this); ret *= scalar; return ret;
	}

	const Vector operator/(float scalar) const
	{
		Vector ret(*this); ret /= scalar; return ret;
	}

	bool operator==(const Vector& other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}

	bool operator!=(const Vector& other) const
	{
		return !(*this == other);
	}

	float Dot(const Vector& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	const Vector Cross(const Vector& other) const
	{
		Vector ret(y * other.z - z * other.y,
		           z * other.x - x * other.z,
		           x * other.y - y * other.x);
		return ret;
	}

	float SquaredSize() const
	{
		return x*x + y*y + z*z;
	}

	float Size() const
	{
		return sqrtf(SquaredSize());
	}

	float DistanceTo(const Vector& other) const
	{
		return (other - *this).Size();
	}

	const Vector Normalize() const
	{
		float size = Size();
		if ( size == 0.0 ) { size = 1.0f; }
		return *this / size;
	}

	const Vector Rotate2D(float radians) const
	{
		float sinr = sinf(radians);
		float cosr = cosf(radians);
		float newx = x * cosr - y * sinr;
		float newy = x * sinr + y * cosr;
		return Vector(newx, newy);
	}

	const Vector Rotate2DAround(float radians, const Vector& off) const
	{
		return Vector(*this - off).Rotate2D(radians) + off;
	}
};

#endif
