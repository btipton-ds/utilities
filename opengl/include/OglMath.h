#pragma once

#include <Kernel/OpenGLib/OGLExport.h>
#include <Kernel/OpenGLib/Logger.h>
#include <Kernel/OpenGLib/Timer.h>
#include <math.h>

/** .
struct containing typical math constants
 */
struct Math
{
	static const float tol;
	static const float pi;
	static const float twopi;
	static const float deg2rad;
	static const float rad2deg;

    static float clamp_rad( float rad  ); // a 2pi fmod, locks on quadrant boundaries if within tolerance
    static int   clamp_deg( float& deg ); // clamps deg between 0-360, locks on quadrant boundaries if within tolerance, returns magnitude, 
};

// equality within a tolerance
#define areEqual(a, b) (fabs(a-b) < Math::tol)

class m33f;
class m44f;
class p3f;

/** .
basic geometric operations on a pair of floats
 */
class OGLAPI p2f
{
public:
	p2f()						{ x=0.0f; y=0.0f;			 }
	p2f(const p2f& rh )			{ x=rh.x; y=rh.y;			 }
	p2f(float x_,  float y_)	{ x=x_; y=y_;				 }
	p2f(double x_, double y_)	{ x=float(x_); y=float(y_);  }
	p2f(int x_,    int y_)		{ x=float(x_); y=float(y_);  }

	p2f& reset()					{ x=0.0f; y=0.0f;  return *this;}
	p2f& set(float x_, float y_)	{ x=x_; y=y_; return *this;}
	p2f& set(double x_, double y_)  { x=float(x_); y=float(y_);  return *this;}
	p2f& lowUniClamp(float c)		{ if(fabs(x)<c||fabs(y)<c) x = y = c; return *this; }
	
	p2f   normalize()  { float len=this->length(); Assert(len>0.0f); return *this = *this / len;}
	float sqlength()   { return x*x+y*y;               }
	float length()     { return float( sqrt(sqlength()) );      }
	float dot( p2f& v) { return x*v.x + y*v.y;         }
	float rad(p2f& v)  { float r=this->normalize().dot(v.normalize()); return float( r>=-1.0f||r<=1.0f?acos(r):0 ); }	
	float deg(p2f& v)  { return Math::rad2deg * rad(v); }

	p2f& operator= ( const p2f& rh )		{ x=rh.x; y=rh.y; return *this;   }
	p2f  operator+ ( const p2f& rh ) const	{ return p2f( x+rh.x, y+rh.y);    }
	p2f& operator+=( const p2f& rh )		{ x+=rh.x; y+=rh.y; return *this; }
	p2f  operator- ( const p2f& rh ) const	{ return p2f( x-rh.x, y-rh.y);    }
	p2f& operator-=( const p2f& rh )		{ x-=rh.x; y-=rh.y; return *this; }
	p2f  operator* ( float f ) const		{ return p2f( x*f, y*f );         }
	p2f& operator*=( float f )				{ x*=f; y*=f; return *this;       }
	p2f  operator/ ( float f ) const		{ return p2f( x/f, y/f );         }
	p2f& operator/=( float f )				{ x/=f; y/=f; return *this;       }

	p2f  operator* ( double d ) const	{ return *this *  float(d); }
	p2f& operator*=( double d )			{ return *this *= float(d); }
	p2f  operator/ ( double d ) const	{ return *this /  float(d); }
	p2f& operator/=( double d )			{ return *this /= float(d); }

	p2f  operator -()			{ return p2f(-x,-y); }

	bool operator==( p2f& rh) const	{ return fabs(x-rh.x) < Math::tol && fabs(y-rh.y) < Math::tol; }
	bool operator!=( p2f& rh) const	{ return !(*this==rh); }

	operator float*() const { return (float*)&x; } 
	const float *asfloat()  const { return &x; }
	float& operator[](int idx) 
	{
		switch(idx)
		{
		case 0: return x; break;
		case 1: return y; break;
		default: Assert(0);
		}
		return x;
	}

	float x,y;
	long  tag;
};

/** .
basic operations for a triple of floats treated as vector or point,
except color use col4f for color
 */
class OGLAPI p3f
{
public:
	p3f() { x=0.0f; y=0.0f; z=0.0f; tag=0; }
	p3f(const p3f& rh) { x=rh.x; y=rh.y; z=rh.z; tag=rh.tag;}
	p3f(const p2f& rh) { x=rh.x; y=rh.y; z=0.0f; tag=0;}
	p3f(float x_, float y_, float z_)		{ x=x_; y=y_; z=z_; tag=0;}
	p3f(double x_, double y_, double z_)	{ x=float(x_); y=float(y_); z=float(z_); tag=0;}
	p3f(const float* p) { x=p[0]; y=p[1]; z=p[2]; tag=0; }

    bool equal( const p3f& rh) const{ return areEqual(x,rh.x) && areEqual(y,rh.y) && areEqual(z,rh.z); }

