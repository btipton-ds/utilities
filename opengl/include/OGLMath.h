#pragma once

#include <math.h>
#include <assert.h>

#ifndef HLOG
#define HLOG(dummy)
#endif

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

#include "OGLp2f.h"
#include "OGLp3f.h"
#include "OGLp4f.h"
#include "OGLm33f.h"
#include "OGLm44f.h"

/** .
plane line routines from: 
http://astronomy.swin.edu.au/pbourke/geometry/
 */

class linef
{
public:
	linef( const p3f& vec )			{ p2 = vec; }
	linef( const p3f& from, p3f& to ) { p1 = from; p2 = to; }

	float sqlength()const	{ return (p2-p1).sqlength(); }
	float length()	const	{ return (p2-p1).length();   }

	p3f vec() const			  { return p1-p2; }
	p3f normalizedVec() const { return vec().normalize(); }

	bool parallel( linef& line )	{ assert(0); return false; }
	bool intersects( linef& line )	{ assert(0); return false; }
	bool intersects( p3f& point)	{ assert(0); return false; }

	p3f closestTo( linef& line )	{ assert(0); return p3f(); }
	p3f closestTo( p3f& p3 ) 
	{
		assert(0);//not sure if this is correct yet
		float length = sqlength();
		assert(p1!=p2 && length > 0.0f);

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

class planef
{
public:
	planef() {}
	planef(const p3f& point, const p3f& normal) { p1 = point; n = normal; }

	bool intersects(  const linef& line ) { return fabs(n.dot(line.vec()))> Math::tol; }
	p3f intersection ( const linef& line ) const
	{ 
		float VecNormDot = n.dot( line.vec() );
		assert( fabs(VecNormDot) > Math::tol ); //parallel

		return line.p1 + line.vec() * (n.dot( p1-line.p1 ) / VecNormDot);
	}

	p3f p1;
	p3f n;	
};

/** .
2d bounding box determined by 4 floats
 */
class box4f
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

