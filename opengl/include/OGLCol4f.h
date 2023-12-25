#pragma once
#include <OGLMath.h> 

/** .
class with typical operations for color
 */
class col4f
{
public:
    col4f();
    col4f(float* col, float opacity = 1.0f);
    col4f(const col4f& col, float opacity);
    col4f(const col4f& col, double opacity);
    col4f(float r_, float g_, float b_, float o_ = 1.0f);
    col4f(double r_, double g_, double b_, double o_ = 1.0);
    col4f(long col, bool defAlpha = true);

    void set(float r_, float g_, float b_, float o_ = 1.0f);
    void set(double r_, double g_, double b_, double o_ = 1.0);

    operator const float*() const;
    operator float*();

	float r,g,b,o;
};

inline col4f::col4f() 
{ 
    r = 1.0f; 
    g = 1.0f; 
    b = 1.0f; 
    o = 1.0f; 
}

inline col4f::col4f(float* col, float opacity) 
{ 
    r = col[0]; 
    g = col[1]; 
    b = col[2]; 
    o = opacity; 
}

inline col4f::col4f(const col4f& col, float opacity) 
{ 
    r = col.r; 
    g = col.g; 
    b = col.b; 
    o = opacity; 
}

inline col4f::col4f(const col4f& col, double opacity) 
{ 
    r = col.r; 
    g = col.g; 
    b = col.b; 
    o = float(opacity); 
}

inline col4f::col4f(float r_, float g_, float b_, float o_) 
{ 
    r = r_; 
    g = g_; 
    b = b_; 
    o = o_; 
}

inline col4f::col4f(double r_, double g_, double b_, double o_) 
{ 
    r = (float)r_; 
    g = (float)g_; 
    b = (float)b_; 
    o = (float)o_; 
}

inline col4f::col4f(long col, bool defAlpha)
{
    r = ((col & 0x000000ff) >> 0) / 255.0f;
    g = ((col & 0x0000ff00) >> 8) / 255.0f;
    b = ((col & 0x00ff0000) >> 16) / 255.0f;
    o = 1.0f;

    if (!defAlpha)
        o = ((col & 0xff000000) >> 24) / 255.0f;
}

inline void col4f::set(float r_, float g_, float b_, float o_)
{

}

inline void col4f::set(double r_, double g_, double b_, double o_)
{

}

inline col4f::operator const float* () const
{
    return &r;
}

inline col4f::operator float* ()
{
    return &r;
}