	static p3f xaxis() {return p3f(1.,0.,0.);}
	static p3f yaxis() {return p3f(0.,1.,0.);}
	static p3f zaxis() {return p3f(0.,0.,1.);}

	p3f& reset() { x=0.0f; y=0.0f; z=0.0f; return *this;}
	p3f& set(float x_, float y_, float z_)		{ x=x_; y=y_; z=z_; return *this;}
	p3f& set(double x_, double y_, double z_)  { x=float(x_); y=float(y_); z=float(z_); return *this;}
	p3f& lowUniClamp(float c) { if(fabs(x)<c||fabs(y)<c||fabs(z)<c) x = y = z = c; return *this; }

    p3f   normalize()              { float len=this->length(); if( len > 0.0f ) return *this = *this / len; else return *this; }
	float sqlength()		  const{ return x*x+y*y+z*z;			}
    float magnitudeSquared()  const{ return x*x+y*y+z*z;			}
	float length()			  const{ return float( sqrt(sqlength()) ); }		
	float dot  (const p3f& v) const{ return x*v.x + y*v.y + z*v.z;	}	
	p3f	  cross(const p3f& v) const{ return p3f(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }
	p3f   rotax(p3f& v)            { return this->normalize().cross(v.normalize()); }
	float deg  (const p3f& v) const{ return Math::rad2deg * atan2( this->cross(v).length(), this->dot(v)); }
	p3f   inverse()           const{ return p3f(-x,-y,-z); }
    p3f   floor()             const{ return p3f(::floor(x),::floor(y),::floor(z)); } 
    p3f   ceil()              const{ return p3f(::ceil(x),::ceil(y),::ceil(z)); } 
    p3f   minimum(const p3f& rh)  const{ return p3f(x<rh.x?x:rh.x, y<rh.y?y:rh.y, z<rh.z?z:rh.z); } 
    p3f   maximum(const p3f& rh)  const{ return p3f(x>rh.x?x:rh.x, y>rh.y?y:rh.y, z>rh.z?z:rh.z); } 

	p3f& operator= ( const p3f& rh )		{ x=rh.x; y=rh.y; z=rh.z; tag=rh.tag; return *this; }
	p3f  operator+ ( const p3f& rh ) const	{ return p3f( x+rh.x, y+rh.y, z+rh.z ); }
	p3f& operator+=( const p3f& rh )		{ x+=rh.x; y+=rh.y; z+=rh.z; return *this; }
	p3f  operator- ( const p3f& rh ) const	{ return p3f( x-rh.x, y-rh.y, z-rh.z ); }
	p3f& operator-=( const p3f& rh )		{ x-=rh.x; y-=rh.y; z-=rh.z; return *this; }
	p3f  operator* ( float f ) const		{ return p3f( x*f, y*f, z*f ); }
	p3f& operator*=( float f )				{ x*=f; y*=f; z*=f; return *this; }
	p3f  operator/ ( float f ) const		{ return p3f( x/f, y/f, z/f ); }
	p3f& operator/=( float f )				{ x/=f; y/=f; z/=f; return *this; }

	p3f  operator* ( double d ) const	{ return *this *  float(d); }
	p3f& operator*=( double d )			{ return *this *= float(d); }
	p3f  operator/ ( double d ) const	{ return *this /  float(d); }
	p3f& operator/=( double d )			{ return *this /= float(d); }

	p3f  operator -() const				{ return p3f(-x,-y,-z); }

	bool operator==( const p3f& rh)	const { return (fabs(x-rh.x) < Math::tol) && (fabs(y-rh.y) < Math::tol) && (fabs(z-rh.z) < Math::tol); }	
	bool operator!=( const p3f& rh)	const { return !(*this==rh); }

	p3f  operator* (const m33f& v) const;
	p3f& operator*=(const m33f& m) { return (*this = *this * m); }
	p3f  operator* (const m44f& m) const;
	p3f& operator*=(const m44f& m) { return (*this = *this * m); }

	operator float*()	const { return (float*)&x; } 
	float*  asfloat()	const { return (float*)&x; } 

	float& operator[](int idx) 
	{
		switch(idx)
		{
		case 0: return x; break;
		case 1: return y; break;
		case 2: return z; break;
		default: Assert(0);
		}
		return x;
	}

	const float& operator[](int idx) const
	{
		switch(idx)
		{
		case 0: return x; break;
		case 1: return y; break;
		case 2: return z; break;
		default: Assert(0);
		}
		return x;
	}

	void set(float* gl44Mat)
	{
		float* cgl = gl44Mat;
		x = cgl[12];  y = cgl[13];  z = cgl[14]; 
	}

	float x,y,z;
	long  tag;
};

inline p3f operator* (const double s, const p3f &v) { return v*s; }

/** .
typical operations on 4 float objects, 
except color use col4f for color
 */
class OGLAPI p4f
{
public:
	p4f()				{ x=0.0f; y=0.0f; z=0.0f; w=1.0f; }
	p4f(const p3f& rh)	{ x=rh.x; y=rh.y; z=rh.z; w=1.0f; }
	p4f(float x_, float y_, float z_, float w_=1.0f)    { x=x_; y=y_; z=z_; w=w_;}
	p4f(double x_, double y_, double z_, double w_=1.0) { x=(float)x_; y=(float)y_; z=(float)z_; w=(float)w_; }
	p4f& reset() { x=0.0f; y=0.0f; z=0.0f; w=1.0f; return *this;}

