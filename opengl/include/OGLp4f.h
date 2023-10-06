#pragma once

#include "OGLp3f.h"

/** .
typical operations on 4 float objects,
except color use col4f for color
 */
class p4f
{
public:
	p4f();
	p4f(const p3f& rh);
	p4f(float x_, float y_, float z_, float w_ = 1.0f);
	p4f(double x_, double y_, double z_, double w_ = 1.0);
	p4f& reset();

	void set(float x_, float y_, float z_, float w_ = 1.0f);
	void set(double x_, double y_, double z_, double w_ = 1.0);

	p4f& operator =(const p4f& rh);
	p4f& operator =(const p3f& rh);
	p4f  operator+ (const p4f& rh) const;
	p4f& operator+=(const p4f& rh);
	p4f  operator- (const p4f& rh) const;
	p4f& operator-=(const p4f& rh);
	p4f  operator* (float f) const;
	p4f& operator*=(float f);
	p4f  operator/ (float f) const;
	p4f& operator/=(float f);

	operator float* ()	const;
	float* asfloat()	const;

	float x, y, z, w;
};

inline p4f::p4f()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	w = 1.0f;
}
inline p4f::p4f(const p3f& rh)
{
	x = rh.x;
	y = rh.y;
	z = rh.z;
	w = 1.0f;
}
inline p4f::p4f(float x_, float y_, float z_, float w_)
{
	x = x_;
	y = y_;
	z = z_;
	w = w_;
}
inline p4f::p4f(double x_, double y_, double z_, double w_)
{
	x = (float)x_;
	y = (float)y_;
	z = (float)z_;
	w = (float)w_;
}
inline p4f& p4f::reset()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	w = 1.0f;
	return *this;
}

inline void p4f::set(float x_, float y_, float z_, float w_)
{
	x = x_;
	y = y_;
	z = z_;
	w = w_;
}
inline void p4f::set(double x_, double y_, double z_, double w_)
{
	x = (float)x_;
	y = (float)y_;
	z = (float)z_;
	w = (float)w_;
}

inline p4f& p4f::operator =(const p4f& rh)
{
	x = rh.x;
	y = rh.y;
	z = rh.z;
	w = rh.w;
	return *this;
}
inline p4f& p4f::operator =(const p3f& rh)
{
	x = rh.x;
	y = rh.y;
	z = rh.z;
	w = 1.0f;
	return *this;
}
inline p4f  p4f::operator+ (const p4f& rh) const
{
	return p4f(x + rh.x, y + rh.y, z + rh.z, w + rh.w);
}
inline p4f& p4f::operator+=(const p4f& rh)
{
	x += rh.x;
	y += rh.y;
	z += rh.z;
	w += rh.w;
	return *this;
}
inline p4f  p4f::operator- (const p4f& rh) const
{
	return p4f(x - rh.x, y - rh.y, z - rh.z, w - rh.w);
}
inline p4f& p4f::operator-=(const p4f& rh)
{
	x -= rh.x;
	y -= rh.y;
	z -= rh.z;
	w -= rh.w;
	return *this;
}
inline p4f  p4f::operator* (float f) const
{
	return p4f(x * f, y * f, z * f, w * f);
}
inline p4f& p4f::operator*=(float f)
{
	x *= f;
	y *= f;
	z *= f;
	w *= f;
	return *this;
}
inline p4f  p4f::operator/ (float f) const
{
	return p4f(x / f, y / f, z / f, w / f);
}
inline p4f& p4f::operator/=(float f)
{
	x /= f;
	y /= f;
	z /= f;
	w /= f;
	return *this;
}

inline p4f::operator float* ()	const
{
	return (float*)&x;
}
inline float* p4f::asfloat()	const
{
	return (float*)&x;
}
