#pragma once

#include "OGLp3f.h"

/** .
typical operations on a 3x3 matrix of floats
 */
class m33f
{
public:
	m33f();
	m33f(const p3f& a, const p3f& b, const p3f& c);
	m33f(const m33f& rh);
	m33f(const m44f& rh); //extracts rotational component of a 4x4
	m33f(float  Xrot, float  Yrot, float  Zrot);
	m33f(double Xrot, double Yrot, double Zrot);
	m33f(p3f& euler);

	bool equal(const m33f& rh) const;

	float length() const;

	static m33f angle(const p3f& xyz);
	static m33f angle(float  x, float  y, float  z);
	static m33f angle(double x, double y, double z);

	static m33f axangle(p3f& axis, float angle);

	p3f euler(const p3f& start_euler = p3f(0.0f, 0.0f, 0.0f)) const;
	p3f computeEulerAngles() const; // this assumes X -> Y -> Z rotation order

	m33f transpose() const;

	static m33f scale(float x, float y, float z);
	static m33f scale(double x, double y, double z);
	static m33f scale(const p3f& xyz);

	static m33f scale(float s);

	static m33f scale(double s);

	void identity();

	m33f& operator=(const m33f& rh);

	m33f& operator*=(const m33f& m);

	p3f diagonal() const;

	m33f  operator-(const m33f& rh) const;

	m33f operator*(const m33f& m) const;

	m33f inverse() const;

	p3f& operator [] (int idx);
	const p3f& operator [] (int idx) const;

	operator float* () const;

	float* transposef() const;

	void set(float* gl44Mat);

	p3f v[3];
private:
	float gl[16];
};


/********************************************************************************/
/********************************** m33f *****************************************/
/********************************************************************************/


inline m33f::m33f()
{
	identity();
}
inline m33f::m33f(const p3f& a, const p3f& b, const p3f& c)
{
	v[0] = a;
	v[1] = b;
	v[2] = c;
}
inline m33f::m33f(const m33f& rh)
{
	v[0] = rh.v[0];
	v[1] = rh.v[1];
	v[2] = rh.v[2];
}
inline m33f::m33f(const m44f& rh)  //extracts rotational component of a 4x4
{
}
inline m33f::m33f(float  Xrot, float  Yrot, float  Zrot)
{
	*this = angle(p3f(Xrot, Yrot, Zrot));
}
inline m33f::m33f(double Xrot, double Yrot, double Zrot)
{
	*this = angle(p3f(Xrot, Yrot, Zrot));
}
inline m33f::m33f(p3f& euler)
{
	*this = angle(euler);
}

inline bool m33f::equal(const m33f& rh) const
{
	return v[0].equal(rh.v[0]) && v[1].equal(rh.v[1]) && v[2].equal(rh.v[2]);
}

inline float m33f::length() const
{
	return sqrtf(v[0].sqlength() + v[1].sqlength() + v[2].sqlength());
}

inline m33f m33f::angle(const p3f& xyz)
{
	float xr = xyz.x * Math::deg2rad;
	float yr = xyz.y * Math::deg2rad;
	float zr = xyz.z * Math::deg2rad;

	float A = (float)cos(xr);
	float B = (float)sin(xr);
	float C = (float)cos(yr);
	float D = (float)sin(yr);
	float E = (float)cos(zr);
	float F = (float)sin(zr);

	float AD = A * D;
	float BD = B * D;

	m33f rot;
	rot[0][0] = C * E;
	rot[0][1] = -C * F;
	rot[0][2] = -D;
	rot[1][0] = -BD * E + A * F;
	rot[1][1] = BD * F + A * E;
	rot[1][2] = -B * C;
	rot[2][0] = AD * E + B * F;
	rot[2][1] = -AD * F + B * E;
	rot[2][2] = A * C;

	return rot;
}

inline m33f m33f::angle(float  x, float  y, float  z)
{
	return angle(p3f(x, y, z));
}
inline m33f m33f::angle(double x, double y, double z)
{
	return angle(p3f(x, y, z));
}