	void set(float x_, float y_, float z_, float w_=1.0f) { x=x_; y=y_; z=z_; w=w_;}
	void set(double x_, double y_, double z_, double w_=1.0) { x=(float)x_; y=(float)y_; z=(float)z_; w=(float)w_; }

	p4f& operator =( const p4f& rh)			{ x=rh.x; y=rh.y; z=rh.z; w=rh.w;  return *this;}
	p4f& operator =( const p3f& rh)			{ x=rh.x; y=rh.y; z=rh.z; w=1.0f;  return *this;}
	p4f  operator+ ( const p4f& rh ) const	{ return p4f( x+rh.x, y+rh.y, z+rh.z, w+rh.w ); }
	p4f& operator+=( const p4f& rh )		{ x+=rh.x; y+=rh.y; z+=rh.z; w+=rh.w; return *this; }
	p4f  operator- ( const p4f& rh ) const	{ return p4f( x-rh.x, y-rh.y, z-rh.z, w-rh.w ); }
	p4f& operator-=( const p4f& rh )		{ x-=rh.x; y-=rh.y; z-=rh.z; w-=rh.w; return *this; }
	p4f  operator* ( float f ) const		{ return p4f( x*f, y*f, z*f, w*f ); }
	p4f& operator*=( float f )				{ x*=f; y*=f; z*=f; w*=f; return *this; }
	p4f  operator/ ( float f ) const		{ return p4f( x/f, y/f, z/f, w/f ); }
	p4f& operator/=( float f )				{ x/=f; y/=f; z/=f; w/=f; return *this; }

	operator float*()	const { return (float*)&x; } 
	float*  asfloat()	const { return (float*)&x; } 

	float x,y,z,w;
};

/** .
typical operations on a 3x3 matrix of floats
 */
class OGLAPI m33f
{
public:
	m33f() { identity(); }
	m33f(p3f& a, p3f& b, p3f& c) { v[0]=a; v[1]=b; v[2]=c; }
	m33f( const m33f& rh) { v[0]=rh.v[0]; v[1]=rh.v[1]; v[2]=rh.v[2]; }
	m33f( const m44f& rh); //extracts rotational component of a 4x4
	m33f( float  Xrot, float  Yrot, float  Zrot) { *this = angle( p3f(Xrot,Yrot,Zrot) ); }
	m33f( double Xrot, double Yrot, double Zrot) { *this = angle( p3f(Xrot,Yrot,Zrot) ); }
	m33f(p3f& euler) { *this = angle(euler); }
	
    bool equal(const m33f& rh) const { return v[0].equal(rh.v[0]) && v[1].equal(rh.v[1]) && v[2].equal(rh.v[2]);}

	float length() const{ return sqrtf(v[0].sqlength() + v[1].sqlength() + v[2].sqlength());}

	static m33f angle(p3f& xyz)
	{		
		float xr   = xyz.x * Math::deg2rad;
		float yr   = xyz.y * Math::deg2rad;
		float zr   = xyz.z * Math::deg2rad;

		float A    = (float) cos(xr);
		float B    = (float) sin(xr);
		float C    = (float) cos(yr);
		float D    = (float) sin(yr);
		float E    = (float) cos(zr);
		float F    = (float) sin(zr);
		
		float AD   =   A * D;
		float BD   =   B * D;
		
		m33f rot;
		rot[0][0]  =   C * E;
		rot[0][1]  =  -C * F;
		rot[0][2]  =  -D;
		rot[1][0]  = -BD * E + A * F;
		rot[1][1]  =  BD * F + A * E;
		rot[1][2]  =  -B * C;
		rot[2][0]  =  AD * E + B * F;
		rot[2][1]  = -AD * F + B * E;
		rot[2][2]  =   A * C;

		return rot;
	}
	static m33f angle(float  x, float  y, float  z) { return angle(p3f(x,y,z)); }
	static m33f angle(double x, double y, double z) { return angle(p3f(x,y,z)); }
	
