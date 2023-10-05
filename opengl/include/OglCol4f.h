#pragma once
#include <Kernel/OpenGLib/OglMath.h> 
#include <Kernel/OpenGLib/OGLExport.h>


//from http://www.seas.rochester.edu/CNG/docs/x11color.html
#define oglRed			1.00f,  0.00f,  0.00f
#define oglGreen		0.00f,  1.00f,  0.00f
#define oglBlue			0.00f,  0.00f,  1.00f
#define oglYellow		1.00f,  1.00f,  0.00f
#define oglIndigo		0.29f,  0.00f,  0.50f 
#define oglOrchid		0.85f,  0.44f,  0.84f 
#define oglSlateBlue	0.42f,  0.35f,  0.80f 
#define oglFireBrick	0.70f,  0.13f,  0.13f 
#define oglLimeGreen	0.20f,  0.80f,  0.20f
#define oglOrange		1.00f,  0.65f,  0.00f
#define oglOrangeRed	1.00f,  0.27f,  0.00f


/** .
class with typical operations for color
 */
class OGLAPI col4f
{
public:
	col4f() { r=1.0f; g=1.0f; b=1.0f; o=1.0f; }
	col4f(float* col, float opacity=1.0f) { r=col[0]; g=col[1]; b=col[2]; o=opacity; }
	col4f(const col4f& col, float opacity) { r=col.r; g=col.g; b=col.b; o=opacity; }
	col4f(const col4f& col, double opacity) { r=col.r; g=col.g; b=col.b; o=float(opacity); }
	col4f(float r_,  float g_,  float b_,  float o_=1.0f)    { r=r_; g=g_; b=b_; o=o_;}
	col4f(double r_, double g_, double b_, double o_=1.0) { r=(float)r_; g=(float)g_; b=(float)b_; o=(float)o_; }
    col4f( long col, bool defAlpha = true) 
	{
        r  = ((col & 0x000000ff ) >> 0)  / 255.0f;
        g  = ((col & 0x0000ff00 ) >> 8)  / 255.0f;
        b  = ((col & 0x00ff0000 ) >> 16) / 255.0f;
        o  = 1.0f;
    
        if( !defAlpha) 
            o =((col & 0xff000000 ) >> 24) / 255.0f;        
    }

	float average() { return (r+g+b)/3.0f; }
	col4f invert()  { return col4f( 1.0f-r, 1.0f-g, 1.0f-b); }
	void  set(float* c,  float o_=1.0f) { r=c[0]; g=c[1]; b=c[2]; o=o_;}
	void  set(float r_,  float g_,  float b_,  float o_=1.0f) { r=r_; g=g_; b=b_; o=o_;}
	void  set(double r_, double g_, double b_, double o_=1.0) { r=(float)r_; g=(float)g_; b=(float)b_; o=(float)o_; }
	void  swap() { float t=r; r=b; b=t; }

    float  k() const { return r>g? ( r>b? r : (b>g? b : g) ) : g>b? g : (b>r? b : r); }
    float  k( const col4f& relativeTo ) const {  float diff = (relativeTo.r > 0.0f ? r / relativeTo.r : 0.0f); diff += (relativeTo.g > 0.0f ? g / relativeTo.g : 0.0f); diff += (relativeTo.b > 0.0f ? b / relativeTo.b : 0.0f); return diff / 3.0f; }
    col4f  de_k(float k) const { return fabs(k) > Math::tol ? *this * (1.0f / k) : *this; }
   
	static col4f rand() { return col4f( .2 + ::rand()/(RAND_MAX*2.0), .2 + ::rand()/(RAND_MAX*2.0), .2 + ::rand()/(RAND_MAX*2.0)); }

	operator float*() const { return (float*)&r; }
	col4f  operator* ( float f ) const		{ return col4f( r*f, g*f, b*f ); }
    long tolong(void) const 
    {
       return ((0x000000ff & (int(r * 255) <<  0))
            |  (0x0000ff00 & (int(g * 255) <<  8))
            |  (0x00ff0000 & (int(b * 255) << 16))
			|  (0xff000000 & (int(o * 255) << 24)));
    }

	float r,g,b,o;
};
