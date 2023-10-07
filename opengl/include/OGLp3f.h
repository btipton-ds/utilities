#pragma once

#include "OGLp2f.h"

/** .
basic operations for a triple of floats treated as vector or point,
except color use col4f for color
 */
class p3f
{
public:
	p3f();
	p3f(const p3f& rh);
	p3f(const p2f& rh);
	p3f(float x_, float y_, float z_);
	p3f(double x_, double y_, double z_);
	p3f(const float* p);

	bool equal(const p3f& rh) const;

	static p3f xaxis();
	static p3f yaxis();
	static p3f zaxis();

	p3f& reset();
	p3f& set(float x_, float y_, float z_);
	p3f& set(double x_, double y_, double z_);
	p3f& lowUniClamp(float c);

	p3f   normalize();
	float sqlength()		  const;
	float magnitudeSquared()  const;
	float length()			  const;
	float dot(const p3f& v) const;
	p3f	  cross(const p3f& v) const;
	p3f   rotax(p3f& v);
	float deg(const p3f& v) const;
	p3f   inverse()           const;
	p3f   floor()             const;
	p3f   ceil()              const;
	p3f   minimum(const p3f& rh)  const;
	p3f   maximum(const p3f& rh)  const;

	p3f& operator= (const p3f& rh);
	p3f  operator+ (const p3f& rh) const;
	p3f& operator+=(const p3f& rh);
	p3f  operator- (const p3f& rh) const;
	p3f& operator-=(const p3f& rh);
	p3f  operator* (float f) const;
	p3f& operator*=(float f);
	p3f  operator/ (float f) const;
	p3f& operator/=(float f);

	p3f  operator* (double d) const;
	p3f& operator*=(double d);
	p3f  operator/ (double d) const;
	p3f& operator/=(double d);

	p3f  operator -() const;

	bool operator==(const p3f& rh)	const;
	bool operator!=(const p3f& rh)	const;

	operator float* ()	const;
	float* asfloat()	const;

	float& operator[](int idx);

	const float& operator[](int idx) const;

	void set(float* gl44Mat);

	float x, y, z;
};

inline p3f operator* (const double s, const p3f& v) 
{ 
	return v * s; 
}

inline p3f::p3f()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
}
inline p3f::p3f(const p3f& rh)
{
	x = rh.x;
	y = rh.y;
	z = rh.z;
}
inline p3f::p3f(const p2f& rh)
{
	x = rh.x;
	y = rh.y;
	z = 0.0f;
}
inline p3f::p3f(float x_, float y_, float z_)
{
	x = x_;
	y = y_;
	z = z_;
}
inline p3f::p3f(double x_, double y_, double z_)
{
	x = float(x_);
	y = float(y_);
	z = float(z_);
}
inline p3f::p3f(const float* p)
{
	x = p[0];
	y = p[1];
	z = p[2];
}

inline bool p3f::equal(const p3f& rh) const
{
	return areEqual(x, rh.x) && areEqual(y, rh.y) && areEqual(z, rh.z);
}

inline static p3f xaxis()
{
	return p3f(1., 0., 0.);
}
inline static p3f yaxis()
{
	return p3f(0., 1., 0.);
}
inline static p3f zaxis()
{
	return p3f(0., 0., 1.);
}

inline p3f& p3f::reset()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	return *this;
}
inline p3f& p3f::set(float x_, float y_, float z_)
{
	x = x_;
	y = y_;
	z = z_;
	return *this;
}
inline p3f& p3f::set(double x_, double y_, double z_)
{
	x = float(x_);
	y = float(y_);
	z = float(z_);
	return *this;
}
inline p3f& p3f::lowUniClamp(float c)
{
	if (fabs(x) < c || fabs(y) < c || fabs(z) < c) x = y = z = c;
	return *this;
}