	static m33f axangle(p3f& axis, float angle)
	{
		p3f ax = axis.normalize();
        float x = ax.x, y = ax.y, z = ax.z;

		float rad = angle * Math::deg2rad;
		float c   = (float) cos(rad);
		float s   = (float) sin(rad);
		float C   = 1.0f - c;	

        float xs  = x*s,   ys = y*s,   zs = z*s;
        float xC  = x*C,   yC = y*C,   zC = z*C;
        float xyC = x*yC, yzC = y*zC, zxC = z*xC;

		return m33f(    p3f( x*xC+c, xyC-zs, zxC+ys),
		                p3f( xyC+zs, y*yC+c, yzC-xs),
		                p3f( zxC-ys, yzC+xs, z*zC+c) );
	}

    p3f euler( const p3f& start_euler = p3f(0.0f, 0.0f, 0.0f)) const;
    p3f computeEulerAngles() const // this assumes X -> Y -> Z rotation order
    {
       double phi1, phi2, theta1, theta2, psi1, psi2;
       if(abs(v[2][0]) != 1)
       {
           theta1 = -asin(double(v[2][0]));
           theta2 = Math::pi - theta1;
           psi1 = atan2(double(v[2][1])/cos(theta1), double(v[2][2])/cos(theta1));
           psi2 = atan2(double(v[2][1])/cos(theta2), double(v[2][2])/cos(theta2));
           phi1 = atan2(double(v[1][0])/cos(theta1), double(v[0][0])/cos(theta1));
           phi2 = atan2(double(v[1][0])/cos(theta2), double(v[0][0])/cos(theta2));
       }
       else
       {
            phi1 = phi2 = 0;//actually can be anything
            if(v[2][0] == -1)
            {
                theta1 = theta2 = Math::pi / 2.0;
                psi1 = psi2 = phi1 + atan2(double(v[0][1]), double(v[0][2]));
            }
            else if (v[2][0] == 1)
            {
                theta1 = theta2 = - Math::pi / 2.0;
                psi1 = psi2 = -phi1 + atan2(-double(v[0][1]), -double(v[0][2]));
            }
            else
                assert(false);
       }
       //we reverse theta here because of corresponding angle is reversed in angle(euler) function
       //this way these two are exactly opposite to each other
       return p3f(psi1 * Math::rad2deg, - theta1 * Math::rad2deg,phi1 * Math::rad2deg);
    }

	m33f transpose() const
	{
		return m33f(p3f(v[0].x, v[1].x, v[2].x),
					p3f(v[0].y, v[1].y, v[2].y),
					p3f(v[0].z, v[1].z, v[2].z));
	};

	static m33f scale( float x,  float y,  float z)  { return scale(p3f(x,y,z));}
	static m33f scale( double x, double y, double z) { return scale(p3f(x,y,z));}
	static m33f scale( p3f& xyz )
	{
		m33f scaleMat;
		scaleMat.v[0][0] = xyz.x;
		scaleMat.v[1][1] = xyz.y;
		scaleMat.v[2][2] = xyz.z;

		return scaleMat;
	}

	static m33f scale( float s )
	{
		m33f scaleMat;
		scaleMat.v[0][0] = s;
		scaleMat.v[1][1] = s;
		scaleMat.v[2][2] = s;

		return scaleMat;
	}

	static m33f scale( double s ) { return scale( float(s) ); }

	void identity()
	{
		v[0] = p3f(1.,0.,0.);
		v[1] = p3f(0.,1.,0.);
		v[2] = p3f(0.,0.,1.);
	};

	m33f& operator=(const m33f& rh) { v[0]=rh.v[0]; v[1]=rh.v[1]; v[2]=rh.v[2]; return *this;}

	m33f& operator*=(const m33f& m)
	{
		return *this = *this * m;
	}

	p3f diagonal() const { return p3f( v[0][0], v[1][1], v[2][2] ); }

	m33f  operator-( const m33f& rh ) const
	{ 
		return m33f(v[0] - rh.v[0], v[1] - rh.v[1], v[2] - rh.v[2]); 
	}

	m33f operator*(const m33f& m) const
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

	m33f inverse() const
	{
		m33f a(*this); // As a evolves from original mat into identity
		m33f b;   // b evolves from identity into inverse(a)
		int  i, j, i1;
        
		// Loop over cols of a from left to right, eliminating above and below diag
		for (j=0; j<3; j++)	  // Find largest pivot in column j among rows j..2
		{   
			i1 = j;             // Row with largest pivot candidate
			for (i=j+1; i<3; i++)
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

			if(a.v[j][j] == 0)
			{
				HLOG(_T("uninvertable matrix"));
				return *this;
			}

			b.v[j] = b.v[j]/a.v[j][j];
			a.v[j] = a.v[j]/a.v[j][j];
			
			// Eliminate off-diagonal elems in col j of a, doing identical ops to b
			for (i=0; i<3; i++)
			{
				if (i!=j) 
				{
					b.v[i] = b.v[i] - b.v[j]*a.v[i][j];
					a.v[i] = a.v[i] - a.v[j]*a.v[i][j];
				}
			}
		}
		return b;
	}

	p3f& operator [] (int idx)			  { Assert(idx >=0 && idx <3); return v[idx]; }
	const p3f& operator [] (int idx)const { Assert(idx >=0 && idx <3); return v[idx]; }