inline m33f m33f::axangle(p3f& axis, float angle)
{
	p3f ax = axis.normalize();
	float x = ax.x, y = ax.y, z = ax.z;

	float rad = angle * Math::deg2rad;
	float c = (float)cos(rad);
	float s = (float)sin(rad);
	float C = 1.0f - c;

	float xs = x * s, ys = y * s, zs = z * s;
	float xC = x * C, yC = y * C, zC = z * C;
	float xyC = x * yC, yzC = y * zC, zxC = z * xC;

	return m33f(
		p3f(x * xC + c, xyC - zs, zxC + ys),
		p3f(xyC + zs, y * yC + c, yzC - xs),
		p3f(zxC - ys, yzC + xs, z * zC + c)
	);
}

inline p3f m33f::euler(const p3f& start_euler) const
{
	return p3f();
}

inline p3f m33f::computeEulerAngles() const // this assumes X -> Y -> Z rotation order
{
	double phi1, phi2, theta1, theta2, psi1, psi2;
	if (abs(v[2][0]) != 1)
	{
		theta1 = -asin(double(v[2][0]));
		theta2 = Math::pi - theta1;
		psi1 = atan2(double(v[2][1]) / cos(theta1), double(v[2][2]) / cos(theta1));
		psi2 = atan2(double(v[2][1]) / cos(theta2), double(v[2][2]) / cos(theta2));
		phi1 = atan2(double(v[1][0]) / cos(theta1), double(v[0][0]) / cos(theta1));
		phi2 = atan2(double(v[1][0]) / cos(theta2), double(v[0][0]) / cos(theta2));
	}
	else
	{
		phi1 = phi2 = 0;//actually can be anything
		if (v[2][0] == -1)
		{
			theta1 = theta2 = Math::pi / 2.0;
			psi1 = psi2 = phi1 + atan2(double(v[0][1]), double(v[0][2]));
		}
		else if (v[2][0] == 1)
		{
			theta1 = theta2 = -Math::pi / 2.0;
			psi1 = psi2 = -phi1 + atan2(-double(v[0][1]), -double(v[0][2]));
		}
		else
			assert(false);
	}
	//we reverse theta here because of corresponding angle is reversed in angle(euler) function
	//this way these two are exactly opposite to each other
	return p3f(psi1 * Math::rad2deg, -theta1 * Math::rad2deg, phi1 * Math::rad2deg);
}

inline m33f m33f::transpose() const
{
	return m33f(p3f(v[0].x, v[1].x, v[2].x),
		p3f(v[0].y, v[1].y, v[2].y),
		p3f(v[0].z, v[1].z, v[2].z));
};

inline m33f m33f::scale(float x, float y, float z) { return scale(p3f(x, y, z)); }
inline m33f m33f::scale(double x, double y, double z) { return scale(p3f(x, y, z)); }
inline m33f m33f::scale(const p3f& xyz)
{
	m33f scaleMat;
	scaleMat.v[0][0] = xyz.x;
	scaleMat.v[1][1] = xyz.y;
	scaleMat.v[2][2] = xyz.z;

	return scaleMat;
}

inline m33f m33f::scale(float s)
{
	m33f scaleMat;
	scaleMat.v[0][0] = s;
	scaleMat.v[1][1] = s;
	scaleMat.v[2][2] = s;

	return scaleMat;
}

inline m33f m33f::scale(double s) { return scale(float(s)); }

inline void m33f::identity()
{
	v[0] = p3f(1., 0., 0.);
	v[1] = p3f(0., 1., 0.);
	v[2] = p3f(0., 0., 1.);
};

inline m33f& m33f::operator=(const m33f& rh) { v[0] = rh.v[0]; v[1] = rh.v[1]; v[2] = rh.v[2]; return *this; }

inline m33f& m33f::operator*=(const m33f& m)
{
	return *this = *this * m;
}

inline p3f m33f::diagonal() const { return p3f(v[0][0], v[1][1], v[2][2]); }

inline m33f  m33f::operator-(const m33f& rh) const
{
	return m33f(v[0] - rh.v[0], v[1] - rh.v[1], v[2] - rh.v[2]);
}

