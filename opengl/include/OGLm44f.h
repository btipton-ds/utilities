#pragma once

#include "OGLm33f.h"


/** .
typical operations on a 4x4 matrix of floats
 */

class m44f
{
public:
	m44f();
	m44f(const p3f& a, const p3f& b, const p3f& c);
	m44f(const p4f& a, const p4f& b, const p4f& c, const p4f& d);
	m44f(const m33f& rot);
	//	m44f( const m33f& rot, const p3f& tra);
	m44f(const m33f& rot, const p3f& tra);

	p4f operator*(const p4f& inVec) const;
	p3f operator*(const p3f& inVec) const;
	m44f  transpose() const;
	bool isIdentity();
	m44f& identity();
	operator float* () const;
	float* transposef() const;
	void set(float* gl44Mat);
	p4f& operator [] (int idx);
	const p4f& operator [] (int idx)const;

	static m44f scale(float x, float y, float z);
	static m44f scale(double x, double y, double z);
	static m44f scale(float s);
	static m44f scale(double s);
	static m44f scale(const p3f& xyz);
	m44f operator*=(const m44f& m);
	m44f operator* (const m44f& m) const;

	m44f inverse() const;

	m33f rotationalDifference(const m44f& rh) const;

private:
	float gl[16] = {};
};

p3f operator *(const p3f& lhs, const m44f& rhs);


inline m44f::m44f() 
{ 
	identity(); 
}
inline m44f::m44f(const p3f& a, const p3f& b, const p3f& c)
{ 
	p4f* v = (p4f*)gl;
	v[0] = a; 
	v[0].w = 0.0f; 
	v[1] = b; 
	v[1].w = 0.0f; 
	v[2] = c; 
	v[2].w = 0.0f; 
	v[3].reset(); 
}
inline m44f::m44f(const p4f& a, const p4f& b, const p4f& c, const p4f& d)
{ 
	p4f* v = (p4f*)gl;
	v[0] = a;
	v[1] = b; 
	v[2] = c;  
	v[3] = d; 
}
inline m44f::m44f(const m33f& rot)
{ 
	p4f* v = (p4f*)gl;
	v[0] = rot.v[0];
	v[0].w = 0.0f; 
	v[1] = rot.v[1]; 
	v[1].w = 0.0f; 
	v[2] = rot.v[2]; 
	v[2].w = 0.0f; 
	v[3].reset(); 
}
//	m44f::m44f( const m33f& rot, const p3f& tra) { v[0]=rot.v[0]; v[0].w=tra.x; v[1]=rot.v[1]; v[1].w=tra.y; v[2]=rot.v[2]; v[2].w=tra.z; v[3].reset();}
inline m44f::m44f(const m33f& rot, const p3f& tra)
{ 
	p4f* v = (p4f*)gl;
	v[0] = rot.v[0];
	v[0].w = 0.0f; 
	v[1] = rot.v[1]; 
	v[1].w = 0.0f; 
	v[2] = rot.v[2]; 
	v[2].w = 0.0f; 
	v[3] = tra; 
	v[3].w = 1.0f; 
}

inline p4f m44f::operator*(const p4f& inVec) const
{
	p4f* v = (p4f*)gl;
	p4f retVec(0.0f, 0.0f, 0.0f, 0.0f);
	retVec.x = v[0].x * inVec.x + v[0].y * inVec.y + v[0].z * inVec.z + v[0].w * inVec.w;
	retVec.y = v[1].x * inVec.x + v[1].y * inVec.y + v[1].z * inVec.z + v[1].w * inVec.w;
	retVec.z = v[2].x * inVec.x + v[2].y * inVec.y + v[2].z * inVec.z + v[2].w * inVec.w;
	retVec.w = v[3].x * inVec.x + v[3].y * inVec.y + v[3].z * inVec.z + v[3].w * inVec.w;
	return retVec;
}

inline p3f m44f::operator*(const p3f& inVec) const
{
	p4f* v = (p4f*)gl;
	p3f retVec(0.0f, 0.0f, 0.0f);
	retVec.x = v[0].x * inVec.x + v[0].y * inVec.y + v[0].z * inVec.z;
	retVec.y = v[1].x * inVec.x + v[1].y * inVec.y + v[1].z * inVec.z;
	retVec.z = v[2].x * inVec.x + v[2].y * inVec.y + v[2].z * inVec.z;

	return retVec;
}

inline m44f  m44f::transpose() const
{
	p4f* v = (p4f*)gl;
	m44f x;
	x[0].x = v[0].x;  x[0].y = v[1].x;  x[0].z = v[2].x;  x[0].w = v[3].x;
	x[1].x = v[0].y;  x[1].y = v[1].y;  x[1].z = v[2].y;  x[1].w = v[3].y;
	x[2].x = v[0].z;  x[2].y = v[1].z;  x[2].z = v[2].z;  x[2].w = v[3].z;
	x[3].x = v[0].w;  x[3].y = v[1].w;  x[3].z = v[2].w;  x[3].w = v[3].w;
	return x;
}