	operator float*() const
	{
		float* cgl = ((m33f*)this)->gl; // breaking the const rule
	
   		cgl[ 0] = v[0].x; cgl[ 1] = v[0].y; cgl[ 2] = v[0].z; cgl[ 3] = 0.0f;
  		cgl[ 4] = v[1].x; cgl[ 5] = v[1].y; cgl[ 6] = v[1].z; cgl[ 7] = 0.0f;
  		cgl[ 8] = v[2].x; cgl[ 9] = v[2].y; cgl[10] = v[2].z; cgl[11] = 0.0f;
  		cgl[12] = 0.0f;	  cgl[13] = 0.0f;	cgl[14] = 0.0f;   cgl[15] = 1.0f;

		return cgl;
	}

	float* transposef() const
	{
		float* cgl = ((m33f*)this)->gl; // breaking the const rule

		cgl[ 0] = v[0].x; cgl[ 4] = v[0].y; cgl[ 8] = v[0].z; cgl[12] = 0.0f;
		cgl[ 1] = v[1].x; cgl[ 5] = v[1].y; cgl[ 9] = v[1].z; cgl[13] = 0.0f;
		cgl[ 2] = v[2].x; cgl[ 6] = v[2].y; cgl[10] = v[2].z; cgl[14] = 0.0f;
		cgl[ 3] = 0.0f;	  cgl[ 7] = 0.0f;	cgl[11] = 0.0f;   cgl[15] = 1.0f;

		return cgl;
	}

	void set(float* gl44Mat)
	{
		float* cgl = gl44Mat;
		 v[0].x = cgl[ 0];  v[0].y = cgl[ 3];  v[0].z = cgl[ 6]; 
		 v[1].x = cgl[ 1];  v[1].y = cgl[ 4];  v[1].z = cgl[ 7]; 
		 v[2].x = cgl[ 2];  v[2].y = cgl[ 5];  v[2].z = cgl[ 8]; 
	}

	p3f v[3];
private:
	float gl[16];
};

/** .
typical operations on a 4x4 matrix of floats
 */
class OGLAPI m44f
{
public:
	m44f() { identity(); }
	m44f( const p3f& a, const p3f& b, const p3f& c) { v[0]=a; v[0].w=0.0f; v[1]=b; v[1].w=0.0f; v[2]=c; v[2].w=0.0f; v[3].reset();  }
	m44f( const p4f& a, const p4f& b, const p4f& c, const p4f& d) { v[0]=a; v[1]=b; v[2]=c;  v[3] = d;  }
	m44f( const m33f& rot) { v[0]=rot.v[0]; v[0].w=0.0f; v[1]=rot.v[1]; v[1].w=0.0f; v[2]=rot.v[2]; v[2].w=0.0f; v[3].reset();}
//	m44f( const m33f& rot, const p3f& tra) { v[0]=rot.v[0]; v[0].w=tra.x; v[1]=rot.v[1]; v[1].w=tra.y; v[2]=rot.v[2]; v[2].w=tra.z; v[3].reset();}
	m44f( const m33f& rot, const p3f& tra) { v[0]=rot.v[0]; v[0].w=0.0f; v[1]=rot.v[1]; v[1].w=0.0f; v[2]=rot.v[2]; v[2].w=0.0f; v[3]=tra; v[3].w=1.0f;}

	p4f operator*(const p4f& inVec)
	{
		p4f retVec(0.0f, 0.0f, 0.0f, 0.0f);
		retVec.x = v[0].x * inVec.x + v[0].y * inVec.y + v[0].z * inVec.z+ v[0].w * inVec.w;
		retVec.y = v[1].x * inVec.x + v[1].y * inVec.y + v[1].z * inVec.z+ v[1].w * inVec.w;
		retVec.z = v[2].x * inVec.x + v[2].y * inVec.y + v[2].z * inVec.z+ v[2].w * inVec.w;
		retVec.w = v[3].x * inVec.x + v[3].y * inVec.y + v[3].z * inVec.z+ v[3].w * inVec.w;
		return retVec;
	}

	m44f  transpose() const
	{
		m44f x;
		x[0].x = v[0].x;  x[0].y = v[1].x;  x[0].z = v[2].x;  x[0].w = v[3].x;
		x[1].x = v[0].y;  x[1].y = v[1].y;  x[1].z = v[2].y;  x[1].w = v[3].y;
		x[2].x = v[0].z;  x[2].y = v[1].z;  x[2].z = v[2].z;  x[2].w = v[3].z;
		x[3].x = v[0].w;  x[3].y = v[1].w;  x[3].z = v[2].w;  x[3].w = v[3].w;	
		return x;
	}