inline m33f m33f::operator*(const m33f& m) const
{
	return m33f(p3f(v[0].x * m.v[0].x + v[0].y * m.v[1].x + v[0].z * m.v[2].x,
		v[0].x * m.v[0].y + v[0].y * m.v[1].y + v[0].z * m.v[2].y,
		v[0].x * m.v[0].z + v[0].y * m.v[1].z + v[0].z * m.v[2].z),

		p3f(v[1].x * m.v[0].x + v[1].y * m.v[1].x + v[1].z * m.v[2].x,
			v[1].x * m.v[0].y + v[1].y * m.v[1].y + v[1].z * m.v[2].y,
			v[1].x * m.v[0].z + v[1].y * m.v[1].z + v[1].z * m.v[2].z),

		p3f(v[2].x * m.v[0].x + v[2].y * m.v[1].x + v[2].z * m.v[2].x,
			v[2].x * m.v[0].y + v[2].y * m.v[1].y + v[2].z * m.v[2].y,
			v[2].x * m.v[0].z + v[2].y * m.v[1].z + v[2].z * m.v[2].z));
};

inline m33f m33f::inverse() const
{
	m33f a(*this); // As a evolves from original mat into identity
	m33f b;   // b evolves from identity into inverse(a)
	int  i, j, i1;

	// Loop over cols of a from left to right, eliminating above and below diag
	for (j = 0; j < 3; j++)	  // Find largest pivot in column j among rows j..2
	{
		i1 = j;             // Row with largest pivot candidate
		for (i = j + 1; i < 3; i++)
		{
			if (fabs(a.v[i][j]) > fabs(a.v[i1][j]))
			{
				i1 = i;

				// Swap rows i1 and j in a and b to p pivot on diagonal
				p3f tmp = a.v[j];
				a.v[j] = a.v[i1]; a.v[i1] = tmp; // swap(a.v[i1], a.v[j]);
				tmp = b.v[j];
				b.v[j] = b.v[i1]; b.v[i1] = tmp; // swap(b.v[i1], b.v[j]);
			}
		}

		if (a.v[j][j] == 0)
		{
			HLOG(_T("uninvertable matrix"));
			return *this;
		}

		b.v[j] = b.v[j] / a.v[j][j];
		a.v[j] = a.v[j] / a.v[j][j];

		// Eliminate off-diagonal elems in col j of a, doing identical ops to b
		for (i = 0; i < 3; i++)
		{
			if (i != j)
			{
				b.v[i] = b.v[i] - b.v[j] * a.v[i][j];
				a.v[i] = a.v[i] - a.v[j] * a.v[i][j];
			}
		}
	}
	return b;
}

inline p3f& m33f::operator [] (int idx) { assert(idx >= 0 && idx < 3); return v[idx]; }
inline const p3f& m33f::operator [] (int idx)const { assert(idx >= 0 && idx < 3); return v[idx]; }

inline m33f::operator float* () const
{
	float* cgl = ((m33f*)this)->gl; // breaking the const rule

	cgl[0] = v[0].x; cgl[1] = v[0].y; cgl[2] = v[0].z; cgl[3] = 0.0f;
	cgl[4] = v[1].x; cgl[5] = v[1].y; cgl[6] = v[1].z; cgl[7] = 0.0f;
	cgl[8] = v[2].x; cgl[9] = v[2].y; cgl[10] = v[2].z; cgl[11] = 0.0f;
	cgl[12] = 0.0f;	  cgl[13] = 0.0f;	cgl[14] = 0.0f;   cgl[15] = 1.0f;

	return cgl;
}

inline float* m33f::transposef() const
{
	float* cgl = ((m33f*)this)->gl; // breaking the const rule

	cgl[0] = v[0].x; cgl[4] = v[0].y; cgl[8] = v[0].z; cgl[12] = 0.0f;
	cgl[1] = v[1].x; cgl[5] = v[1].y; cgl[9] = v[1].z; cgl[13] = 0.0f;
	cgl[2] = v[2].x; cgl[6] = v[2].y; cgl[10] = v[2].z; cgl[14] = 0.0f;
	cgl[3] = 0.0f;	  cgl[7] = 0.0f;	cgl[11] = 0.0f;   cgl[15] = 1.0f;

	return cgl;
}

inline void m33f::set(float* gl44Mat)
{
	float* cgl = gl44Mat;
	v[0].x = cgl[0];  v[0].y = cgl[3];  v[0].z = cgl[6];
	v[1].x = cgl[1];  v[1].y = cgl[4];  v[1].z = cgl[7];
	v[2].x = cgl[2];  v[2].y = cgl[5];  v[2].z = cgl[8];
}
