#pragma once

class m33f;
class m44f;
class p3f;

/** .
basic geometric operations on a pair of floats
 */
class p2f
{
public:
	p2f();
	p2f(const p2f& rh);
	p2f(float x_, float y_);
	p2f(double x_, double y_);
	p2f(int x_, int y_);

	p2f& reset();
	p2f& set(float x_, float y_);
	p2f& set(double x_, double y_);
	p2f& lowUniClamp(float c);

	p2f   normalize();
	float sqlength();
	float length();
	float dot(const p2f& v);
	float rad(const p2f& v);
	float deg(const p2f& v);

	p2f& operator= (const p2f& rh);
	p2f  operator+ (const p2f& rh) const;
	p2f& operator+=(const p2f& rh);
	p2f  operator- (const p2f& rh) const;
	p2f& operator-=(const p2f& rh);
	p2f  operator* (float f) const;
	p2f& operator*=(float f);
	p2f  operator/ (float f) const;
	p2f& operator/=(float f);

	p2f  operator* (double d) const;
	p2f& operator*=(double d);
	p2f  operator/ (double d) const;
	p2f& operator/=(double d);

	p2f  operator -();

	bool operator==(p2f& rh) const;
	bool operator!=(p2f& rh) const;

	operator float* () const;
	const float* asfloat() const;
	const float& operator[](int idx) const;
	float& operator[](int idx);

	float x, y;
	long  tag;
};

/********************************************************************************/
/********************************** p2f *****************************************/
/********************************************************************************/

inline p2f::p2f()
{
	x = 0.0f;
	y = 0.0f;
}
inline p2f::p2f(const p2f& rh)
{
	x = rh.x;
	y = rh.y;
}
inline p2f::p2f(float x_, float y_)
{
	x = x_;
	y = y_;
}
inline p2f::p2f(double x_, double y_)
{
	x = float(x_);
	y = float(y_);
}
inline p2f::p2f(int x_, int y_)
{
	x = float(x_);
	y = float(y_);
}

inline p2f& p2f::reset()
{
	x = 0.0f;
	y = 0.0f;  return *this;
}
inline p2f& p2f::set(float x_, float y_)
{
	x = x_;
	y = y_;
	return *this;
}
inline p2f& p2f::set(double x_, double y_)
{
	x = float(x_);
	y = float(y_);
	return *this;
}
inline p2f& p2f::lowUniClamp(float c)
{
	if (fabs(x) < c || fabs(y) < c)
		x = y = c;
	return *this;
}

inline p2f   p2f::normalize()
{
	float len = this->length();
	assert(len > 0.0f);
	return *this = *this / len;
}
inline float p2f::sqlength()
{
	return x * x + y * y;
}
inline float p2f::length()
{
	return float(sqrt(sqlength()));
}
inline float p2f::dot(const p2f& v)
{
	return x * v.x + y * v.y;
}
inline float p2f::rad(const p2f& v)
{
	p2f v2(v);

	float r = this->normalize().dot(v2.normalize());
	return float(r >= -1.0f || r <= 1.0f ? acos(r) : 0);
}
inline float p2f::deg(const p2f& v)
{
	return Math::rad2deg * rad(v);
}

inline p2f& p2f::operator= (const p2f& rh)
{
	x = rh.x;
	y = rh.y;
	return *this;
}
inline p2f  p2f::operator+ (const p2f& rh) const
{
	return p2f(x + rh.x, y + rh.y);
}
inline p2f& p2f::operator+=(const p2f& rh)
{
	x += rh.x;
	y += rh.y;
	return *this;
}
inline p2f  p2f::operator- (const p2f& rh) const
{
	return p2f(x - rh.x, y - rh.y);
}
inline p2f& p2f::operator-=(const p2f& rh)
{
	x -= rh.x;
	y -= rh.y;
	return *this;
}
inline p2f  p2f::operator* (float f) const
{
	return p2f(x * f, y * f);
}
inline p2f& p2f::operator*=(float f)
{
	x *= f;
	y *= f;
	return *this;
}
inline p2f  p2f::operator/ (float f) const
{
	return p2f(x / f, y / f);
}
inline p2f& p2f::operator/=(float f)
{
	x /= f;
	y /= f;
	return *this;
}

inline p2f  p2f::operator* (double d) const
{
	return *this * float(d);
}
inline p2f& p2f::operator*=(double d)
{
	return *this *= float(d);
}
inline p2f  p2f::operator/ (double d) const
{
	return *this / float(d);
}
inline p2f& p2f::operator/=(double d)
{
	return *this /= float(d);
}

inline p2f  p2f::operator -()
{
	return p2f(-x, -y);
}

inline bool p2f::operator==(p2f& rh) const
{
	return fabs(x - rh.x) < Math::tol && fabs(y - rh.y) < Math::tol;
}
inline bool p2f::operator!=(p2f& rh) const
{
	return !(*this == rh);
}

inline p2f::operator float* () const
{
	return (float*)&x;
}
inline const float* p2f::asfloat() const
{
	return &x;
}
inline float& p2f::operator[](int idx)
{
	switch (idx)
	{
	case 0: return x; break;
	case 1: return y; break;
	default: assert(0);
	}
	return x;
}