    bool isIdentity()
    {
         return
 		 areEqual(v[0].x,1.0f) && areEqual(v[0].y, 0.0f)  && areEqual(v[0].z, 0.0f)  && areEqual(v[0].w, 0.0f) &&
		 areEqual(v[1].x,0.0f) && areEqual(v[1].y, 1.0f)  && areEqual(v[1].z, 0.0f)  && areEqual(v[1].w, 0.0f) && 
		 areEqual(v[2].x,0.0f) && areEqual(v[2].y, 0.0f)  && areEqual(v[2].z, 1.0f)  && areEqual(v[2].w, 0.0f) &&
		 areEqual(v[3].x,0.0f) && areEqual(v[3].y, 0.0f)  && areEqual(v[3].z, 0.0f)  && areEqual(v[3].w, 1.0f);	       
    }

	m44f& identity() 
	{
		 v[0].x = 1.0f;  v[0].y = 0.0f;  v[0].z = 0.0f;  v[0].w = 0.0f;
		 v[1].x = 0.0f;  v[1].y = 1.0f;  v[1].z = 0.0f;  v[1].w = 0.0f;
		 v[2].x = 0.0f;  v[2].y = 0.0f;  v[2].z = 1.0f;  v[2].w = 0.0f;
		 v[3].x = 0.0f;  v[3].y = 0.0f;  v[3].z = 0.0f;  v[3].w = 1.0f;	
		 return * this;
	}
	operator float*() const
	{
		float* cgl = ((m44f*)this)->gl; // breaking the const rule
		cgl[ 0] = v[0].x; cgl[ 1] = v[0].y; cgl[ 2] = v[0].z; cgl[ 3] = v[0].w;
		cgl[ 4] = v[1].x; cgl[ 5] = v[1].y; cgl[ 6] = v[1].z; cgl[ 7] = v[1].w;
		cgl[ 8] = v[2].x; cgl[ 9] = v[2].y; cgl[10] = v[2].z; cgl[11] = v[2].w;
		cgl[12] = v[3].x; cgl[13] = v[3].y; cgl[14] = v[3].z; cgl[15] = v[3].w;

		return cgl;
	}

	float* transposef() const
	{
		float* cgl = ((m44f*)this)->gl; // breaking the const rule
		cgl[ 0] = v[0].x; cgl[ 4] = v[0].y; cgl[ 8] = v[0].z; cgl[12] = v[0].w;
		cgl[ 1] = v[1].x; cgl[ 5] = v[1].y; cgl[ 9] = v[1].z; cgl[13] = v[1].w;
		cgl[ 2] = v[2].x; cgl[ 6] = v[2].y; cgl[10] = v[2].z; cgl[14] = v[2].w;
		cgl[ 3] = v[3].x; cgl[ 7] = v[3].y; cgl[11] = v[3].z; cgl[15] = v[3].w;

		return cgl;
	}

	void set(float* gl44Mat)
	{
		float* cgl = gl44Mat;
		 v[0].x = cgl[ 0];  v[0].y = cgl[ 4];  v[0].z = cgl[ 8];  v[0].w = cgl[12];
		 v[1].x = cgl[ 1];  v[1].y = cgl[ 5];  v[1].z = cgl[ 9];  v[1].w = cgl[13];
		 v[2].x = cgl[ 2];  v[2].y = cgl[ 6];  v[2].z = cgl[10];  v[2].w = cgl[14];
		 v[3].x = cgl[ 3];  v[3].y = cgl[ 7];  v[3].z = cgl[11];  v[3].w = cgl[15];
	}

	p4f& operator [] (int idx)			  { Assert(idx >=0 && idx <4); return v[idx]; }
	const p4f& operator [] (int idx)const { Assert(idx >=0 && idx <4); return v[idx]; }