inline p3f   p3f::normalize()
{
	float len = this->length();
	if (len > 0.0f)
		return *this = *this / len;
	else
		return *this;
}
inline float p3f::sqlength()		  const
{
	return x * x + y * y + z * z;
}
inline float p3f::magnitudeSquared()  const
{
	return x * x + y * y + z * z;
}
inline float p3f::length()			  const
{
	return float(sqrt(sqlength()));
}
inline float p3f::dot(const p3f& v) const
{
	return x * v.x + y * v.y + z * v.z;
}
inline p3f	  p3f::cross(const p3f& v) const
{
	return p3f(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
}
inline p3f   p3f::rotax(p3f& v) {
	return this->normalize().cross(v.normalize());
}
inline float p3f::deg(const p3f& v) const {
	return (float) (Math::rad2deg * atan2(this->cross(v).length(), this->dot(v)));
}
inline p3f   p3f::inverse()           const
{
	return p3f(-x, -y, -z);
}
inline p3f   p3f::floor()             const
{
	return p3f(::floor(x), ::floor(y), ::floor(z));
}
inline p3f   p3f::ceil()              const
{
	return p3f(::ceil(x), ::ceil(y), ::ceil(z));
}
inline p3f   p3f::minimum(const p3f& rh)  const
{
	return p3f(x < rh.x ? x : rh.x, y < rh.y ? y : rh.y, z < rh.z ? z : rh.z);
}
inline p3f   p3f::maximum(const p3f& rh)  const
{
	return p3f(x > rh.x ? x : rh.x, y > rh.y ? y : rh.y, z > rh.z ? z : rh.z);
}

inline p3f& p3f::operator= (const p3f& rh)
{
	x = rh.x;
	y = rh.y;
	z = rh.z;
	return *this;
}
inline p3f  p3f::operator+ (const p3f& rh) const
{
	return p3f(x + rh.x, y + rh.y, z + rh.z);
}
inline p3f& p3f::operator+=(const p3f& rh)
{
	x += rh.x;
	y += rh.y;
	z += rh.z;
	return *this;
}
inline p3f  p3f::operator- (const p3f& rh) const
{
	return p3f(x - rh.x, y - rh.y, z - rh.z);
}
inline p3f& p3f::operator-=(const p3f& rh)
{
	x -= rh.x;
	y -= rh.y;
	z -= rh.z;
	return *this;
}
inline p3f  p3f::operator* (float f) const
{
	return p3f(x * f, y * f, z * f);
}
inline p3f& p3f::operator*=(float f)
{
	x *= f;
	y *= f;
	z *= f;
	return *this;
}
inline p3f  p3f::operator/ (float f) const
{
	return p3f(x / f, y / f, z / f);
}
inline p3f& p3f::operator/=(float f)
{
	x /= f;
	y /= f;
	z /= f;
	return *this;
}

inline p3f  p3f::operator* (double d) const
{
	return *this * float(d);
}
inline p3f& p3f::operator*=(double d)
{
	return *this *= float(d);
}
inline p3f  p3f::operator/ (double d) const
{
	return *this / float(d);
}
inline p3f& p3f::operator/=(double d)
{
	return *this /= float(d);
}

inline p3f  p3f::operator -() const
{
	return p3f(-x, -y, -z);
}

inline bool p3f::operator==(const p3f& rh)	const
{
	return (fabs(x - rh.x) < Math::tol) && (fabs(y - rh.y) < Math::tol) && (fabs(z - rh.z) < Math::tol);
}
inline bool p3f::operator!=(const p3f& rh)	const
{
	return !(*this == rh);
}

inline p3f::operator float* ()	const
{
	return (float*)&x;
}
inline float* p3f::asfloat()	const
{
	return (float*)&x;
}

inline float& p3f::operator[](int idx)
{
	switch (idx)
	{
	case 0: return x; break;
	case 1: return y; break;
	case 2: return z; break;
	default: assert(0);
	}
	return x;
}

inline const float& p3f::operator[](int idx) const
{
	switch (idx)
	{
	case 0: return x; break;
	case 1: return y; break;
	case 2: return z; break;
	default: assert(0);
	}
	return x;
}

inline void p3f::set(float* gl44Mat)
{
	float* cgl = gl44Mat;
	x = cgl[12];  y = cgl[13];  z = cgl[14];
}