inline bool m44f::isIdentity()
{
	p4f* v = (p4f*)gl;
	return
		areEqual(v[0].x, 1.0f) && areEqual(v[0].y, 0.0f) && areEqual(v[0].z, 0.0f) && areEqual(v[0].w, 0.0f) &&
		areEqual(v[1].x, 0.0f) && areEqual(v[1].y, 1.0f) && areEqual(v[1].z, 0.0f) && areEqual(v[1].w, 0.0f) &&
		areEqual(v[2].x, 0.0f) && areEqual(v[2].y, 0.0f) && areEqual(v[2].z, 1.0f) && areEqual(v[2].w, 0.0f) &&
		areEqual(v[3].x, 0.0f) && areEqual(v[3].y, 0.0f) && areEqual(v[3].z, 0.0f) && areEqual(v[3].w, 1.0f);
}

inline m44f& m44f::identity()
{
	p4f* v = (p4f*)gl;
	v[0].x = 1.0f;  v[0].y = 0.0f;  v[0].z = 0.0f;  v[0].w = 0.0f;
	v[1].x = 0.0f;  v[1].y = 1.0f;  v[1].z = 0.0f;  v[1].w = 0.0f;
	v[2].x = 0.0f;  v[2].y = 0.0f;  v[2].z = 1.0f;  v[2].w = 0.0f;
	v[3].x = 0.0f;  v[3].y = 0.0f;  v[3].z = 0.0f;  v[3].w = 1.0f;
	return *this;
}
inline m44f::operator float* () const
{
	p4f* v = (p4f*)gl;
	float* cgl = ((m44f*)this)->gl; // breaking the const rule
	cgl[0] = v[0].x; cgl[1] = v[0].y; cgl[2] = v[0].z; cgl[3] = v[0].w;
	cgl[4] = v[1].x; cgl[5] = v[1].y; cgl[6] = v[1].z; cgl[7] = v[1].w;
	cgl[8] = v[2].x; cgl[9] = v[2].y; cgl[10] = v[2].z; cgl[11] = v[2].w;
	cgl[12] = v[3].x; cgl[13] = v[3].y; cgl[14] = v[3].z; cgl[15] = v[3].w;

	return cgl;
}

inline float* m44f::transposef() const
{
	p4f* v = (p4f*)gl;
	float* cgl = ((m44f*)this)->gl; // breaking the const rule
	cgl[0] = v[0].x; cgl[4] = v[0].y; cgl[8] = v[0].z; cgl[12] = v[0].w;
	cgl[1] = v[1].x; cgl[5] = v[1].y; cgl[9] = v[1].z; cgl[13] = v[1].w;
	cgl[2] = v[2].x; cgl[6] = v[2].y; cgl[10] = v[2].z; cgl[14] = v[2].w;
	cgl[3] = v[3].x; cgl[7] = v[3].y; cgl[11] = v[3].z; cgl[15] = v[3].w;

	return cgl;
}

inline void m44f::set(float* gl44Mat)
{
	p4f* v = (p4f*)gl;
	float* cgl = gl44Mat;
	v[0].x = cgl[0];  v[0].y = cgl[4];  v[0].z = cgl[8];  v[0].w = cgl[12];
	v[1].x = cgl[1];  v[1].y = cgl[5];  v[1].z = cgl[9];  v[1].w = cgl[13];
	v[2].x = cgl[2];  v[2].y = cgl[6];  v[2].z = cgl[10];  v[2].w = cgl[14];
	v[3].x = cgl[3];  v[3].y = cgl[7];  v[3].z = cgl[11];  v[3].w = cgl[15];
}

inline p4f& m44f::operator [] (int idx)
{ 
	p4f* v = (p4f*)gl;
	assert(idx >= 0 && idx < 4);
	return v[idx]; 
}
inline const p4f& m44f::operator [] (int idx)const
{ 
	p4f* v = (p4f*)gl;
	assert(idx >= 0 && idx < 4);
	return v[idx]; 
}

inline m44f m44f::scale(float x, float y, float z)
{ 
	return scale(p3f(x, y, z)); 
}
inline m44f m44f::scale(double x, double y, double z)
{ 
	return scale(p3f(x, y, z)); 
}
inline m44f m44f::scale(float s)
{ 
	return scale(p3f(s, s, s)); 
}
inline m44f m44f::scale(double s)
{ 
	return scale(p3f(s, s, s)); 
}
inline m44f m44f::scale(const p3f& xyz)
{
	m33f scaleMat;
	scaleMat.v[0][0] = xyz.x;
	scaleMat.v[1][1] = xyz.y;
	scaleMat.v[2][2] = xyz.z;
	return scaleMat;
}
inline m44f m44f::operator*=(const m44f& m)
{ 
	return *this = *this * m; 
}