	static m44f scale( float x,  float y,  float z)  { return scale(p3f(x,y,z));}
	static m44f scale( double x, double y, double z) { return scale(p3f(x,y,z));}
	static m44f scale( float s  ) { return scale(p3f(s,s,s));}
    static m44f scale( double s ) { return scale(p3f(s,s,s));}
	static m44f scale( p3f& xyz )
	{
		m33f scaleMat;
		scaleMat.v[0][0] = xyz.x;
		scaleMat.v[1][1] = xyz.y;
		scaleMat.v[2][2] = xyz.z;
		return scaleMat;
	}
    m44f operator*=(const m44f& m) { return *this = *this * m; }
	m44f operator* (const m44f& m) const
	{
	    return m44f(p4f(v[0].x * m.v[0].x + v[0].y * m.v[1].x + v[0].z * m.v[2].x + v[0].w * m.v[3].x,
				        v[0].x * m.v[0].y + v[0].y * m.v[1].y + v[0].z * m.v[2].y + v[0].w * m.v[3].y,
				        v[0].x * m.v[0].z + v[0].y * m.v[1].z + v[0].z * m.v[2].z + v[0].w * m.v[3].z,
                        v[0].x * m.v[0].w + v[0].y * m.v[1].w + v[0].z * m.v[2].w + v[0].w * m.v[3].w), 

				    p4f(v[1].x * m.v[0].x + v[1].y * m.v[1].x + v[1].z * m.v[2].x + v[1].w * m.v[3].x,
					    v[1].x * m.v[0].y + v[1].y * m.v[1].y + v[1].z * m.v[2].y + v[1].w * m.v[3].y,
				        v[1].x * m.v[0].z + v[1].y * m.v[1].z + v[1].z * m.v[2].z + v[1].w * m.v[3].z,
                        v[1].x * m.v[0].w + v[1].y * m.v[1].w + v[1].z * m.v[2].w + v[1].w * m.v[3].w),

				    p4f(v[2].x * m.v[0].x + v[2].y * m.v[1].x + v[2].z * m.v[2].x + v[2].w * m.v[3].x,
					    v[2].x * m.v[0].y + v[2].y * m.v[1].y + v[2].z * m.v[2].y + v[2].w * m.v[3].y, 
				        v[2].x * m.v[0].z + v[2].y * m.v[1].z + v[2].z * m.v[2].z + v[2].w * m.v[3].z,
                        v[2].x * m.v[0].w + v[2].y * m.v[1].w + v[2].z * m.v[2].w + v[2].w * m.v[3].w),
                        
                    p4f(v[3].x * m.v[0].x + v[3].y * m.v[1].x + v[3].z * m.v[2].x + v[3].w * m.v[3].x,
					    v[3].x * m.v[0].y + v[3].y * m.v[1].y + v[3].z * m.v[2].y + v[3].w * m.v[3].y, 
				        v[3].x * m.v[0].z + v[3].y * m.v[1].z + v[3].z * m.v[2].z + v[3].w * m.v[3].z,
                        v[3].x * m.v[0].w + v[3].y * m.v[1].w + v[3].z * m.v[2].w + v[3].w * m.v[3].w)); 
	};

#if 0
	m44f operator*(const m33f& m) const
	{
		const p4f&  v0=v[0];   const p4f&  v1=v[1];   const p4f&  v2=v[2];   const p4f&  v3=v[3];
		const p4f& mv0=m.v[0]; const p4f& mv1=m.v[1]; const p4f& mv2=m.v[2]; const p4f& mv3=m.v[3];

	    return m44f(	p4f(v0.x * mv0.x + v0.y * mv1.x +
							v0.z * mv2.x + v0.w * mv3.x,
							v0.x * mv0.y + v0.y * mv1.y +
							v0.z * mv2.y + v0.w * mv3.y,
							v0.x * mv0.z + v0.y * mv1.z +
							v0.z * mv2.z + v0.w * mv3.z,
							v0.x * mv0.w + v0.y * mv1.w +
							v0.z * mv2.w + v0.w * mv3.w),
							
						p4f(v1.x * mv0.x + v1.y * mv1.x +
							v1.z * mv2.x + v1.w * mv3.x,
							v1.x * mv0.y + v1.y * mv1.y +
							v1.z * mv2.y + v1.w * mv3.y,
							v1.x * mv0.z + v1.y * mv1.z +
							v1.z * mv2.z + v1.w * mv3.z,
							v1.x * mv0.w + v1.y * mv1.w +
							v1.z * mv2.w + v1.w * mv3.w), 

						p4f(v2.x * mv0.x + v2.y * mv1.x +
							v2.z * mv2.x + v2.w * mv3.x,
							v2.x * mv0.y + v2.y * mv1.y +
							v2.z * mv2.y + v2.w * mv3.y,
							v2.x * mv0.z + v2.y * mv1.z +
							v2.z * mv2.z + v2.w * mv3.z,
							v2.x * mv0.w + v2.y * mv1.w +
							v2.z * mv2.w + v2.w * mv3.w), 

						p4f(v3.x * mv0.x + v3.y * mv1.x +
							v3.z * mv2.x + v3.w * mv3.x,
							v3.x * mv0.y + v3.y * mv1.y +
							v3.z * mv2.y + v3.w * mv3.y,
							v3.x * mv0.z + v3.y * mv1.z +
							v3.z * mv2.z + v3.w * mv3.z,
							v3.x * mv0.w + v3.y * mv1.w +
							v3.z * mv2.w + v3.w * mv3.w)); 
	};
#endif

	m44f inverse() const
	{
		m44f a(*this); // As a evolves from original mat into identity
		m44f b;   // b evolves from identity into inverse(a)
		int  i, j, i1;
        
		// Loop over cols of a from left to right, eliminating above and below diag
		for (j=0; j<4; j++)	  // Find largest pivot in column j among rows j..2
		{   
			i1 = j;             // Row with largest pivot candidate
			for (i=j+1; i<4; i++)
			{
				if (fabs(a.v[i][j]) > fabs(a.v[i1][j]))
				{
					i1 = i;
            
					// Swap rows i1 and j in a and b to p pivot on diagonal
					p4f tmp = a.v[j];
					a.v[j] = a.v[i1]; a.v[i1] = tmp; // swap(a.v[i1], a.v[j]);
					tmp = b.v[j];
					b.v[j] = b.v[i1]; b.v[i1] = tmp; // swap(b.v[i1], b.v[j]);
				}
			}
			Assert(a.v[j][j] != 0);

			b.v[j] = b.v[j]/a.v[j][j];
			a.v[j] = a.v[j]/a.v[j][j];
			
			// Eliminate off-diagonal elems in col j of a, doing identical ops to b
			for (i=0; i<4; i++)
			{
				if (i!=j) 
				{
					b.v[i] = b.v[i] - b.v[j]*a.v[i][j];
					a.v[i] = a.v[i] - a.v[j]*a.v[i][j];
				}
			}
		}
		return b;
	}

    m33f rotationalDifference( const m44f& rh ) const
    {
        // take two vectors and transform them, then extract the rotational component from the vectors
        p3f o( 0.0, 0.0, 0.0 );
        p3f z( 0.0, 0.0, 1.0 );
        p3f op = o * rh;
        p3f zp = z * rh;
        zp -= op; // remove translational component

        p3f axis    = z.rotax( zp );
        float angle = z.deg( zp );       

        // this is OK for x & y but it's missing rotation around the z axis
        m33f rotDiffXY = m33f::axangle( axis, angle);
    
        // now do the same with y axis
        p3f y( 0.0, 1.0, 0.0 );
        p3f yp = y * rh;
        yp -= op;            // remove translational component
        yp = yp * m33f::axangle( axis, -angle); // remove xy rotational component
        
        // find remaining component of rotation
        axis  = y.rotax( yp );
        angle = y.deg( yp );  
        m33f rotDiffZ = m33f::axangle( axis, angle);

        return rotDiffXY * rotDiffZ;
    }

	p4f v[4];
private:
	float gl[16];
};



/** .
plane line routines from: 
http://astronomy.swin.edu.au/pbourke/geometry/
 */

class OGLAPI linef
{
public:
	linef( const p3f& vec )			{ p2 = vec; }
	linef( const p3f& from, p3f& to ) { p1 = from; p2 = to; }

	float sqlength()const	{ return (p2-p1).sqlength(); }
	float length()	const	{ return (p2-p1).length();   }

	p3f vec() const			  { return p1-p2; }
	p3f normalizedVec() const { return vec().normalize(); }

	bool parallel( linef& line )	{ Assert(0); return false; }
	bool intersects( linef& line )	{ Assert(0); return false; }
	bool intersects( p3f& point)	{ Assert(0); return false; }

	p3f closestTo( linef& line )	{ Assert(0); return p3f(); }
	p3f closestTo( p3f& p3 ) 
	{
		Assert(0);//not sure if this is correct yet
		float length = sqlength();
		Assert(p1!=p2 && length > 0.0f);

		float u = ((p3.x-p1.x)*(p2.x-p1.x)+(p3.y-p1.y)*(p2.y-p1.y)) / length;
		return p1 + (p2-p1)*u;
	}

	p3f p1;
	p3f p2;
};

/** .
plane line routines from: 
http://astronomy.swin.edu.au/pbourke/geometry/
 */

class OGLAPI planef
{
public:
	planef() {}
	planef(const p3f& point, const p3f& normal) { p1 = point; n = normal; }

	bool intersects(  const linef& line ) { return fabs(n.dot(line.vec()))> Math::tol; }
	p3f intersection ( const linef& line ) const
	{ 
		float VecNormDot = n.dot( line.vec() );
		Assert( fabs(VecNormDot) > Math::tol ); //parallel

		return line.p1 + line.vec() * (n.dot( p1-line.p1 ) / VecNormDot);
	}

	p3f p1;
	p3f n;	
};

/** .
2d bounding box determined by 4 floats
 */
class OGLAPI box4f
{
public:
	box4f() { x1=y1=x2=y2=0.0f; }
	box4f(float x, float y, float wh)         { wh/=2.0f; x1=x-wh; y1=y-wh; x2=x+wh; y2=y+wh; }
	box4f(float x, float y, float w, float h) { w/=2.0f; h/=2.0f; x1=x-w; y1=y-h; x2=x+w; y2=y+h;}

	box4f(double x, double y, double wh)         { wh/=2.0; x1=float(x-wh); y1=float(y-wh); x2=float(x+wh); y2=float(y+wh); }
	box4f(double x, double y, double w, double h){  w/=2.0; h/=2.0; x1=float(x-w); y1=float(y-h); x2=float(x+w); y2=float(y+h);}

	box4f move(float x, float y)	{ box4f ret(*this); ret.x1+=x;ret.x2+=x;ret.y1+=y;ret.y2+=y; return ret;}
	box4f move(double x, double y)  { return move(float(x), float(y)); }

	operator float*() { return &x1; }

	float x1, y1, x2, y2;
};