inline m44f m44f::operator* (const m44f& m) const
{
	p4f* v = (p4f*)gl;
	p4f* mv = (p4f*)m.gl;
	return m44f(p4f(v[0].x * mv[0].x + v[0].y * mv[1].x + v[0].z * mv[2].x + v[0].w * mv[3].x,
		v[0].x * mv[0].y + v[0].y * mv[1].y + v[0].z * mv[2].y + v[0].w * mv[3].y,
		v[0].x * mv[0].z + v[0].y * mv[1].z + v[0].z * mv[2].z + v[0].w * mv[3].z,
		v[0].x * mv[0].w + v[0].y * mv[1].w + v[0].z * mv[2].w + v[0].w * mv[3].w),

		p4f(v[1].x * mv[0].x + v[1].y * mv[1].x + v[1].z * mv[2].x + v[1].w * mv[3].x,
			v[1].x * mv[0].y + v[1].y * mv[1].y + v[1].z * mv[2].y + v[1].w * mv[3].y,
			v[1].x * mv[0].z + v[1].y * mv[1].z + v[1].z * mv[2].z + v[1].w * mv[3].z,
			v[1].x * mv[0].w + v[1].y * mv[1].w + v[1].z * mv[2].w + v[1].w * mv[3].w),

		p4f(v[2].x * mv[0].x + v[2].y * mv[1].x + v[2].z * mv[2].x + v[2].w * mv[3].x,
			v[2].x * mv[0].y + v[2].y * mv[1].y + v[2].z * mv[2].y + v[2].w * mv[3].y,
			v[2].x * mv[0].z + v[2].y * mv[1].z + v[2].z * mv[2].z + v[2].w * mv[3].z,
			v[2].x * mv[0].w + v[2].y * mv[1].w + v[2].z * mv[2].w + v[2].w * mv[3].w),

		p4f(v[3].x * mv[0].x + v[3].y * mv[1].x + v[3].z * mv[2].x + v[3].w * mv[3].x,
			v[3].x * mv[0].y + v[3].y * mv[1].y + v[3].z * mv[2].y + v[3].w * mv[3].y,
			v[3].x * mv[0].z + v[3].y * mv[1].z + v[3].z * mv[2].z + v[3].w * mv[3].z,
			v[3].x * mv[0].w + v[3].y * mv[1].w + v[3].z * mv[2].w + v[3].w * mv[3].w));
};

inline p3f operator *(const p3f& lhs, const m44f& rhs)
{
	m44f inv = rhs.inverse();
	return inv * lhs;
}

inline m44f m44f::inverse() const
{
	m44f a(*this); // As a evolves from original mat into identity
	m44f b;   // b evolves from identity into inverse(a)
	p4f* av = (p4f*)a.gl;
	p4f* bv = (p4f*)b.gl;
	int  i, j, i1;

	// Loop over cols of a from left to right, eliminating above and below diag
	for (j = 0; j < 4; j++)	  // Find largest pivot in column j among rows j..2
	{
		i1 = j;             // Row with largest pivot candidate
		for (i = j + 1; i < 4; i++)
		{
			if (fabs(av[i][j]) > fabs(av[i1][j]))
			{
				i1 = i;

				// Swap rows i1 and j in a and b to p pivot on diagonal
				p4f tmp = av[j];
				av[j] = av[i1]; av[i1] = tmp; // swap(av[i1], av[j]);
				tmp = bv[j];
				bv[j] = bv[i1]; bv[i1] = tmp; // swap(bv[i1], bv[j]);
			}
		}
		assert(av[j][j] != 0);

		bv[j] = bv[j] / av[j][j];
		av[j] = av[j] / av[j][j];

		// Eliminate off-diagonal elems in col j of a, doing identical ops to b
		for (i = 0; i < 4; i++)
		{
			if (i != j)
			{
				bv[i] = bv[i] - bv[j] * av[i][j];
				av[i] = av[i] - av[j] * av[i][j];
			}
		}
	}
	return b;
}

inline m33f m44f::rotationalDifference(const m44f& rh) const
{
	// take two vectors and transform them, then extract the rotational component from the vectors
	p3f o(0.0, 0.0, 0.0);
	p3f z(0.0, 0.0, 1.0);
	p3f op = o * rh;
	p3f zp = z * rh;
	zp -= op; // remove translational component

	p3f axis = z.rotax(zp);
	float angle = z.deg(zp);

	// this is OK for x & y but it's missing rotation around the z axis
	m33f rotDiffXY = m33f::axangle(axis, angle);

	// now do the same with y axis
	p3f y(0.0, 1.0, 0.0);
	p3f yp = y * rh;
	yp -= op;            // remove translational component
	yp = yp * m33f::axangle(axis, -angle); // remove xy rotational component

	// find remaining component of rotation
	axis = y.rotax(yp);
	angle = y.deg(yp);
	m33f rotDiffZ = m33f::axangle(axis, angle);

	return rotDiffXY * rotDiffZ;
}