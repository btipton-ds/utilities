//-----------------------------------------------------------------------------
//
// Copyright 1999, SensAble Technologies, Inc.
//
// File: BitmapUtil.cpp
//
// Created: 8/21/2000
//
//-----------------------------------------------------------------------------

#include <bitmaputil.h>

#include <gdiplus.h>
#include <GdiPlusHeaders.h>
#include <rgbaColor.h>

const int boxpad = 75;
const int align  = 1;
#define PAD_ALIGN(a)

using namespace FS;

//Segment functions
Segment::Segment(int s, int axis, int sliceAt)
{
	m_start = s; m_end = s; m_len = 1; m_changingOnAxis=axis;
	m_constant = sliceAt;
}

void Segment::SetEnd(int e)
{
	m_end = e; m_len = m_end - m_start +1;
}

//returns segment's length
int Segment::ToPoint(int& oX, int& oY)
{
	int middle = (m_start+m_end)/2;
	if( m_changingOnAxis == 0)
	{
		oX = middle;
		oY = m_constant;
	}
	else
	{
		oY = middle;
		oX = m_constant;
	}
	return m_len;
}


namespace
{
//-----------------------------------------------------------------------------
// ColorByIntensityProc
//
// Replace the intensity of each pixel with an interpolated value
// between the background and foreground.  Zero intensity (black)
// becomes background, max intensity (white) is foreground.
//-----------------------------------------------------------------------------
class ColorByIntensityProc : public PixelProcessor
{
public:
    ColorByIntensityProc(const RGBAPixel &foreground,
                         const RGBAPixel &background) :
        m_foreground(foreground),
        m_background(background)
    {}

    virtual RGBAPixel process(const RGBAPixel &input)
    {
        // intensity() considers perceptual contribution
        // of colors: 30% red, 60% green, 10% blue.
        double t = intensity(input)/255.0;

        // Compute the new color values, interpolate between
        // the background and foreground according to the intensity
        const uchar r = lerp(m_background.r(), m_foreground.r(), t);
        const uchar g = lerp(m_background.g(), m_foreground.g(), t);
        const uchar b = lerp(m_background.b(), m_foreground.b(), t);

        // No alpha: make this opaque
        return RGBAPixel(r, g, b);
    }

private:
    RGBAPixel m_foreground, m_background;
};

//-----------------------------------------------------------------------------
// ReplaceColorProc
//
// Replace the intensity of each pixel with an interpolated value
// between the background and foreground.  Zero intensity (black)
// becomes background, max intensity (white) is foreground.
//-----------------------------------------------------------------------------
class ReplaceColorProc : public PixelProcessor
{
public:
    ReplaceColorProc(const RGBAPixel &orig,
                     const RGBAPixel &replace,
					 int tolerance255) :
        m_orig(orig),
        m_repl(replace),
		m_tolerance255(tolerance255)
    {}

    virtual RGBAPixel process(const RGBAPixel &input)
    {
		if (m_tolerance255 > 0)
		{
			int deltaR = abs(input.r() - m_orig.r());
			int deltaG = abs(input.g() - m_orig.g());
			int deltaB = abs(input.b() - m_orig.b());
			int deltaAverage = (int)((double)(deltaR + deltaG + deltaB) / 3.0f);
			return (deltaAverage <= m_tolerance255) ? m_repl : input;
		}

        return (input == m_orig) ? m_repl : input;
    }

private:
    RGBAPixel m_orig, m_repl;
	int m_tolerance255;
};

//-----------------------------------------------------------------------------
// AlphaModulateProc
//
//-----------------------------------------------------------------------------
class AlphaModulateProc : public PixelProcessor
{
public:
    AlphaModulateProc(double alpha) :
        m_alpha(alpha)
    {}

    virtual RGBAPixel process(const RGBAPixel &input)
    {
        uchar newAlpha = input.r() * m_alpha;

        return RGBAPixel(input.r(), input.g(), input.b(), newAlpha);
    }

private:
    double m_alpha;

};

}

//-----------------------------------------------------------------------------
// blitBetweenRGB_RGBA
//
// Blits from CF_RGB to CF_RGBA or vice versa
//-----------------------------------------------------------------------------
void blitBetweenRGB_RGBA( Bitmap* pDst, const Bitmap* pSrc, int x, int y )
{
    int w = pSrc->width();
    if (w > pDst->width() - x)
        w = pDst->width() - x;

    int h = pSrc->height();
    if (h > pDst->height() - y)
        h = pDst->height() - y;

    unsigned char* pSrcBits = pSrc->bits();
    unsigned char* pDstBits = pDst->bits() + y * pDst->bytesPerRow() + 
                                             x * pDst->depth();

    const int srcDepth = pSrc->depth();
    const int dstDepth = pDst->depth();
    const int minDepth = min(srcDepth, dstDepth);
    int i, j, k;

    for (i = 0; i < h; ++i)
    {
        for (j = 0; j < w; ++j)
        {
            for (k = 0; k < minDepth; ++k)
            {
                *(pDstBits++) = *(pSrcBits++);
            }
            if (dstDepth > srcDepth)
            {
                *(pDstBits++) = 255;
            }
            else if (srcDepth > dstDepth)
            {
                ++pSrcBits;
            }
        }
        pSrcBits +=  pSrc->bytesPerRow() - (srcDepth * j);
        pDstBits +=  pDst->bytesPerRow() - (dstDepth * j);
    }
}

//-----------------------------------------------------------------------------
// blitSameColorFormat
//
//-----------------------------------------------------------------------------
void blitSameColorFormat( Bitmap* pDst, const Bitmap* pSrc, int x, int y )
{
    assert( pDst->format() == pSrc->format() );

    // non-color converting
    int w = pSrc->width();
    if (w > pDst->width() - x)
        w = pDst->width() - x;
    w *= pDst->depth();

    int h = pSrc->height();
    if (h > pDst->height() - y)
        h = pDst->height() - y;

    unsigned char* pSrcBits = pSrc->bits();
    unsigned char* pDstBits = pDst->bits() + y * pDst->bytesPerRow() + 
                                             x * pDst->depth();
    for (int i = 0; i < h; ++i)
    {
        memcpy( pDstBits, pSrcBits, w );
        pSrcBits += pSrc->bytesPerRow();
        pDstBits += pDst->bytesPerRow();
    }
}

//-----------------------------------------------------------------------------
// blitSameColorFormat
//
//-----------------------------------------------------------------------------
// WARNING srcy is inverted in this call, srcy=0 is the bottom, not the top.
// So a valid srcy range is (0 to pSrc->height() - height)
void blitSameColorFormat( Bitmap* pDst, const Bitmap* pSrc, int srcx, int srcy,
						  int width, int height, int dstx, int dsty)
{
    assert( pDst->format() == pSrc->format() );
    assert( dstx >= 0 && dstx + width <= pDst->width());
    assert( srcx >= 0 && srcx + width <= pSrc->width());
    assert( dsty >= 0 && dsty + height <= pDst->height());
    assert( srcy >= 0 && srcy + height <= pSrc->height());


    // non-color converting
    int w = width * pDst->depth();
    int h = height;

    unsigned char* pSrcRow = pSrc->bits() + srcy * pSrc->bytesPerRow();
    unsigned char* pDstRow = pDst->bits() + dsty * pSrc->bytesPerRow();

    for (int i = 0; i < h; ++i)
    {
        unsigned char* pSrcBits = pSrcRow + srcx * pSrc->depth();
        unsigned char* pDstBits = pDstRow + dstx * pDst->depth();
        memcpy( pDstBits, pSrcBits, w );
        pSrcRow += pSrc->bytesPerRow();
        pDstRow += pDst->bytesPerRow();
    }
}

// Coefficients to convert and RGB value to an intensity
// The eye doesn't perceive colors equally
#define RED_COEFF   0.299
#define GREEN_COEFF 0.587
#define BLUE_COEFF  0.114

//-----------------------------------------------------------------------------
// blitRGBtoI8
//
//-----------------------------------------------------------------------------
void blitRGBtoI8( Bitmap* pDst, const Bitmap* pSrc, int x, int y )
{
    assert( pSrc->format() == CF_RGB && pDst->format() == CF_I8 );

    // non-color converting
    int w = pSrc->width();
    if (w > pDst->width() - x)
        w = pDst->width() - x;

    int h = pSrc->height();
    if (h > pDst->height() - y)
        h = pDst->height() - y;

    RGBColor *pSrcBits = (RGBColor*)pSrc->bits();
    I8Color* pDstBits = (I8Color*)(pDst->bits() + y * pDst->bytesPerRow() + 
                                                  x * pDst->depth());
    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            pDstBits[j] = pSrcBits[j].r * RED_COEFF +
                          pSrcBits[j].g * GREEN_COEFF +
                          pSrcBits[j].b * BLUE_COEFF;
        }

        pSrcBits += pSrc->width();
        pDstBits += pDst->width();
    }
}

//-----------------------------------------------------------------------------
// blitI8toRGB
//
//-----------------------------------------------------------------------------
void blitI8toRGB( Bitmap* pDst, const Bitmap* pSrc, int x, int y )
{
    assert( pSrc->format() == CF_I8 && pDst->format() == CF_RGB );

    // non-color converting
    int w = pSrc->width();
    if (w > pDst->width() - x)
        w = pDst->width() - x;

    int h = pSrc->height();
    if (h > pDst->height() - y)
        h = pDst->height() - y;

    I8Color *pSrcBits = (I8Color*)pSrc->bits();
    RGBColor* pDstBits = (RGBColor*)(pDst->bits() + y * pDst->bytesPerRow() + 
                                                  x * pDst->depth());
    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            pDstBits[j].r = pSrcBits[j];
            pDstBits[j].g = pSrcBits[j];
            pDstBits[j].b = pSrcBits[j];
        }

        pSrcBits += pSrc->width();
        pDstBits += pDst->width();
    }
}

void blitI8toRGBA( Bitmap* pDst, const Bitmap* pSrc, int x, int y )
{
    assert( pSrc->format() == CF_I8 && pDst->format() == CF_RGBA );

    // non-color converting
    int w = pSrc->width();
    if (w > pDst->width() - x)
        w = pDst->width() - x;

    int h = pSrc->height();
    if (h > pDst->height() - y)
        h = pDst->height() - y;

    I8Color *pSrcBits = (I8Color*)pSrc->bits();
    RGBAColor* pDstBits = (RGBAColor*)(pDst->bits() + y * pDst->bytesPerRow() + 
                                                  x * pDst->depth());
    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            pDstBits[j].r = pSrcBits[j];
            pDstBits[j].g = pSrcBits[j];
            pDstBits[j].b = pSrcBits[j];
            pDstBits[j].a = 255;
        }

        pSrcBits += pSrc->width();
        pDstBits += pDst->width();
    }
}

void blitI16toRGB( Bitmap* pDst, const Bitmap* pSrc, int x, int y )
{
    assert( pSrc->format() == CF_I16 && pDst->format() == CF_RGB );

    // non-color converting
    int w = pSrc->width();
    if (w > pDst->width() - x)
        w = pDst->width() - x;

    int h = pSrc->height();
    if (h > pDst->height() - y)
        h = pDst->height() - y;

    I16Color *pSrcBits = (I16Color*)pSrc->bits();
    RGBColor* pDstBits = (RGBColor*)(pDst->bits() + y * pDst->bytesPerRow() + 
                                                  x * pDst->depth());
    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            pDstBits[j].r = pSrcBits[j]/256;
            pDstBits[j].g = pSrcBits[j]/256;
            pDstBits[j].b = pSrcBits[j]/256;
        }

        pSrcBits += pSrc->width();
        pDstBits += pDst->width();
    }
}

void blitI16toRGBA( Bitmap* pDst, const Bitmap* pSrc, int x, int y )
{
    assert( pSrc->format() == CF_I16 && pDst->format() == CF_RGBA );

    // non-color converting
    int w = pSrc->width();
    if (w > pDst->width() - x)
        w = pDst->width() - x;

    int h = pSrc->height();
    if (h > pDst->height() - y)
        h = pDst->height() - y;

    I16Color *pSrcBits = (I16Color*)pSrc->bits();
    RGBAColor* pDstBits = (RGBAColor*)(pDst->bits() + y * pDst->bytesPerRow() + 
                                                  x * pDst->depth());
    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            pDstBits[j].r = pSrcBits[j]/256;
            pDstBits[j].g = pSrcBits[j]/256;
            pDstBits[j].b = pSrcBits[j]/256;
            pDstBits[j].a = 255;
        }

        pSrcBits += pSrc->width();
        pDstBits += pDst->width();
    }
}

//-----------------------------------------------------------------------------
// blitI16toI8
//
//-----------------------------------------------------------------------------
void blitI16toI8( Bitmap* pDst, const Bitmap* pSrc, int x, int y )
{
    assert( pSrc->format() == CF_I16 && pDst->format() == CF_I8 );

    // non-color converting
    int w = pSrc->width();
    if (w > pDst->width() - x)
        w = pDst->width() - x;

    int h = pSrc->height();
    if (h > pDst->height() - y)
        h = pDst->height() - y;

    I16Color *pSrcBits = (I16Color*)pSrc->bits();
    I8Color* pDstBits = (I8Color*)(pDst->bits() + y * pDst->bytesPerRow() + 
                                                  x * pDst->depth());
    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            pDstBits[j] = pSrcBits[j]/256;
        }

        pSrcBits += pSrc->width();
        pDstBits += pDst->width();
    }
}

// BTW, uchar is in Common.h
typedef unsigned short ushort;

// In converting a 16 bit (0 - 65,535) value to a 8 bit (0 - 255) value,
// keep in mind that 65,535 = 255 * 257
// x^2 - 1 = (x - 1) (x + 1)
//
const int scale_8_to_16 = 257;
const int div_16_to_8 = 257;

// No longer an empty placeholder, this now implements
// a general-purpose format to format converter.
//
void blitPlaceholder( Bitmap* pDst, const Bitmap* pSrc, int x, int y )
{
    // x and y are destination offsets (umm, when do we ever use them?)

    int w = pSrc->width();
    if (w > pDst->width() - x)
        w = pDst->width() - x;

    int h = pSrc->height();
    if (h > pDst->height() - y)
        h = pDst->height() - y;

    uchar  *pSrcBits1 = pSrc->bits();
    ushort *pSrcBits2 = (ushort *)pSrc->bits();

    uchar *pDstBits1 = pDst->bits() + (y * pDst->bytesPerRow()) + 
                                                (x * pDst->depth());
    ushort *pDstBits2 = (ushort *)pDstBits1;

    int srcChannels = pSrc->channels();
    int srcDepth = pSrc->channelDepth();
    assert( 1 <= srcChannels && srcChannels <= 4 );
    assert( srcDepth == 1 || srcDepth == 2 );

    int dstChannels = pDst->channels();
    int dstDepth = pDst->channelDepth();
    assert( 1 <= dstChannels && dstChannels <= 4 );
    assert( dstDepth == 1 || dstDepth == 2 );

    unsigned int pixel[4];	// RGBA16 (holds anything we support)

    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            // 1) Any format to RGBA16 or RGBA8
            if (srcDepth == 1)
            {
                uchar *s1 = pSrcBits1 + (j * srcChannels);
                pixel[0] = s1[0];			// R or I
                if (srcChannels <= 2)
                {
                    pixel[2] = pixel[1] = pixel[0];	// G,B
                    if (srcChannels == 2)
                        pixel[3] = s1[3];	// A
                    else
                        pixel[3] = 0xFF;	// A
                }
                else
                {
                    pixel[1] = s1[1];		// G
                    pixel[2] = s1[2];		// B
                    if (srcChannels == 4)
                        pixel[3] = s1[3];	// A
                    else
                        pixel[3] = 0xFF;	// A
                }

                if (dstDepth == 2) 
                    for( int i = 0; i < 4; ++i )
                        pixel[i] *= scale_8_to_16;
            }
            else // if (srcDepth == 2)
            {
                ushort *s2 = pSrcBits2 + (j * srcChannels);
                pixel[0] = s2[0];			// R or I
                if (srcChannels <= 2)
                {
                    pixel[2] = pixel[1] = pixel[0];	// G,B
                    if (srcChannels == 2)
                        pixel[3] = s2[3];	// A
                    else
                        pixel[3] = 0xFFFF;	// A
                }
                else
                {
                    pixel[1] = s2[1];		// G
                    pixel[2] = s2[2];		// B
                    if (srcChannels == 4)
                        pixel[3] = s2[3];	// A
                    else
                        pixel[3] = 0xFFFF;	// A
                }

                if (dstDepth == 1) 
                    for( int i = 0; i < 4; ++i )
                        pixel[i] /= div_16_to_8;
            }

            // 2) RGBA16 or RGBA8 to any format
            if (dstDepth == 1)
            {
                uchar *d1 = pDstBits1 + (j * dstChannels);
                if (dstChannels <= 2)
                {
                    if (srcChannels <= 2)
                        d1[0] = pixel[0];	// I to I
                    else
                        d1[0] = (RED_COEFF * pixel[0]) + (GREEN_COEFF * pixel[1]) + (BLUE_COEFF * pixel[2]);		// RGB to I
                    if (dstChannels == 2)
                        d1[1] = pixel[3];	// A
                }
                else // if (dstChannels > 2)
                {
                    d1[0] = pixel[0];		// R
                    d1[1] = pixel[1];		// G
                    d1[2] = pixel[2];		// B
                    if (dstChannels == 4)
                        d1[3] = pixel[3];	// A
                }
            }
            else // if (dstDepth == 2)
            {
                ushort *d2 = pDstBits2 + (j * dstChannels);
                if (dstChannels <= 2)
                {
                    if (srcChannels <= 2)
                        d2[0] = pixel[0];	// I to I
                    else
                        d2[0] = (RED_COEFF * pixel[0]) + (GREEN_COEFF * pixel[1]) + (BLUE_COEFF * pixel[2]);		// RGB to I
                    if (dstChannels == 2)
                        d2[1] = pixel[3];	// A
                }
                else // if (dstChannels > 2)
                {
                    d2[0] = pixel[0];		// R
                    d2[1] = pixel[1];		// G
                    d2[2] = pixel[2];		// B
                    if (dstChannels == 4)
                        d2[3] = pixel[3];	// A
                }
            }

        }	// x iter over row pixels

        // Point at next row.

        if (srcDepth == 1)
            pSrcBits1 += (pSrc->width()*srcChannels);
        if (srcDepth == 2)
            pSrcBits2 += (pSrc->width()*srcChannels);

        if (dstDepth == 1)
            pDstBits1 += (pDst->width()*dstChannels);
        if (dstDepth == 2)
            pDstBits2 += (pDst->width()*dstChannels);

    }	// y iter over rows
}

//-----------------------------------------------------------------------------
// g_depth
//
// Bits per pixel for each color format
//-----------------------------------------------------------------------------
static int g_depth[ NUM_COLOR_FORMATS ] =
{
    24, // CF_RGB
    8,  // CF_I8
    32, // CF_RGBA
    16, //  CF_I16
    16, //  CF_IA8
    32  //  CF_IA16
};

typedef void (*BlitFunc)( Bitmap* pDst, const Bitmap* pSrc, int x, int y );

//-----------------------------------------------------------------------------
// g_blitFunc
//
// the format here is g_blitFunc[src format][dst format]
//-----------------------------------------------------------------------------
//
//static BlitFunc g_blitFunc[ NUM_COLOR_FORMATS ][ NUM_COLOR_FORMATS ] =
const int formatTableSize = 4;
static BlitFunc g_blitFunc[ formatTableSize ][ formatTableSize ] =
{
    { blitSameColorFormat, blitRGBtoI8,         blitBetweenRGB_RGBA, blitPlaceholder},	// RGB_to_X
    { blitI8toRGB,         blitSameColorFormat, blitI8toRGBA, blitPlaceholder},			// I8_to_X
    { blitBetweenRGB_RGBA, blitPlaceholder,     blitSameColorFormat, blitPlaceholder},	// RGBA_to_X
    { blitI16toRGB,        blitI16toI8,         blitI16toRGBA, blitSameColorFormat}		// I16_to_X
};

//-----------------------------------------------------------------------------
// copyToBufferRGBtoRGBA
//
// Copies from CF_RGB to CF_RGBA
//-----------------------------------------------------------------------------
void copyToBufferRGBtoRGBA( unsigned char* pDstBits, const Bitmap* pSrc)
{
    //VHTBDBMP no way to check pDstBits size
    assert( pSrc->format() == CF_RGBA );

    int sizeInPixels = pSrc->width() * pSrc->height();
    unsigned char* pSrcBits = pSrc->bits();

    for (int i = 0; i < sizeInPixels; i++)
    {
        *(pDstBits++) = *(pSrcBits++);  // r
        *(pDstBits++) = *(pSrcBits++);  // g
        *(pDstBits++) = *(pSrcBits++);  // b
        *(pDstBits++) = 255;            // a
    }
}

//-----------------------------------------------------------------------------
// copyToBufferRGBAtoRGB
//
// Copies from CF_RGBA to CF_RGB
//-----------------------------------------------------------------------------
void copyToBufferRGBAtoRGB( unsigned char* pDstBits, const Bitmap* pSrc)
{
    // Not implemented
    assert(0);
}

//-----------------------------------------------------------------------------
// copyToBufferSameColorFormat
//
//-----------------------------------------------------------------------------
void copyToBufferSameColorFormat( unsigned char* pDstBits, const Bitmap* pSrc)
{
    //VHTBD: this relies on that the caller does the right thing with format make this less risky
    memcpy((void*) pDstBits, (void*) pSrc->bits(), (size_t) pSrc->getSizeBytes());
}

//-----------------------------------------------------------------------------
// copyToBufferRGBtoI8
//
//-----------------------------------------------------------------------------
void copyToBufferRGBtoI8( unsigned char* pDstBits, const Bitmap* pSrc)
{
    // Not implemented
    assert(0);
}

//-----------------------------------------------------------------------------
// copyToBufferI8toRGB
//
//-----------------------------------------------------------------------------
void copyToBufferI8toRBG( unsigned char* pDstBits, const Bitmap* pSrc)
{
    // Not implemented
    assert(0);
}

void copyToBufferI8toRGBA( unsigned char* pDstBits, const Bitmap* pSrc)
{
    // Not implemented
    assert(0);
}

void copyToBufferRGBAtoI8( unsigned char* pDstBits, const Bitmap* pSrc)
{
    // Not implemented
    assert(0);
}

void copyPlaceholder( unsigned char* pDstBits, const Bitmap* pSrc)
{
    // Not implemented
    assert(0);
}

typedef void (*CopyToBufferFunc)( unsigned char* pDstBits, const Bitmap* pSrc);

//-----------------------------------------------------------------------------
// g_copyToBufferFunc
//
// the format here is g_copyToBufferFunc[src format][dst format]
//-----------------------------------------------------------------------------
// TODO
//static CopyToBufferFunc g_copyToBufferFunc[ NUM_COLOR_FORMATS ][ NUM_COLOR_FORMATS ] =
static CopyToBufferFunc g_copyToBufferFunc[ formatTableSize ][ formatTableSize ] =
{
    { copyToBufferSameColorFormat, copyToBufferRGBtoI8,         copyToBufferRGBtoRGBA, copyPlaceholder },
    { copyToBufferI8toRBG,         copyToBufferSameColorFormat, copyToBufferI8toRGBA, copyPlaceholder },
    { copyToBufferRGBAtoRGB,       copyToBufferRGBAtoI8,        copyToBufferSameColorFormat, copyPlaceholder },
    { copyPlaceholder, copyPlaceholder, copyPlaceholder, copyToBufferSameColorFormat }
};


//-----------------------------------------------------------------------------
// accessors
//-----------------------------------------------------------------------------

int Bitmap::width() const
{
    return m_width;
}

int Bitmap::height() const
{
    return m_height;
}

// Calculate image size.
int Bitmap::getSizeBytes() const
{
    return width() * height() * depth() * sizeof(unsigned char);
}

ColorFormat Bitmap::format() const
{
    assert( m_colorFormat < NUM_COLOR_FORMATS );
    return (ColorFormat)m_colorFormat;
}

int Bitmap::depth() const
{
    return Bitmap::depth(m_colorFormat);
}

int Bitmap::depth(int colorFormatIn)
{
    assert( colorFormatIn < NUM_COLOR_FORMATS );
    return g_depth[colorFormatIn] >> 3;
}

int Bitmap::channelDepth() const
{
    return Bitmap::channelDepth(format());
}

int Bitmap::channelDepth(ColorFormat fmt)
{
    switch (fmt)
    {
        case CF_RGB:
        case CF_RGBA:
        case CF_I8:
        case CF_IA8:
            return 1;
        case CF_I16:
        case CF_IA16:
            return 2;
        default:
            assert(!"unknown image format");
            return 0;
    }
}

int Bitmap::channels() const
{
    return Bitmap::channels(format());
}

int Bitmap::channels(ColorFormat fmt)
{
    switch (fmt)
    {
        case CF_RGB: return 3;
        case CF_RGBA: return 4;
        case CF_I8:
        case CF_I16:
            return 1;
        case CF_IA8:
        case CF_IA16:
            return 2;
        default:
            assert(!"unknown image format");
            return 0;
    }
}

int Bitmap::depthInBits() const
{
    assert( m_colorFormat < NUM_COLOR_FORMATS );
    return g_depth[m_colorFormat];
}

int Bitmap::bytesPerRow() const
{
    return m_width * (depthInBits() >> 3);
}

unsigned char* Bitmap::bits() const
{
    return m_pBits;
}

//-----------------------------------------------------------------------------
// getGLFormat
//
//-----------------------------------------------------------------------------
GLenum Bitmap::getGLFormat() const { return getGLFormat(format()); }
GLenum Bitmap::getGLFormat(ColorFormat fmt)
{
    switch (fmt)
    {
        case CF_RGB: return GL_RGB;
        case CF_RGBA: return GL_RGBA;
        case CF_I8: return GL_LUMINANCE;

        case CF_I16:
            // Note that GL_LUMINANCE16 is an enum specific to texturing.
            // Calls like gluScaleImage and gluBuild2DMipmaps take a separate
            // data type parameter to specify 8 bits vs 16 bits.
            return GL_LUMINANCE;

        case CF_IA8:
        case CF_IA16:
            return GL_LUMINANCE_ALPHA;

        default:
            assert(!"unknown image format");
            return 0;
    }
}

//-----------------------------------------------------------------------------
// getGLDataType
//
//-----------------------------------------------------------------------------
GLenum Bitmap::getGLDataType() const { return getGLDataType(format()); }
GLenum Bitmap::getGLDataType(ColorFormat fmt)
{
    switch (fmt)
    {
        case CF_RGB:
        case CF_RGBA:
        case CF_I8:
        case CF_IA8:
            return GL_UNSIGNED_BYTE;
        case CF_I16:
        case CF_IA16:
            return GL_UNSIGNED_SHORT;
        default:
            assert(!"unknown image format");
            return 0;
    }
}

//-----------------------------------------------------------------------------
// getGLTextureFormat
//
// OpenGL docs call this the texture "Sized Internal Format" which
// is used for the 'internalformat' parameter to GL texture calls
// such as glTexImage2D.
//-----------------------------------------------------------------------------
GLint Bitmap::getGLTextureFormat() const { return getGLTextureFormat(format()); }
GLint Bitmap::getGLTextureFormat(ColorFormat fmt)
{
    switch (fmt)
    {
        case CF_RGB:
            return GL_RGB8;
        case CF_RGBA:
            return GL_RGBA8;
        case CF_I8:
            return GL_LUMINANCE8;
        case CF_I16:
            return GL_LUMINANCE16;
        default:
            assert(!"unknown image format");
            return 0;
    }
}

//-----------------------------------------------------------------------------
// getPixel
//
//-----------------------------------------------------------------------------
void* Bitmap::getPixel( int x, int y ) const
{
    // early out if we can
    if ( (!m_pBits) || ( x < 0) || (x >= m_width) || (y < 0) || (y >= m_height) )
        return 0;

    return m_pBits + y * bytesPerRow() + x * depth();
}

//-----------------------------------------------------------------------------
// getRGBAPixel
//
// Lookup a pixel from the bitmap
//-----------------------------------------------------------------------------
bool Bitmap::getRGBAPixel( int x, int y, RGBAPixel &result ) const
{
    switch (format())
    {
        case CF_RGB:
            if (RGBColor *p = (RGBColor *)getPixel(x, y))
            {
                result.set(p->r, p->g, p->b, 255);
                return true;
            }
            break;
        case CF_RGBA:
            if (RGBAColor *p = (RGBAColor *)getPixel(x, y))
            {
                result.set(p->r, p->g, p->b, p->a);
                return true;
            }
            break;
        case CF_I8:
            if (I8Color *p = (I8Color *)getPixel(x, y))
            {
                result.set(*p, *p, *p, 255);
                return true;
            }
            break;
        case CF_IA8:
            if (I8Color *p = (I8Color *)getPixel(x, y))
            {
                result.set(*p, *p, *p, *(p+1));
                return true;
            }
            break;
        case CF_I16:
            if (I16Color *p = (I16Color *)getPixel(x, y))
            {
                result.set(*p/256, *p/256, *p/256, 255);
                return true;
            }
            break;
        case CF_IA16:
            if (I16Color *p = (I16Color *)getPixel(x, y))
            {
                result.set(*p/256, *p/256, *p/256, (*(p+1))/256);
                return true;
            }
            break;
        default:
            assert(!"unknown format");
            return false;
    }

    assert(!"bad lookup");
    return false;
}

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

Bitmap::Bitmap() 
    : m_pBits(0)
    , m_width(0)
    , m_height(0)
    , m_colorFormat(0)
    , m_GlType( GL_TEXTURE_2D )
{
}

Bitmap::Bitmap(int width, int height, ColorFormat fmt) 
    : m_pBits(0)
    , m_width(width)
    , m_height(height)
    , m_colorFormat(fmt)
    , m_GlType( GL_TEXTURE_2D )
{
	init(width, height, fmt);	// alloc the bits
}

Bitmap::Bitmap(const Bitmap& Bmp)
{
    m_GlType = Bmp.m_GlType;
	m_width = Bmp.m_width;
    m_height = Bmp.m_height;
    m_colorFormat = Bmp.m_colorFormat;

	m_pBits = new unsigned char[ m_width * m_height * (depthInBits() >> 3) ];
	memcpy( m_pBits, Bmp.m_pBits, m_height * bytesPerRow() );
}

Bitmap::~Bitmap()
{
    //HLOG("~Bitmap()");
    delete_array(m_pBits);
}

// Move constructor
Bitmap::Bitmap( Bitmap&& other )
  : m_GlType( other.m_GlType )
  , m_width( other.m_width )
  , m_height( other.m_height )
  , m_colorFormat( other.m_colorFormat )
  , m_pBits( other.m_pBits )
{
    other.m_pBits = NULL;

    other.m_GlType = GL_TEXTURE_2D;
    other.m_width = 0;
    other.m_height = 0;
    other.m_colorFormat = CF_INVALID;
}

// Move assignment operator
Bitmap& Bitmap::operator=( Bitmap&& other )
{
	if (this == &other)
		return *this;

    m_GlType      = other.m_GlType;
    m_width       = other.m_width;
    m_height      = other.m_height;
    m_colorFormat = other.m_colorFormat;

    delete_array(m_pBits);
    m_pBits = other.m_pBits;
    other.m_pBits = NULL;

    other.m_GlType = GL_TEXTURE_2D;
    other.m_width = 0;
    other.m_height = 0;
    other.m_colorFormat = CF_INVALID;

    return *this;
}

Bitmap& Bitmap::operator = (const Bitmap& rhs)
{
    delete_array(m_pBits);

	m_GlType = rhs.m_GlType;
	m_width = rhs.m_width;
    m_height = rhs.m_height;
    m_colorFormat = rhs.m_colorFormat;

	m_pBits = new unsigned char[ m_width * m_height * (depthInBits() >> 3) ];
	memcpy( m_pBits, rhs.m_pBits, m_height * bytesPerRow() );
	return *this;
}

//-----------------------------------------------------------------------------
// initializes a bitmap
//-----------------------------------------------------------------------------

bool Bitmap::init( int width, int height, ColorFormat fmt )
{
    assert( m_colorFormat < NUM_COLOR_FORMATS );
    assert( width > 0 );
    assert( height > 0 );

    m_colorFormat = fmt;
    m_width = width;
    m_height = height;

    // clear current state
    delete_array(m_pBits);

    // allocate new state
    m_pBits = new unsigned char[ m_width * m_height * (depthInBits() >> 3) ];
	if(!m_pBits)
		printf("ERROR in allocating m_pBits\n");
    assert( m_pBits );

    return true;
}

bool Bitmap::loadBmpFromActiveRenderBuffer( ColorFormat fmt )
{
    if(!wglGetCurrentContext())
        return false;

    GLenum glformat=0, gltype=0;
    switch(fmt)
    {
        case CF_RGBA: glformat=GL_RGBA/* this was erroneously GL_BGRA_EXT*/;      gltype=GL_UNSIGNED_BYTE; break;
        // LIMBO really need a flag as to which channel you want
        // if you are in a one channel framebuffer you can't fetch alpha
        case CF_I8:   glformat=GL_LUMINANCE;     gltype=GL_UNSIGNED_BYTE; break;

        default: assert(!"unsupported"); return false;
    };

	glFlush();
	GLint vp[4];
	glGetIntegerv( GL_VIEWPORT, vp ); GL_ASSERT;
    init( vp[2], vp[3], fmt );

	OglPixelStore ps1(GL_PACK_ROW_LENGTH, 0);
	OglPixelStore ps2(GL_PACK_ALIGNMENT, 1);
	OglPixelStore ps3(GL_PACK_SKIP_ROWS, 0);
	OglPixelStore ps4(GL_PACK_SKIP_PIXELS, 0);


	glReadPixels ( vp[0], vp[1], vp[2], vp[3], glformat, gltype, m_pBits );
	GLenum error = glGetError();
	assert(error==GL_NO_ERROR);

    return true;
}

//-----------------------------------------------------------------------------
// loadBmp
// 
// initializes a bitmap from a file.  Uses OleLoadPicture to 
// handle GIF, JPG, and BMP formats.
// Return true on success
//-----------------------------------------------------------------------------
bool Bitmap::loadBmp( TCHAR const* pFileName, ColorFormat fmt, bool convertFmt )
{
    bool result = true;

    StringT fileName = pFileName;
    StringT fileExt = fileName.getFileExtension();
    if (fileExt.comparei(_T(".psd")))
    {
        try
        {
            // Get the Photoshop image as a bitmap.
            PhotoshopImage image(fileName);

            Bitmap tempBitmap;
            image.getBitmap(tempBitmap);

            // Keep the native format.
            if (tempBitmap.format() == fmt || fmt == CF_UNSPECIFIC)
            {
                // Efficient because it uses the move assignment operator.
                *this = std::move(tempBitmap);
            }
            // Convert the Photoshop bitmap to the requested format.
            else if (convertFmt)
            {
                init(tempBitmap.width(), tempBitmap.height(), fmt);
                blit(&tempBitmap, 0, 0);
            }
            else result = false;
        }
        catch(const ErrorID &err)
        {
            err;
            result = false;
            throw;
        }
        catch(const Error &err)
        {
            err;
            result = false;
            throw;
        }
    }
	else if (fileExt.comparei(_T(".tif")))
	{
		// OleLoadPictureFile() doesn't support .tif, so need to use Gdiplus::Bitmap::FromFile()
		// Could use this instead: http://www.libtiff.org/libtiff.html

		using namespace Gdiplus;

		GdiplusStartupInput gdiplusStartupInput;
		ULONG_PTR gdiplusToken;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

		Gdiplus::Bitmap* bm_plus = Gdiplus::Bitmap::FromFile(pFileName, FALSE);
		int err = GetLastError();
		if (bm_plus)
		{
			if (err == 0)
			{
				// Get the bitmap handle
				HBITMAP hBitmap = NULL;
				Gdiplus::Color bgColor(0,0,0); // should be black for DICOM... but tiff isn't transparent anyway
				bm_plus->GetHBITMAP(bgColor, &hBitmap);
				if (hBitmap)
				{
					loadBmp(hBitmap, fmt);
					DeleteObject( hBitmap );
				}
			}
			else
				result = false;

			delete bm_plus;
		}
		else
			result = false;

		GdiplusShutdown(gdiplusToken);
	}
    else if (fileExt.comparei(_T(".png")))
    {
        PngFile png;
        bool stripAlpha = ((fmt == CF_RGBA || fmt == CF_UNSPECIFIC) ? false : true);
        if (0 != png.read(fileName, stripAlpha))
            return false;

        const size_t sz1 = png.getBufferSize();
        const int pixels = png.width() * png.height();

        const int channels = png.numChannels();
        const PngFile::ChannelDepth depth = png.getChannelDepth();

        result = true;

        if( fmt == CF_UNSPECIFIC )
        {
            // CF_UNSPECIFIC - Keep the native format of the png file.
            if( channels == 1 && depth == PngFile::CD_8 )
            {
                init( png.width(), png.height(), CF_I8 );
                result = png.getGrayImage( m_pBits, sz1, m_width, m_height );
            }
            else if( channels == 1 && depth == PngFile::CD_16 )
            {
                init( png.width(), png.height(), CF_I16 );
                unsigned short *p2 = (unsigned short *)m_pBits;
                result = png.getGrayImage( p2, sz1, m_width, m_height );
            }
            else if( channels == 2 && depth == PngFile::CD_8 )
            {
                init( png.width(), png.height(), CF_IA8 );
                memcpy(bits(), png.getImagePtr(), getSizeBytes());
            }
            else if( channels == 2 && depth == PngFile::CD_16 )
            {
                init( png.width(), png.height(), CF_IA16 );
                memcpy(bits(), png.getImagePtr(), getSizeBytes());
            }
            else if( channels == 3 && depth == PngFile::CD_8 )
            {
                init( png.width(), png.height(), CF_RGB );
                result = png.getRGBImage( m_pBits, sz1, m_width, m_height );
            }
            else if( channels == 4 && depth == PngFile::CD_8 )
            {
                init( png.width(), png.height(), CF_RGBA );
                result = png.getRGBAImage( m_pBits, sz1, m_width, m_height );
            }
            // Convert RGBx16 to RGBx8 since FF doesn't (yet) have CF_RGB16.
            else if( channels == 3 && depth == PngFile::CD_16 )
            {
                fmt = CF_RGB;
                convertFmt = true;
                init( png.width(), png.height(), CF_RGB );
                // Fall through to conversion code below
            }
            // Convert RGBAx16 to RGBAx8 since FF doesn't (yet) have CF_RGBA16.
            // We don't have a good way to indicate that the tool would rather
            // have I8 ... that is the weakness of this approach.
            else if( channels == 4 && depth == PngFile::CD_16 )
            {
                fmt = CF_RGBA;
                convertFmt = true;
                init( png.width(), png.height(), CF_RGBA );
                // Fall through to conversion code below
            }
            else {
                result = false;
            }
        }
        else
        {
            // Caller resquested a specific format
            init( png.width(), png.height(), fmt );
        }

        unsigned char *p2 = m_pBits;

        if( fmt == CF_RGB ) {
            if( png.numChannels() == 3 &&
                png.getChannelDepth() == PngFile::CD_8 )
                result = png.getRGBImage( m_pBits, sz1, m_width, m_height );

            else if( png.numChannels() == 3 &&
                     png.getChannelDepth() == PngFile::CD_16 &&
                     convertFmt ) {
                const unsigned short *p1 = (unsigned short *)png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
                    *p2++ = *p1++/256;
                    *p2++ = *p1++/256;
                    *p2++ = *p1++/256;
                }
            }

            else if( png.numChannels() == 4 && convertFmt ) {
                // Now that I've added the strip-alpha-on-read flag, 
                // this combination can't happen.
                const unsigned char *p1 = png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
                    *p2++ = *p1++;
                    *p2++ = *p1++;
                    *p2++ = *p1++;
                    p1++;
                }
            }
            else if( (png.numChannels() == 1 || png.numChannels() == 2) && 
                     png.getChannelDepth() == PngFile::CD_8 &&
                     convertFmt ) {
                const int nc = png.numChannels();
                const unsigned char *p1 = png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
				    *p2++ = *p1;
				    *p2++ = *p1;
				    *p2++ = *p1;
				    p1 += nc;
                }
            }
            else if( (png.numChannels() == 1 || png.numChannels() == 2) && 
                     png.getChannelDepth() == PngFile::CD_16 &&
                     convertFmt ) {
                const int nc = png.numChannels();
                const unsigned short *p1 = (unsigned short *)png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
				    *p2++ = *p1/256;
				    *p2++ = *p1/256;
				    *p2++ = *p1/256;
				    p1 += nc;
                }
            }
            else result = false;
        }
        else if( fmt == CF_RGBA ) {
            if( png.numChannels() == 4 &&
                png.getChannelDepth() == PngFile::CD_8 )
                result = png.getRGBAImage( m_pBits, sz1, m_width, m_height );

            else if( png.numChannels() == 4 &&
                     png.getChannelDepth() == PngFile::CD_16 &&
                     convertFmt ) {
                const unsigned short *p1 = (unsigned short *)png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
                    *p2++ = *p1++/256;
                    *p2++ = *p1++/256;
                    *p2++ = *p1++/256;
                    *p2++ = *p1++/256;
                }
            }

            else if( png.numChannels() == 3 &&
                     png.getChannelDepth() == PngFile::CD_8 ) {
                const unsigned char *p1 = png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
				    *p2++ = *p1++;
				    *p2++ = *p1++;
				    *p2++ = *p1++;
				    *p2++ = 255;
                }
            }

            else if( png.numChannels() == 3 &&
                     png.getChannelDepth() == PngFile::CD_16 &&
                     convertFmt ) {
                const unsigned short *p1 = (unsigned short *)png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
                    *p2++ = *p1++/256;
                    *p2++ = *p1++/256;
                    *p2++ = *p1++/256;
                    *p2++ = 255;
                }
            }

            else if( (png.numChannels() == 1 || png.numChannels() == 2) && 
                     png.getChannelDepth() == PngFile::CD_8 &&
                     convertFmt ) {
                const int nc = png.numChannels();
                const unsigned char *p1 = png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
				    *p2++ = *p1;
				    *p2++ = *p1;
				    *p2++ = *p1++;
					if (nc == 1)	// Input is I
						*p2++ = 255;
					else if (nc == 2)	// Input is IA
						*p2++ = *p1++;
                }
            }
            else if( (png.numChannels() == 1 || png.numChannels() == 2) && 
                     png.getChannelDepth() == PngFile::CD_16 &&
                     convertFmt ) {
                const int nc = png.numChannels();
                const unsigned short *p1 = (unsigned short *)png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
				    *p2++ = *p1/256;
				    *p2++ = *p1/256;
				    *p2++ = *p1++/256;
					if (nc == 1)	// Input is I
						*p2++ = 255;
					else if (nc == 2)	// Input is IA
						*p2++ = *p1++/256;
                }
            }
            else result = false;
        }
        else if( fmt == CF_I8 ) {
            if( png.numChannels() == 1 && 
                png.getChannelDepth() == PngFile::CD_8 ) {
                result = png.getGrayImage( m_pBits, sz1, m_width, m_height );
            }
            else if( (png.numChannels() == 1 || png.numChannels() == 2) && 
                     png.getChannelDepth() == PngFile::CD_16 &&
                     convertFmt ) {
                const int nc = png.numChannels();
                const unsigned short *p1 = (unsigned short *)png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
                    *p2 = *p1 / 256;
                    p1 += nc;
                    p2++;
                }
            }
            else if( png.numChannels() == 2 && 
                     png.getChannelDepth() == PngFile::CD_8 &&
                     convertFmt ) {
                const unsigned char *p1 = png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
                    *p2 = *p1 / 256;
                    p1 += 2;
                    p2++;
                }
            }
            else if( png.numChannels() == 3 && 
                     png.getChannelDepth() == PngFile::CD_8 &&
                     convertFmt ) {
                const unsigned char *p1 = png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
                    *p2 = (p1[0] * RED_COEFF) + (p1[1] * GREEN_COEFF) + (p1[2] * BLUE_COEFF);
                    p1 += 3;
                    p2++;
                }
            }
            else if( png.numChannels() == 4 && 
                     png.getChannelDepth() == PngFile::CD_8 &&
                     convertFmt ) {
                // Now that I've added the strip-alpha-on-read flag, 
                // this combination can't happen.
                const unsigned char *p1 = png.getImagePtr();
                for( int i = 0; i < pixels; ++i ) {
                    *p2 = (p1[0] * RED_COEFF) + (p1[1] * GREEN_COEFF) + (p1[2] * BLUE_COEFF);
                    p1 += 4;
                    p2++;
                }
            }
            else result = false;
        }
        else if( fmt == CF_I16 ) {
            if( png.numChannels() == 1 &&
                png.getChannelDepth() == PngFile::CD_16 ) {
                result = png.getGrayImage( (unsigned short *)m_pBits, sz1, m_width, m_height );
            }
            else if( (png.numChannels() == 1 || png.numChannels() == 2) && 
                     png.getChannelDepth() == PngFile::CD_8 &&
                     convertFmt ) {
                const unsigned char *p1 = png.getImagePtr();
                const int nc = png.numChannels();
                for( int i = 0; i < pixels; ++i ) {
                    *(unsigned short *)p2 = *p1 * 256;
                    p1 += nc;
                    p2 += 2;	// p2 is a byte ptr
                }
            }
            // channels == 1 is actually covered above
            else if( (png.numChannels() == 1 || png.numChannels() == 2) && 
                     png.getChannelDepth() == PngFile::CD_16 &&
                     convertFmt ) {
                const unsigned short *p1 = (unsigned short *)png.getImagePtr();
                const int nc = png.numChannels();
                for( int i = 0; i < pixels; ++i ) {
                    *(unsigned short *)p2 = *p1;
                    p1 += nc;
                    p2 += 2;	// p2 is a byte ptr
                }
            }
            // TODO: Convert from color formats
            else result = false;
        }
    }	// png
    else if ((fmt == CF_RGB) || (fmt == CF_RGBA) || (fmt == CF_UNSPECIFIC))
    {
        // Handle all Windows supported file types.
        result = loadImageFromFile(pFileName, fmt);

        // make sure we succeeded as promised.
        assert( result == ( m_pBits != 0 ) );
    }
    else result = false;

    return result;
}

//-----------------------------------------------------------------------------
// loadBmp
//
// Load bitmap from the given bitmap handle.  This load maintains alpha
// from the given bitmap handle if it has it and the requested format
// is CF_RGBA, otherwise the alpha is removed.  If the given bitmap handle
// doesn't contain an alpha channel but one is requested, it is synthesized
// (i.e. all opaque).  Formats other than CF_RGBA and CF_RGB are not 
// supported. 
//-----------------------------------------------------------------------------
bool Bitmap::loadBmp( HBITMAP hBitmap, ColorFormat fmt )
{
    if ((fmt != CF_RGB) && (fmt != CF_RGBA) && (fmt != CF_UNSPECIFIC))
    {
        assert(!"direct load to this format not supported");
        return false;
    }

    if ( hBitmap == NULL )
    {
        return false;
    }

    // retrieve bitmap values
    BITMAPINFO bmpInfo;
    memset( &bmpInfo, 0, sizeof( BITMAPINFO ) );
    bmpInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    HDC hDC = ::CreateDC( _T("DISPLAY"), NULL, NULL, NULL );
    int iRet = ::GetDIBits(hDC, hBitmap, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);

    // if getting the bitmap info failed
    if ( iRet == 0 )
        return false;

    int srcNumBitsNative = bmpInfo.bmiHeader.biBitCount;

    // For some reason, Windows files tend to be 32 bit, even if there is 
    // no transparency information.  So scale to 24 bit if the caller is 
    // looking for CF_RGB only. - PR 5570
    // Bump this up to 24, so GetDIBits will convert lower
    // paletted formats (16, 8) to 24-bit for us. - PR 5475
    if (srcNumBitsNative <= 24 || fmt == CF_RGB)
    {
        if (fmt == CF_UNSPECIFIC) fmt = CF_RGB;
        bmpInfo.bmiHeader.biBitCount = 24;
    }
    else if (srcNumBitsNative == 32 && fmt == CF_UNSPECIFIC)
    {
        // It's actually pretty hard to find a sample BMP out in
        // "the wild" that really has 32 bits and an alpha channel 
        // I guess PNG dominates these days.
        //
        // However we have some in our GUI directory that are used for icons
        // E.g. FF/data/GUI/curves/Select.bmp
        //
        fmt = CF_RGBA;
    }
    else if (srcNumBitsNative != 32)
    {
        // We currently expect <= 24 or 32 bits only
        assert(!"unexpected bmp format");
        return false;
    }

    int srcNumBits = bmpInfo.bmiHeader.biBitCount;

    // save width and height and allocate space
    init( bmpInfo.bmiHeader.biWidth,
          bmpInfo.bmiHeader.biHeight,
          fmt );

    // Compute padding in bytes
    int bytesInLine = ((((m_width * srcNumBits) + 31) / 32) * 4);
    int rowPadding = bytesInLine - (((m_width * srcNumBits) + 7) / 8);
    int totalPadding = rowPadding * m_height;

    // Create storage for GetDIBits to copy the bits from the bitmap
    // handle.  
    BYTE *bits = new unsigned char [ width() * height() * 
                                     srcNumBits/8 + totalPadding];

    bmpInfo.bmiHeader.biCompression = BI_RGB;  // We don't want the bmiColors 
                                               // field of BITMAPINFO filled 
                                               // in.  For 32 bit images at
                                               // least, biCompression will not
                                               // be RI_RGB.  This just fills
                                               // in the color table which we
                                               // don't need.  If we decide
                                               // we do want it, we need to 
                                               // allocate space for it.
    iRet = ::GetDIBits(hDC, hBitmap, 0, height(), bits, &bmpInfo, DIB_RGB_COLORS);

    // if getting the bitmap bits pointer failed
    if ( iRet == 0 )
        return false;

    // setup pointers
    uchar *pInput = bits;
    uchar *pOutput = m_pBits;

    uchar alphaSum = 0;

    // copy all the bits to our storage
    for ( int j = 0; j < height(); j++ )
    {
        for ( int i = 0; i < width(); i++ )
        {
            // DIB data is in BGR order
            uchar blue = *pInput++;
            uchar green = *pInput++;
            uchar red = *pInput++;

            // Our format is RGB order
            *pOutput++ = red;
            *pOutput++ = green;
            *pOutput++ = blue;

            if (format() == CF_RGBA)
            {
                // Transfer the alpha if we have it.  Otherwise, just set it
                // to opaque.
                uchar alpha = (srcNumBits == 32) ? *pInput++ : 255;
                alphaSum |= alpha;
                *pOutput++ = alpha;
            }
        }

        // increase input pointer for padding
        pInput += rowPadding;
    }

    // 100% transparent bmp file? More likely that the alpha channel is garbage.
    // This has been known to happen from images embedded in resource files
    // E.g. libsrc/UI/WrapTextureUI/res/wrap_tex.bmp (but if you read the
    // file directly, it does not claim to be a 32-bit file).
    if ((fmt == CF_RGBA) && (srcNumBits == 32) && (alphaSum == 0))
    {
        pOutput = m_pBits;
        for ( int j = 0; j < height(); j++ )
        {
            for ( int i = 0; i < width(); i++ )
            {
                pOutput += 3;		// skip RGB
                *pOutput++ = 255;	// force opaque alpha
           }
        }
    }

    // cleanup
    delete_array( bits );
    ::DeleteDC( hDC );

    return true;        // return true on success
}

//-----------------------------------------------------------------------------
// initializes a bitmap from a Windows Resource ID
//-----------------------------------------------------------------------------

bool Bitmap::loadBmp( unsigned int nIDResource, ColorFormat fmt )
{
    // load the bitmap from the resources
    HBITMAP hBitmap = getResourceBitmap( nIDResource );

    // if loading the bitmap failed
    if ( hBitmap == NULL )
        return false;

    // if creating the bitmap from the handle fails
    if ( loadBmp( hBitmap, fmt ) != true )
        return false;

    // release handle
    ::DeleteObject( hBitmap );

    return true;
}

//-----------------------------------------------------------------------------
// initializes a bitmap from a width, height and OpenGL bits
//-----------------------------------------------------------------------------

bool Bitmap::loadBmp( 
    int widthIn, 
    int heightIn, 
    unsigned char *pBitsIn, 
    int colorFormatIn
)
{
    // If we're loading the same image, skip it.
    if (m_pBits == pBitsIn)
    {
        return true;
    }

    ColorFormat format = CF_RGBA;
    if (colorFormatIn != -1)
    {
        format = (ColorFormat)colorFormatIn;
    }

    // save width and height and allocate space
    init(widthIn, heightIn, format);

    // copy the image to allocated storage 
    memcpy( m_pBits, pBitsIn, m_height * bytesPerRow() );

    return true;
}


//-----------------------------------------------------------------------------
// clear
//
// initializes a bitmap to its original constructed state 
//-----------------------------------------------------------------------------

bool Bitmap::clear()
{
	clearBinding();

    delete_array(m_pBits);
    m_width = 0;
    m_height = 0;
    m_colorFormat = 0;

    return true;
}

//-----------------------------------------------------------------------------
// blit
//
// Copies a bitmap, converts formats as needed
//-----------------------------------------------------------------------------
void Bitmap::blit( const Bitmap* pSrc, int x, int y )
{
    assert( pSrc );

    // early out if we can
    if ( (x >= m_width) || (y >= m_height) )
        return;

    // 4 is number of formats where I gave up on special-casing
    // every possible combination. So g_blitFunc is a 4 x 4 table.
    if ((pSrc->m_colorFormat < formatTableSize) && 
        (m_colorFormat < formatTableSize))
    {
        // call the appropriate blitter function
        (*g_blitFunc[pSrc->m_colorFormat][m_colorFormat])( this, pSrc, x, y );
    }
    else
    {
        // No longer an empty placeholder, this now implements a
        // a general-purpose format to format converter.
        blitPlaceholder( this, pSrc, x, y );
    }
}

// Bugzilla 872 Added. Copies and converts as needed to desired format in 
// generic destination buffer. Size of buffer must be 
// image width * height * bytes per pixel.
// Considered adding a "blitToBuffer" method similar to the blit methods above,
// but chose simpler copy of the entire image buffer. Currently are no callers
// of blit that copy anything smaller than the entire buffer.
void Bitmap::copyToBuffer(unsigned char *imageBits, ColorFormat format) const
{
    // VHTBDBMP we need a better implementation of this, make image bits a structure so we can verify 
    // dimensions
    assert(imageBits);

    // call the appropriate copy function
    if ((m_colorFormat < formatTableSize) &&
        (format < formatTableSize))
    {
        (*g_copyToBufferFunc[m_colorFormat][format])(imageBits, this);
    }
    else
        assert( !"not implemented" );
}

//-----------------------------------------------------------------------------
// alphaKey
//
// Set alpha of all pixels with a given color to 0 and all other pixel
// alphas to 1. also replace that pixel with the passed in color.
// This makes all pixels of the given color transparent if you set the
// alpha test to be greater than zero when drawing.
// This works like chroma key in the video world.
//-----------------------------------------------------------------------------
void Bitmap::alphaKey( const RGBPixel &keyColor,
                       const RGBPixel &replaceColor,
					   int tolerance255)
{
    assert( format() == CF_RGBA );

    // setup pointers
    unsigned char *out_ptr = m_pBits;

    // iterate through all the pixels
    for ( int i = 0; i < width() * height(); i++ )
    {
        unsigned char *currPix = out_ptr;
        unsigned char red = *out_ptr++;
        unsigned char green = *out_ptr++;
        unsigned char blue = *out_ptr++;

        // if the key color is equal to our current pixel
		if (tolerance255 > 0)
		{
			int deltaR = abs((int)keyColor.r() - (int)red);
			int deltaG = abs((int)keyColor.g() - (int)green);
			int deltaB = abs((int)keyColor.b() - (int)blue);
			int deltaAverage = (int)((double)(deltaR + deltaG + deltaB) / 3.0f);
			if (deltaAverage <= tolerance255)
			{
				// set alpha to 0 and replace pixel color
				*out_ptr++ = 0;
				*currPix++ = replaceColor.r();
				*currPix++ = replaceColor.g();
				*currPix++ = replaceColor.b();
			}
			else
			{
				// set alpha to 1
				*out_ptr++ = 255;
			}
		}
		else
		{
			if (keyColor.r() == red &&
				keyColor.g() == green &&
				keyColor.b() == blue)
			{
				// set alpha to 0 and replace pixel color
				*out_ptr++ = 0;
				*currPix++ = replaceColor.r();
				*currPix++ = replaceColor.g();
				*currPix++ = replaceColor.b();
			}
			else
			{
				// set alpha to 1
				*out_ptr++ = 255;
			}
		}
    }
}

//-----------------------------------------------------------------------------
// scale
//
// scales bitmap by a percentage.
// 
// Note redundancy with both resize() and the other scale() overloads.
//
//-----------------------------------------------------------------------------
void Bitmap::scale( double percent )
{
    int newWidth = m_width * percent;
    int newHeight = m_height * percent;
    if (newWidth < 1 || newHeight < 1)
    {
        return;
    }

    resize( newWidth, newHeight );
}

//-----------------------------------------------------------------------------
// scale
//
// scales image with filtering
//-----------------------------------------------------------------------------
void Bitmap::scale( int newWidth, int newHeight )
{
    unsigned char *cBits = 0;

    // Use box filter for downsampling and bilinear filer for upsampling
    // Note: The C2PassScale facility is templatized, so we can't easily
    // work around these case statements.

    // Ideally we should be performing this 2PassScale in each dimension
    // separately, so we can choose box filter or bilinear filter.
    // For now, we assume we are upsampling or downsampling both
    // dimensions together.
    assert((newWidth < width() && newHeight < height()) ||
           (newWidth >= width() && newHeight >= height()));

    if (newWidth < width() && newHeight < height())
    {
        // Downsample using box filter
        if ( depth() == 3 )
        {
            C2PassTiled<CBoxFilter, CDataRGB_UBYTE> filter;

            cBits = (unsigned char*) filter.AllocAndScale( 
                (unsigned char (*)[3]) bits(), 
                width(), height(), newWidth, newHeight );
        }
        else if ( depth() == 4 )
        {
            assert( format() == CF_RGBA );	// possible confusion with IA16
            C2PassTiled<CBoxFilter, CDataRGBA_UBYTE> filter;

            cBits = (unsigned char*) filter.AllocAndScale( 
                (unsigned char (*)[4]) bits(), 
                width(), height(), newWidth, newHeight );
        }
        else if ( depth() == 2 )	// I16
        {
            assert( format() == CF_I16 );	// possible confusion with IA8
            C2PassTiled<CBoxFilter, CData_USHORT> filter;

            cBits = (unsigned char*) filter.AllocAndScale( 
                (unsigned short *) bits(), 
                width(), height(), newWidth, newHeight );
        }
        else if ( depth() == 1 )	// I8
        {
            C2PassTiled<CBoxFilter, CData_UBYTE> filter;

            cBits = (unsigned char*) filter.AllocAndScale( 
                (unsigned char *) bits(), 
                width(), height(), newWidth, newHeight );
        }
        else
        {
            assert( false && "Bit depth unsupported" );
        }
    }
    else
    {
        // Upsample using bilinear filter (note: CBoxFilter is chunky but preserves colors)
        if ( depth() == 3 )
        {
            C2PassScale<CBilinearFilter, CDataRGB_UBYTE> filter;

            cBits = (unsigned char*) filter.AllocAndScale( 
                (unsigned char (*)[3]) bits(), 
                width(), height(), newWidth, newHeight );
        }
        else if ( depth() == 4 )
        {
            assert( format() == CF_RGBA );	// possible confusion with IA16
            C2PassScale<CBilinearFilter, CDataRGBA_UBYTE> filter;

            cBits = (unsigned char*) filter.AllocAndScale( 
                (unsigned char (*)[4]) bits(), 
                width(), height(), newWidth, newHeight );
        }
        else if ( depth() == 2 )	// I16
        {
            assert( format() == CF_I16 );	// possible confusion with IA8
            C2PassScale<CBilinearFilter, CData_USHORT> filter;

            cBits = (unsigned char*) filter.AllocAndScale( 
                (unsigned short *) bits(), 
                width(), height(), newWidth, newHeight );
        }
        else if ( depth() == 1 )	// I8
        {
            C2PassScale<CBilinearFilter, CData_UBYTE> filter;

            cBits = (unsigned char*) filter.AllocAndScale( 
                (unsigned char *) bits(), 
                width(), height(), newWidth, newHeight );
        }
        else
        {
            assert( false && "Bit depth unsupported" );
        }

    }

    // Delete the old storage
    delete_array(m_pBits);

    // Assign the newly rescaled bits
    m_pBits = cBits;
    m_width = newWidth;
    m_height = newHeight;
}

//-----------------------------------------------------------------------------
// scale
//
// scales image to fit in newWidth and newHeight without loosing
// the aspect ratio.  Uses given color for any necessary "letterboxing"
//-----------------------------------------------------------------------------
void Bitmap::scale( int newWidth, int newHeight,
                    const RGBAPixel& color,
                    bool isAspectPreserving )
{
    assert(format() == CF_RGB || format() == CF_RGBA || 
           format() == CF_I8 || format() == CF_IA8);

    // form rect with 0 for left and top and
    // width and height for right and bottom
    long finalTop = 0;
    long finalBottom = newHeight;
    long finalLeft = 0;
    long finalRight = newWidth;

    // if we want to preserve aspect ratio
    if ( isAspectPreserving )
    {
        const double aspect = (double)m_width/m_height;

        // do correction for width/height ratio
        if (m_width > m_height)
        {
            long h1 = newHeight;

            finalBottom  = (long) (finalTop + (finalRight - finalLeft) / aspect);

            long h2 = (h1 - (finalBottom - finalTop)) / 2;

            finalTop += h2;
            finalBottom += h2;
        }
        else
        {
            long w1 = newWidth;

            finalRight  = (long) (finalLeft + (finalBottom - finalTop) * aspect);

            long w2 = (w1 - (finalRight - finalLeft)) / 2;

            finalLeft += w2;
            finalRight += w2;
        }

        // scale the bitmap to the aspect corrected width and height
        int correctedWidth = finalRight - finalLeft;
        int correctedHeight = finalBottom - finalTop;
        scale(correctedWidth, correctedHeight);
    }

    unsigned char *newBits = new unsigned char[newWidth * newHeight * depth()];

    if(format() == CF_RGB || format() == CF_RGBA)
    {
    bool alpha = (channels() == 4);
    int depth = this->depth();	// in bytes
    // copy into newBits, center and pad outside with given color
    for ( int i = 0; i < newHeight; ++i )
    {
        for ( int j = 0; j < newWidth; ++j )
        {
            const int destOffset = ( i * newWidth * depth ) + ( j * depth );

            // if we are inside, copy the bits over
            if ( ( j >= finalLeft ) && ( j < finalRight ) &&
                 ( i >= finalTop ) && ( i < finalBottom ) )
            {
                const int oi = i - finalTop;
                const int oj = j - finalLeft;

                const int srcOffset = ( oi * width() * depth ) + ( oj * depth );

                // Get RGB
                newBits[destOffset + 0] = m_pBits[srcOffset + 0];
                newBits[destOffset + 1] = m_pBits[srcOffset + 1];
                newBits[destOffset + 2] = m_pBits[srcOffset + 2];

                // if there is an alpha channel
                if ( alpha )
                {
                    newBits[destOffset + 3] = m_pBits[srcOffset + 3];
                }
            }
            // else we are outside the scaled down bitmap so
            // set the bits to the background color
            else
            {
                // set to the supplied background color
                newBits[destOffset + 0] = color[0];
                newBits[destOffset + 1] = color[1];
                newBits[destOffset + 2] = color[2];

                // if there is an alpha channel
                if ( alpha )
                {
                    newBits[destOffset + 3] = color[3];
                }
            }
        }
    }
    } 	// format = RGB||RGBA
    if(format() == CF_I8 || format() == CF_IA8)
    {
    bool alpha = (channels() == 2);
    int depth = this->depth();	// in bytes
    // copy into newBits, center and pad outside with given color
    for ( int i = 0; i < newHeight; ++i )
    {
        for ( int j = 0; j < newWidth; ++j )
        {
            const int destOffset = ( i * newWidth * depth ) + ( j * depth );

            // if we are inside, copy the bits over
            if ( ( j >= finalLeft ) && ( j < finalRight ) &&
                 ( i >= finalTop ) && ( i < finalBottom ) )
            {
                const int oi = i - finalTop;
                const int oj = j - finalLeft;

                const int srcOffset = ( oi * width() * depth ) + ( oj * depth );

                newBits[destOffset] = m_pBits[srcOffset];

                if (alpha)
                    newBits[destOffset + 1] = m_pBits[srcOffset + 1];
            }
            // else we are outside the scaled down bitmap so
            // set the bits to the background color
            else
            {
                // set to the supplied background color
                newBits[destOffset] = (RED_COEFF * color[0]) +
                                      (GREEN_COEFF * color[1]) +
                                      (BLUE_COEFF * color[2]);
                if (alpha)
                    newBits[destOffset + 1] = color[3];
            }
        }
    }
    }

    delete_array( m_pBits );
    m_pBits = newBits;
    m_width = newWidth;
    m_height = newHeight;
}

//-----------------------------------------------------------------------------
// resize
//
// Scale the image to the given size. If a target/dest image is supplied
// does not alter the src image.

/*
GLU doc: 

"When shrinking an image, gluScaleImage uses a box filter to sample the source image and create pixels for the destination image.  When magnifying an image, the pixels from the source image are linearly interpolated to create the destination image.
*/

//-----------------------------------------------------------------------------
void Bitmap::resize(int widthIn, int heightIn, Bitmap *target )
{
    // We don't do it in place so we can reclaim memory
    // (Maybe the original image was gigantic)
    GLubyte* pTempImage = new GLubyte[widthIn * heightIn * (depthInBits() >> 3)];

    assert(pTempImage);

	if (m_pBits) // if no existing bits, don't need to scale
	{
#if 0
		// This does work (glPushAttrib/glPopAttrib does not), but is
		// somewhat pointless since every place in FF that relies on
		// the PACK/UNPACK settings goes ahead and sets them directly.
		//
		glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);	// default is 4
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);	// default is 0
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);	// default is 0
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);// default is 0

		glPixelStorei(GL_PACK_ALIGNMENT,   1);	// default is 4
		glPixelStorei(GL_PACK_ROW_LENGTH, 0);	// default is 0
		glPixelStorei(GL_PACK_SKIP_ROWS, 0);	// default is 0
		glPixelStorei(GL_PACK_SKIP_PIXELS, 0);	// default is 0

		GLenum dtype = getGLDataType();	// GL_UNSIGNED_BYTE vs GL_UNSIGNED_SHORT
		gluScaleImage(getGLFormat(), m_width, m_height, dtype, m_pBits,
						widthIn, heightIn, dtype, pTempImage);

		glPopClientAttrib();
#else
		FFScaleImage(format(), m_width, m_height, m_pBits,
						widthIn, heightIn, pTempImage);
#endif
	}

	if (target)
	{
		target->deinit();
		// loadBmp will copy the image data; we just want to assign the pointer.
		target->m_GlType = m_GlType;
		target->m_width = widthIn;
		target->m_height = heightIn;
		target->m_colorFormat = m_colorFormat;
		target->m_pBits = pTempImage;
	}
	else
	{
        delete_array(m_pBits);
        m_pBits = pTempImage;

        m_width = widthIn;
        m_height = heightIn;
	}
}

void Bitmap::halve( Bitmap *target )
{
	int depthInBytes = depth();
	int newWidth = width() / 2;
	int newHeight = height() / 2;

	if( width() == 1 && height() == 1 )	{
		// Very uncommon special case - source image is 1x1
		if (target) *target = *this;
		return;
	}

	// Handle 1xN, Nx1 special cases.
	bool transpose = false;
	bool oneRow = false;
	if( height() == 1 ) {
		oneRow = true;
		newHeight = 1;	// instead of 0
	}
	else if( width() == 1 ) {
		// Memory layout of 1 row vs 1 column will be the same
		oneRow = true;
		transpose = true;
		newWidth = newHeight;
		newHeight = 1;
	}

	unsigned char* pTempImage = new unsigned char[newWidth * newHeight * depthInBytes];

	if( format() == CF_I8 )
	{
		for ( int j = 0; j < newHeight; ++j )
		{
			unsigned char* pRow0 = m_pBits + (j*2) * width();
			unsigned char* pRow1 = m_pBits + ((j*2) + 1) * width();
			if (oneRow) pRow1 = pRow0;
			unsigned char* pDstRow = pTempImage + j * newWidth;
			for ( int i = 0; i < newWidth; ++i )
			{
				int sum = *pRow0 + *(pRow0+1) + *pRow1 + *(pRow1+1);
				*pDstRow = sum / 4;	// what about rounding?
				if (sum%4==3) *pDstRow += 1;	// round up

				pRow0 += 2;
				pRow1 += 2;
				pDstRow += 1;
			}
		}
	}
	else if( channelDepth() == 1 ) // RGB, RGBA, IA8. I8 handled above
	{
		int chan = channels();
		for ( int j = 0; j < newHeight; ++j )
		{
			unsigned char* pRow0 = m_pBits + ((j*2) * width() * chan);
			unsigned char* pRow1 = m_pBits + (((j*2) + 1) * width() * chan);
			if (oneRow) pRow1 = pRow0;
			unsigned char* pDstRow = pTempImage + (j * newWidth * chan);
			for ( int i = 0; i < newWidth; ++i )
			{
				for ( int c = 0; c < chan; ++c )
				{
					int sum = *pRow0 + *(pRow0+chan) + *pRow1 + *(pRow1+chan);
					*pDstRow = sum / 4;	// what about rounding?
					if (sum%4==3) *pDstRow += 1;	// round up

					pRow0 += 1;	// visit each channel of src pixel
					pRow1 += 1;
					pDstRow += 1;
				}
				pRow0 += chan;	// skip next src pixel
				pRow1 += chan;
			}
		}
	}
	else if( format() == CF_I16 )
	{
		unsigned short *pBits16 = (unsigned short *)m_pBits;
		unsigned short *pTemp16 = (unsigned short *)pTempImage;
		for ( int j = 0; j < newHeight; ++j )
		{
			unsigned short* pRow0 = pBits16 + (j*2) * width();
			unsigned short* pRow1 = pBits16 + ((j*2) + 1) * width();
			if (oneRow) pRow1 = pRow0;
			unsigned short* pDstRow = pTemp16 + (j * newWidth);
			for ( int i = 0; i < newWidth; ++i )
			{
				int sum = *pRow0 + *(pRow0+1) + *pRow1 + *(pRow1+1);
				*pDstRow = sum / 4;	// what about rounding?
				if (sum%4==3) *pDstRow += 1;	// round up

				pRow0 += 2;
				pRow1 += 2;
				pDstRow += 1;
			}
		}
	}
	else if( channelDepth() == 2 ) // IA16 (now), RGB16, RGBA16 (future)
	{
		ushort *pBits16 = (ushort *)m_pBits;
		ushort *pTemp16 = (ushort *)pTempImage;
		int chan = channels();
		for ( int j = 0; j < newHeight; ++j )
		{
			ushort* pRow0 = pBits16 + ((j*2) * width() * chan);
			ushort* pRow1 = pBits16 + (((j*2) + 1) * width() * chan);
			if (oneRow) pRow1 = pRow0;
			ushort* pDstRow = pTemp16 + (j * newWidth * chan);
			for ( int i = 0; i < newWidth; ++i )
			{
				for ( int c = 0; c < chan; ++c )
				{
					int sum = *pRow0 + *(pRow0+chan) + *pRow1 + *(pRow1+chan);
					*pDstRow = sum / 4;	// what about rounding?
					if (sum%4==3) *pDstRow += 1;	// round up

					pRow0 += 1;	// visit each channel of src pixel
					pRow1 += 1;
					pDstRow += 1;
				}
				pRow0 += chan;	// skip next src pixel
				pRow1 += chan;
			}
		}
	}
	else assert( !"not implemented" );

	if( transpose ) {
		std::swap( newWidth, newHeight );
	}

	if (target)
	{
		target->deinit();
		// loadBmp will copy the image data; we just want to assign the pointer.
		target->m_GlType = m_GlType;
		target->m_width = newWidth;
		target->m_height = newHeight;
		target->m_colorFormat = m_colorFormat;
		target->m_pBits = pTempImage;
	}
	else
	{
		delete_array(m_pBits);
		m_pBits = pTempImage;

		m_width = newWidth;
		m_height = newHeight;
	}
}

//-----------------------------------------------------------------------------
// resizeCanvas
//
// Can be used to add or remove from the image dimensions. Specify
// the number of pixels to add or remove in each direction
//-----------------------------------------------------------------------------
void Bitmap::resizeCanvas(int deltaLeft, int deltaRight,
                          int deltaTop, int deltaBottom,
                          unsigned long fillValue)
{
    // Determine the dimensions of the new image
    int newWidth = m_width - (deltaLeft + deltaRight);
    int newHeight = m_height - (deltaTop + deltaBottom);
    int currHeight = m_height;
    int currWidth = m_width;

    // Make a copy of myself
    Bitmap tmpBitmap;
    bool bNoError = tmpBitmap.loadBmp(width(), height(), bits(), format());
    assert(bNoError);

    // Resize the canvas
    bNoError = init(newWidth, newHeight, format());
    assert(bNoError);

    // Fill the canvas with a solid color before blitting
    fill(fillValue);

    // need to figure which locations to copy/from to in the src and dst
    // images.  This will depend on whether the image is shrinking or
    // growing at a given edge.
    int srcx, srcy, dstx, dsty;
    if (deltaLeft > 0)
    {
    // image is getting smaller on left edge so blit from
    // inside the edge of the src to the edge of the dst
        srcx = deltaLeft;
        dstx = 0;
    }
    else
    {
    // image is getting bigger on left edge so blit from
    // edge of to src to inside the dst.
        srcx = 0;
        dstx = -deltaLeft;
    }

    // do same as above for top edge
    if (deltaTop > 0)
    {
        srcy = deltaTop;
        dsty = 0;
    }
    else
    {
        srcy = 0;
        dsty = -deltaTop;
    }

    // width and height to blit are always smaller
    // of new and existing.  When image is shrinking
    // in a given dimension it will be new and when it is
    // growing it will be old.
    int w = min(newWidth, currWidth);
    int h = min(newHeight, currHeight);

    // Now blit the copy into the new resized image
	// WARNING srcy is inverted in this call, srcy=0 is the bottom, not the top.
	// So a valid srcy range is (0 to pSrc->height() - height)
    blitSameColorFormat(this, &tmpBitmap, srcx, srcy, w, h, dstx, dsty);
}

//-----------------------------------------------------------------------------
// new corner coordinates
bool Bitmap::copySection(int x1, int y1, int x2, int y2, Bitmap &bm_cropped)
{
	int currWidth = width();
	int currHeight = height(); 

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;

	if (x2 > currWidth)
		x2 = currWidth;
	if (y2 > currHeight)
		y2 = currHeight;

	int newWidth = x2 - x1;
    int newHeight = y2 - y1;

	if (newWidth > currWidth)
		newWidth = currWidth;
	if (newHeight > currHeight)
		newHeight = currHeight;

	bm_cropped.clear();
	bool bNoError = bm_cropped.init(newWidth, newHeight, format());
	assert(bNoError);
	bm_cropped.fill(0);
	if (!bNoError)
		return bNoError;

	// WARNING srcy is inverted in this call, srcy=0 is the bottom, not the top.
	// So a valid srcy range is (0 to pSrc->height() - height)
	int srcy = currHeight - y2;
	if (srcy < 0)
		srcy = 0;
	else if (srcy > currHeight)
		srcy = currHeight;
	blitSameColorFormat(&bm_cropped, this, x1, srcy, newWidth, newHeight, 0, 0);

	return bNoError;
}

//-----------------------------------------------------------------------------
// fill
//
// Fills the contents of the image with the value specified. Depending
// on the bit depth, only some of the value will actually be used. The
// fill value format follows the ColorFormat spec.
//-----------------------------------------------------------------------------
void Bitmap::fill(unsigned long fillValue)
{
    uchar *pBits = m_pBits;

    int nNumPixels = m_width * m_height;
    int nDepth = depth();

    for (int i = 0; i < nNumPixels; i++)
    {
        for (int j = 0; j < nDepth; j++)
        {
            *pBits++ = fillValue & (0xFF << j);
        }
    }
}

//-----------------------------------------------------------------------------
// fill
//
//-----------------------------------------------------------------------------
void Bitmap::fill(const RGBAPixel &fillColor)
{
    if (format() == CF_RGBA)
    {
        int nNumPixels = m_width * m_height;

        RGBAColor *pData = (RGBAColor *)m_pBits;

        for (int i = 0; i < nNumPixels; i++)
        {
            pData->r = fillColor.r();
            pData->g = fillColor.g();
            pData->b = fillColor.b();
            pData->a = fillColor.a();
            pData++;
        }
    }
    else
    {
        int nNumPixels = m_width * m_height;

        RGBColor *pData = (RGBColor *)m_pBits;

        for (int i = 0; i < nNumPixels; i++)
        {
            pData->r = fillColor.r();
            pData->g = fillColor.g();
            pData->b = fillColor.b();
            pData++;
        }
    }
}

//-----------------------------------------------------------------------------
// saveToFile
//
//-----------------------------------------------------------------------------
bool Bitmap::saveToFile( const TCHAR *pszFilename ) const
{
    bool success = false;

    StringT fileName( pszFilename );
    StringT fileExt = fileName.getFileExtension();
    if (fileExt.comparei(_T(".png")))
    {
        assert( m_pBits && (width() > 0) && (height() > 0) );
        if( !(m_pBits && (width() > 0) && (height() > 0)) )
            return false;
        PngFile png;
        int ret = -1;
        if( format() == CF_RGB ) {
            png.setRGBImage( m_pBits, width(), height() );
            ret = png.write( fileName );
        }
        else if( format() == CF_RGBA ) {
            png.setRGBAImage( m_pBits, width(), height() );
            ret = png.write( fileName );
        }
        else if( format() == CF_I8 ) {
            png.setGrayImage( m_pBits, width(), height() );
            ret = png.write( fileName );
        }
        else if( format() == CF_I16 ) {
            png.setGrayImage( (unsigned short *)m_pBits, width(), height() );
            ret = png.write( fileName );
        }
        else if( format() == CF_IA8 ) {
            auto *imgPtr = png.allocImage( channels(), PngFile::CD_8, width(), height() );
            memcpy( imgPtr, bits(), getSizeBytes() );
            ret = png.write( fileName );
        }
        else if( format() == CF_IA16 ) {
            auto *imgPtr = png.allocImage( channels(), PngFile::CD_16, width(), height() );
            memcpy( imgPtr, bits(), getSizeBytes() );
            ret = png.write( fileName );
        }

        else return false;

        return ret == 0;
    }

    TCHAR *TmpPath = _tgetenv(_T("TEMP")), *TmpFile;
	if (TmpPath)
		TmpFile = _ttempnam(TmpPath, NULL);
	else
		TmpFile = _ttmpnam(NULL);

    // if opening the file for binary writing works
    if ( FILE *temp = _tfopen(TmpFile, _T("wb")) )
    {
        // write bitmap to disk
        success = writeToFile( temp );

        // close the file
        fclose( temp );

        // rename the temp file to the input filename
        if (success)
        {
            _tremove(pszFilename);
            success = _trename(TmpFile, pszFilename) == 0;
        }
    }

	// Need to free the buffer returned by _tempnam()
	if (TmpPath)
		free(TmpFile);
    return success;
}

//-----------------------------------------------------------------------------
// writeToFile
//
//-----------------------------------------------------------------------------
bool Bitmap::writeToFile( FILE *file ) const
{
    // make sure each scan line is aligned to a DWord (32-bit) boundary
    // the "(x + 31) & ~31" rounds to the next DWord boundary
    // note: each pixel is ALWAYS 24 bits wide because we always want
    // to output 24bpp bitmaps so they don't die in sad bitmap programs
    // like Microsoft Imaging and other things that don't like 32bpp

    // calculate the new buffer's scanline width
    long lScanLineWidth = ( ( ( width() * 24 ) + 31 ) & ~31 ) / 8;

    // calculate the total size of the data in bytes
    long lNumDataBytes = lScanLineWidth * height();

    // allocate the new buffer
    Array<unsigned char> newBits(lNumDataBytes);

    if (format() == CF_I8)	// LIMBO/TODO - add support for CF_IA8
    {
        int w = width();
        // depth is fixed == 1
        for ( int j = 0; j < height(); ++j )
        {
            for ( int i = 0; i < w; ++i )
            {
                int loc;

                // Set the B channel with the gray value
                loc = (j * lScanLineWidth) + (i * 3);
                newBits[ loc ] = bits()[ (j * w) + i ];

                // Set the G channel with the gray value
                loc += 1;
                newBits[ loc ] = bits()[ (j * w) + i ];

                // Set the R channel with the gray value
                loc += 1;
                newBits[ loc ] = bits()[ (j * w) + i ];
            }
        }
    }
    // iterate through bits and convert from RGB to BGR
    else if (format() == CF_RGB || format() == CF_RGBA) 
    {
        int w = width();
        int d = depth();	// == 3 or 4
        for ( int j = 0; j < height(); ++j )
        {
            for ( int i = 0; i < w; ++i )
            {
                int loc;

                // Copy B channel
                loc = (j * lScanLineWidth) + (i * 3) + 0;
                newBits[ loc ] = bits()[ (j * w * d) + (i * d) + 2 ];

                // Copy G channel
                loc = (j * lScanLineWidth) + (i * 3) + 1;
                newBits[ loc ] = bits()[ (j * w * d) + (i * d) + 1 ];

                // Copy R channel
                loc = (j * lScanLineWidth) + (i * 3) + 2;
                newBits[ loc ] = bits()[ (j * w * d) + (i * d) + 0 ];
            }
        }
    }
    else	// CF_IA8, CF_I16, CF_IA16
    {
        assert( !"BMP files do not support this color format" );
        unsigned char *p = newBits;
        memset(p, 0, lNumDataBytes);
    }

    // prepare the headers to be written to disk
    BITMAPFILEHEADER hdr;
    BITMAPINFOHEADER bi;

    hdr.bfType              = ( ( WORD ) ('M' << 8) | 'B');    // is always "BM"
    hdr.bfSize              = ( ( DWORD ) sizeof( hdr ) + sizeof( bi ) +
                                lNumDataBytes );
    hdr.bfReserved1         = 0;
    hdr.bfReserved2         = 0;
    hdr.bfOffBits           = ( ( DWORD ) sizeof( hdr ) + sizeof( bi ) );

    bi.biSize               = sizeof( bi );
    bi.biWidth              = width();
    bi.biHeight             = height();
    bi.biPlanes             = 1;
    bi.biBitCount           = 24;       // always 24bpp!!!
    bi.biCompression        = BI_RGB;
    bi.biSizeImage          = 0;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = 0;
    bi.biClrImportant       = 0;

    size_t totalBytesWritten = 0;
    size_t bytesWritten;

    // Write the file header 
    bytesWritten = fwrite( &hdr, sizeof( char ), sizeof( hdr ), file );
    if (bytesWritten != sizeof(hdr))
    {
        return false;
    }
    totalBytesWritten += bytesWritten;

    // Write the DIB header 
    bytesWritten = fwrite( &bi, sizeof( char ), sizeof( bi ), file );
    if (bytesWritten != sizeof(bi))
    {
        return false;
    }
    totalBytesWritten += bytesWritten;

    // Write the bits
    bytesWritten = fwrite( newBits, sizeof( unsigned char ), 
        lNumDataBytes , file );
    if (bytesWritten != (lNumDataBytes * sizeof( unsigned char )))
    {
        return false;
    }
    totalBytesWritten += bytesWritten;

    assert(totalBytesWritten == hdr.bfSize);

    return true;
}

//-----------------------------------------------------------------------------
// mirror
//
// Mirror the image about it's center either left to right or top to bottom.
//-----------------------------------------------------------------------------
void Bitmap::mirror(bool leftToRightIn)
{
    int nBytesPerLine = m_width * 3;

    if (leftToRightIn == true)
    {
        // Copy bytes left to right.
        BYTE tmpBytes[3];
        int halfWidth = m_width / 2;
        for (int i = 0; i < m_height; i++)
        {
            // Set the start and end of this line.
            BYTE *pStartBytes = m_pBits + i * nBytesPerLine;
            BYTE *pEndBytes = m_pBits + (i + 1) * nBytesPerLine - 3;

            for (int j = 0; j < halfWidth; j++)
            {
                ::memcpy(tmpBytes, pStartBytes, 3);
                ::memcpy(pStartBytes, pEndBytes, 3);
                ::memcpy(pEndBytes, tmpBytes, 3);

                pStartBytes += 3;
                pEndBytes -= 3;
            }
        }
    }
    else
    {
        // Copy bytes top to bottom.
        BYTE *pStartBytes = m_pBits;
        BYTE *pTmpBytes = new BYTE [nBytesPerLine];
        BYTE *pEndBytes = m_pBits + m_height * nBytesPerLine - nBytesPerLine;
        int halfHeight = m_height / 2;
        for (int j = 0; j < halfHeight; j++)
        {
            ::memcpy(pTmpBytes, pStartBytes, nBytesPerLine);
            ::memcpy(pStartBytes, pEndBytes, nBytesPerLine);
            ::memcpy(pEndBytes, pTmpBytes, nBytesPerLine);

            pStartBytes += nBytesPerLine;
            pEndBytes -= nBytesPerLine;
        }
        delete [] pTmpBytes;
    }
}  // Bitmap::mirror

//-----------------------------------------------------------------------------
// loadImageFromFile
//
// We use OleLoadPicture to handle varous formats (BMP, JPG, GIF, ???)
//-----------------------------------------------------------------------------
bool Bitmap::loadImageFromFile(LPCTSTR szFile, ColorFormat fmt)
{
    // Open file
    HANDLE hFile = CreateFile( szFile, GENERIC_READ, 0, NULL, 
                               OPEN_EXISTING, 0, NULL );

    // Create IPicture from image file
    LPPICTURE pPicture;

    // If the file or path requested here does not exist, this handle will be null.
    // (This can happen if one user has a local BMP file that he uses for 
    //  WrapTexture, saves the clay file, and then transfers the clauy file to 
    //  a second user.  If that second user does not have access to the same
    //  BMP file, then the above call will fail.)
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        USES_CONVERSION;
        ::OleLoadPicturePath(const_cast<LPOLESTR>(T2COLE(szFile)),
							  NULL,
							  0,
							  0,
							  IID_IPicture,
							  (LPVOID *)(&pPicture));

        if (!pPicture)
            return false;
    }
    else
    {
        // Get file size
        DWORD dwFileSize = GetFileSize(hFile, NULL);
        _ASSERTE(-1 != dwFileSize);

        // Alloc memory based on file size
        LPVOID pvData = NULL;
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwFileSize);
        _ASSERTE(NULL != hGlobal);

        pvData = GlobalLock(hGlobal);
        if (pvData)
        {
            // Read file and store in global memory
            DWORD dwBytesRead = 0;
            BOOL bRead = ReadFile(hFile, pvData, dwFileSize, &dwBytesRead, NULL);
            _ASSERTE(FALSE != bRead);
        }
        GlobalUnlock(hGlobal);
        CloseHandle(hFile);

        // Create IStream* from global memory
        LPSTREAM pstm = NULL;
        HRESULT hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pstm);
        _ASSERTE(SUCCEEDED(hr) && pstm);
		if (!SUCCEEDED(hr) && pstm)
			return false;

        //// Create IPicture from image file
        hr = ::OleLoadPicture(pstm, dwFileSize, FALSE, IID_IPicture, (LPVOID*)&pPicture);

		if (hr != 0 )
		{
			 if (hr == CTL_E_INVALIDPICTURE)
				 assert(!"CTL_E_INVALIDPICTURE");
			 else if (hr == E_NOINTERFACE)
				assert(!"E_NOINTERFACE");
			 else if (hr == E_POINTER)
				assert(!"E_POINTER");
		}

        pstm->Release();

        // Bail if load didn't work
        if (!pPicture)
            return false;
    }

    // Get the bitmap handle
    HBITMAP hBitmap;
    if (S_OK != pPicture->get_Handle((OLE_HANDLE FAR *) &hBitmap))
    {
        // Couldn't get bitmap handle from picture
        ASSERT(FALSE);
        pPicture->Release();
        return false;
    }

    loadBmp(hBitmap, fmt);

    pPicture->Release();

    return true;
}

void Bitmap::sharpen(int level, RGBAPixel *pIgnoreColor)
{
    assert( m_pBits );
    if(!m_pBits || (level <= 0))
        return;

	if (format() != CF_RGB)
	{
		assert(!"sharpen not supported for this pixel format");
		return;
	}

	unsigned char* pBits = m_pBits;
	unsigned char* pOldBits = new unsigned char[ m_width * m_height * (depthInBits() >> 3) ];
	memcpy( pOldBits, pBits, m_height * bytesPerRow() );

	double weights[9];
	// ordered by matrix rows (0,0) at top left

	// https://en.wikipedia.org/wiki/Kernel_(image_processing)
	// https://azzlsoft.com/2011/02/21/phone-vision-13-sharpening-filters/
	if (level == 1)
	{
		weights[0] = 0;
		weights[1] = -1;
		weights[2] = 0;

		weights[3] = -1;
		weights[4] = 5;
		weights[5] = -1;

		weights[6] = 0;
		weights[7] = -1;
		weights[8] = 0;
	}
	else // use corner pixels too
	{
		weights[0] = -1;
		weights[1] = -1;
		weights[2] = -1;

		weights[3] = -1;
		weights[4] = 9;
		weights[5] = -1;

		weights[6] = -1;
		weights[7] = -1;
		weights[8] = -1;
	}

	RGBAPixel p[9];
	double r = 0;
	double g = 0;
	double b = 0;

	bool bUseRGB = false;
	bool bUseHSL = true;
	double val = 0;
	double H,S,V;

	for (int x = 1; x < m_width-1; x++)
		for (int y = 1; y < m_height-1; y++)
    {			
		RGBColor* pPixel = (RGBColor*) getPixel(x, y);

		// ignore standard "transparent" color
		if (pIgnoreColor)
		{
			if (pPixel->r == pIgnoreColor->r() &&
				pPixel->g == pIgnoreColor->g() &&
				pPixel->b == pIgnoreColor->b())
				continue;
		}
	
		m_pBits = pOldBits; // must always look at unmodified pixels
		// ordered by matrix rows (0,0) at top left
		getRGBAPixel(x - 1, y - 1, p[0]);
		getRGBAPixel(x + 0, y - 1, p[1]);
		getRGBAPixel(x + 1, y - 1, p[2]);

		getRGBAPixel(x - 1, y + 0, p[3]);
		getRGBAPixel(x + 0, y + 0, p[4]); // center
		getRGBAPixel(x + 1, y + 0, p[5]);

		getRGBAPixel(x - 1, y + 1, p[6]);
		getRGBAPixel(x + 0, y + 1, p[7]);
		getRGBAPixel(x + 1, y + 1, p[8]);
		m_pBits = pBits; // restore to modified pixels for getPixel()

		
		if (!bUseRGB)
		{
			val = 0;
			for (int i = 0; i < 9; i++)
			{
				COLORREF rgb = RGB(p[i].r(), p[i].g(), p[i].b());			
				if (bUseHSL)
					CDrawingManager::RGBtoHSL(rgb, &H, &S, &V);
				else
					CDrawingManager::RGBtoHSV(rgb, &H, &S, &V);

				val += V * weights[i];
			}

			// LIMBO what does it mean when it is out of range
			val = clamp(val, 0.0F, 1.0F);

			COLORREF rgb = RGB(p[4].r(), p[4].g(), p[4].b()); // center unmodified pixel

			if (bUseHSL)
			{
				CDrawingManager::RGBtoHSL(rgb, &H, &S, &V);
				H *= 360.0F;// convert range (0 - 1) to (0 - 360)
				rgb = CDrawingManager::HLStoRGB_TWO(H, val, S);
			}
			else
			{
				CDrawingManager::RGBtoHSV(rgb, &H, &S, &V);
				rgb = CDrawingManager::HSVtoRGB(H, S, val);
			}

			pPixel->r = (unsigned char)(GetRValue(rgb));
			pPixel->g = (unsigned char)(GetGValue(rgb));
			pPixel->b = (unsigned char)(GetBValue(rgb));
		}
		else
		{
			r = 0;
			g = 0;
			b = 0;
			for (int i = 0; i < 9; i++)
			{
				r += (double)p[i].r() * weights[i];
				g += (double)p[i].g() * weights[i];
				b += (double)p[i].b() * weights[i];
			}

			// LIMBO what does it mean when it is out of range
			r = clamp(r, 0, 255);
			g = clamp(g, 0, 255);
			b = clamp(b, 0, 255);
			
			pPixel->r = (unsigned char)((int)r);
			pPixel->g = (unsigned char)((int)g);
			pPixel->b = (unsigned char)((int)b);
		}
    }

	delete_array(pOldBits);
}

void Bitmap::blur(int level)
{
    IppiPoint anchor;
    IppiSize  maskSize;
    IppStatus s;
    IppiSize  srcsz,dstsz;

    assert( m_pBits );
    if(!m_pBits || (level <= 0))
        return;

	if (level > 74)
        level = 74;

	if (format() == CF_RGB)
	{
		for (int x = 1; x < m_width-1; x++)
			for (int y = 1; y < m_height-1; y++)
        {
			RGBColor* pPixel = (RGBColor*) getPixel(x, y);

			RGBAPixel p[9];

			// ordered by matrix rows (0,0) at top left
			getRGBAPixel(x - 1, y - 1, p[0]);
			getRGBAPixel(x + 0, y - 1, p[1]);
			getRGBAPixel(x + 1, y - 1, p[2]);

			getRGBAPixel(x - 1, y + 0, p[3]);
			getRGBAPixel(x + 0, y + 0, p[4]); // center
			getRGBAPixel(x + 1, y + 0, p[5]);

			getRGBAPixel(x - 1, y + 1, p[6]);
			getRGBAPixel(x + 0, y + 1, p[7]);
			getRGBAPixel(x + 1, y + 1, p[8]);

			int r = 0;
			int g = 0;
			int b = 0;
			for (int i = 0; i < 9; i++)
			{
				r += (int)p[i].r();
				g += (int)p[i].g();
				b += (int)p[i].b();
			}
			
			// ignores level
			pPixel->r = (unsigned char)((double)r / 9.0f);
			pPixel->g = (unsigned char)((double)g / 9.0f);
			pPixel->b = (unsigned char)((double)b / 9.0f);
        }
	}
	else if (format() == CF_I8)
	{
		// this blur only works for 8 bit...

		int boxsize = 1+level*2; //3x3 is minimum & boxsize must be odd numbered

 		anchor.x = boxsize/2.0f;
		anchor.y = boxsize/2.0f;
		maskSize.height = boxsize;
		maskSize.width  = boxsize;

		srcsz.width     = width();
		srcsz.height    = height();
		dstsz.width     = width()+2*boxpad;
		dstsz.width     += dstsz.width%align ? align-dstsz.width%align : 0;
		dstsz.height    = height()+2*boxpad;	

		// setup temp buffer
		int rowbytes = width() + 2*boxpad;
		rowbytes += rowbytes%align ? align-rowbytes%align : 0; // pad to align bytes for IPP
		Ipp8u *ptemp = (Ipp8u *)new unsigned char[ rowbytes * (height() + 2*boxpad) ];

		if(ptemp)
		do
		{
			Ipp8u *psrc  = (Ipp8u*)m_pBits;
			s = ippiCopyConstBorder_8u_C1R(psrc,width(),srcsz,ptemp,dstsz.width,dstsz,boxsize,boxsize,0);
			if(s)
			   break;

			s=(IppStatus)0;

			psrc = (Ipp8u *)ptemp + dstsz.width*boxsize + boxsize;
			s = ippiFilterBox_8u_C1IR( psrc, dstsz.width, srcsz, maskSize, anchor);
			if(s)
				break;

			s=(IppStatus)0;
			s = ippiCopy_8u_C1R( psrc, dstsz.width, (Ipp8u*)m_pBits, srcsz.width, srcsz);
			if(s)
				break;

			s=(IppStatus)0;
		}while(0);

		delete [] ptemp;
		assert(!s); // ipp error 
	}
	else 
	{
		assert(!"blur not supported for this pixel format");
	}
}

long lerpColor(int a, int b, double t)
{
	return (long)((1.0 - t)*(double)a + t*(double)b);
}

void Bitmap::blurBlend(int r_bcolor, int g_bcolor, int b_bcolor, int nPasses, bool bBlur, bool bRadialBlend)
{
	assert(m_pBits);
	if (!m_pBits || (nPasses <= 0))
		return;

	if (format() != CF_RGB)
	{
		assert(!"only CF_RGB is supported");
		return;
	}

	RGBColor bcolor;
	bcolor.r = r_bcolor;
	bcolor.g = g_bcolor;
	bcolor.b = b_bcolor;
	RGBColor *pixel = NULL;
	RGBColor p[9];

	Point2Dd center(m_width/2, m_height/2);
	double end_radius = (double)m_width / 2.0;
	double start_radius = (double)m_width / 4.0;

	static double radius_normalized_offset = 0; // LIMBO increase to make the 

	for (int pass = 0; pass < nPasses; pass++)
		for (int x = 0; x < m_width; x++)
			for (int y = 0; y < m_height; y++)
			{
				RGBColor* pPixel = (RGBColor*)getPixel(x, y);

				// does not alter the bcolor since it should remain constant
				if (pPixel->r == r_bcolor && pPixel->g == g_bcolor && pPixel->b == b_bcolor)
				{
					*pPixel = bcolor;
				}
				else
				{
					if (bBlur)
					{
						// average all the surrounding colors
						// anything off the edge of the bitmap is bcolor

						pixel = (RGBColor *)getPixel(x - 1, y + 1); p[0] = pixel ? *pixel : bcolor;
						pixel = (RGBColor *)getPixel(x, y + 1); p[1] = pixel ? *pixel : bcolor;
						pixel = (RGBColor *)getPixel(x + 1, y + 1); p[2] = pixel ? *pixel : bcolor;
						pixel = (RGBColor *)getPixel(x + 1, y); p[3] = pixel ? *pixel : bcolor;
						pixel = (RGBColor *)getPixel(x - 1, y); p[4] = pixel ? *pixel : bcolor;
						pixel = (RGBColor *)getPixel(x + 1, y - 1); p[5] = pixel ? *pixel : bcolor;
						pixel = (RGBColor *)getPixel(x, y - 1); p[6] = pixel ? *pixel : bcolor;
						pixel = (RGBColor *)getPixel(x - 1, y - 1); p[7] = pixel ? *pixel : bcolor;
						pixel = (RGBColor *)getPixel(x, y); p[8] = pixel ? *pixel : bcolor;

						int r = 0;
						int g = 0;
						int b = 0;
						for (int i = 0; i < 9; i++)
						{
							r += (int)p[i].r;
							g += (int)p[i].g;
							b += (int)p[i].b;
						}

						pPixel->r = (unsigned char)((double)r / 9.0f);
						pPixel->g = (unsigned char)((double)g / 9.0f);
						pPixel->b = (unsigned char)((double)b / 9.0f);
					}

					if (bRadialBlend)
					{
						// interpolate into bcolor from the center

						double center_dist = Point2Dd(x, y).distance(center);
						if (center_dist >= start_radius)
						{
							double radius_normalized = (center_dist - start_radius) / (end_radius - start_radius); // 0 at center
							if (radius_normalized > 1.0)
								radius_normalized = 1.0;
	
							pPixel->r = (unsigned char)lerpColor((int)pPixel->r, r_bcolor, radius_normalized);
							pPixel->g = (unsigned char)lerpColor((int)pPixel->g, g_bcolor, radius_normalized);
							pPixel->b = (unsigned char)lerpColor((int)pPixel->b, b_bcolor, radius_normalized);
						}
						else
						{

						}
					}
				}
			}
}
//-----------------------------------------------------------------------------
// applyPixelProcessor
//
//-----------------------------------------------------------------------------
void Bitmap::applyPixelProcessor(PixelProcessor &proc)
{
    if (format() == CF_RGBA)
    {
        int nNumPixels = m_width * m_height;

        RGBAColor *pData = (RGBAColor *)m_pBits;

        for (int i = 0; i < nNumPixels; i++)
        {
            const RGBAPixel oldPixel(pData->r,
                                     pData->g,
                                     pData->b,
                                     pData->a);

            RGBAPixel newPixel = proc.process(oldPixel);

            pData->r = newPixel.r();
            pData->g = newPixel.g();
            pData->b = newPixel.b();
            pData->a = newPixel.a();

            pData++;
        }
    }
    else if (format() == CF_RGB)
    {
        int nNumPixels = m_width * m_height;

        RGBColor *pData = (RGBColor *)m_pBits;

        for (int i = 0; i < nNumPixels; i++)
        {
            const RGBAPixel oldPixel(pData->r,
                                     pData->g,
                                     pData->b);

            RGBAPixel newPixel = proc.process(oldPixel);

            pData->r = newPixel.r();
            pData->g = newPixel.g();
            pData->b = newPixel.b();

            pData++;
        }
    }
    else
    {
        assert(!"not implemented");
    }
}

//-----------------------------------------------------------------------------
// modulateAlpha
//
//-----------------------------------------------------------------------------
void Bitmap::modulateAlpha(double alpha)
{
    assert(alpha >= 0);
    assert(alpha <= 1);

    if (!(format() == CF_RGBA))
    {
        assert(!"requires CF_RGBA");
        return;
    }

    AlphaModulateProc proc(alpha);

    applyPixelProcessor(proc);
}

//-----------------------------------------------------------------------------
// colorByIntensity
//
//-----------------------------------------------------------------------------
void Bitmap::colorByIntensity(const RGBAPixel &foreground,
                              const RGBAPixel &background)
{
    ColorByIntensityProc proc(foreground, background);

    applyPixelProcessor(proc);
}

//-----------------------------------------------------------------------------
// createCBitmap
//
// Use this bitmap to do a CreateBitmap on the given CBitmap
//-----------------------------------------------------------------------------
void Bitmap::createCBitmap(CBitmap *pBitmap) const
{
    const int bytesPerPixel = depth();

    int numPixels = width() * height();

    std::unique_ptr<uchar[]> pTempBytes(new uchar[numPixels * 4]);
    uchar *pDst = pTempBytes.get();

    for (int row = 0; row < height(); row++)
    {
        // We need to flip vertically as we copy because
        // CreateBitmap expects rows in the opposite order
        // from how we store them
        int mirroredRow = (height() - row - 1);
        int offsetToRow = (mirroredRow * width() * bytesPerPixel);

        uchar *pSrc = (bits() + offsetToRow);
        uchar r, g, b, a;

        for (int col = 0; col < width(); ++col)
        {
            switch (format()) {
              case CF_RGBA:
                r = *pSrc++;
                g = *pSrc++;
                b = *pSrc++;
                a = *pSrc++;
                break;
              case CF_RGB:
                r = *pSrc++;
                g = *pSrc++;
                b = *pSrc++;
                a = 255;
                break;
              case CF_I8:
                r = *pSrc++;
                b = g = r;
                a = 255;
                break;
              case CF_IA8:
                r = *pSrc++;
                b = g = r;
                a = *pSrc++;
                break;
              case CF_I16:
                // LIMBO: 256 vs 257
                r = *(unsigned short *)pSrc / 256;
                pSrc += 2;
                b = g = r;
                a = 255;
                break;
              case CF_IA16:
                // LIMBO: 256 vs 257
                r = *(unsigned short *)pSrc / 256;
                pSrc += 2;
                b = g = r;
                a = *(unsigned short *)pSrc / 256;
                pSrc += 2;
                break;
              default:
                assert( !"not implemented");
            }

            // Note; CreateBitmap wants data in BGRA order
            *pDst++ = b;
            *pDst++ = g;
            *pDst++ = r;
            *pDst++ = a;
        }
    }

    VERIFY(pBitmap->CreateBitmap(width(), 
                                 height(), 1, 32,
                                 pTempBytes.get()));
}

//---------------------------------------
//
// Resizes bitmap so that width and height are power of 2.
// If they already are, don't do anything
//
void Bitmap::makeSizePowerOf2()
{
    int wd = getSizePowerOf2(m_width);
    int ht = getSizePowerOf2(m_height);

    // If size are already power of 2, no need to scale
    if ((wd==m_width)&&(ht==m_height))
        return;

    scale(wd,ht);
}

//-----------------------------------------------------------------------------
// getSizePowerOf2
//
//-----------------------------------------------------------------------------
int Bitmap::getSizePowerOf2(int size)
{
    if (size==0)
        return size;

    int startMask = 0x40000000;  // Avoid sign bit.
    int shiftMask = startMask;
    while ((size&shiftMask)==0) // Find the left most bit that is non-zero
        shiftMask>>=1;

    int revShiftMask = ~shiftMask;
    if ((size&revShiftMask)==0) // Already poer of 2
        return size;

    size = shiftMask<<1; // Return next power of 2

    return size;
}

//-----------------------------------------------------------------------------
// interpolateImage
//
//-----------------------------------------------------------------------------
bool Bitmap::interpolateImage(double x, double y, RGBAPixel &result,
                              bool wrapHoriz, bool wrapVert) const
{
    assert( channelDepth() == 1 );

    FPChecker fpc;

    // If we're wrapping the image, we assume the lookup coordinates
    // can extend one additional pixel beyond the image extents 
    double maxX = wrapHoriz ? width() - ((double)FLT_EPSILON) : width() - 1;
    double maxY = wrapVert ? height() - ((double)FLT_EPSILON) : height() - 1;

    x = MiscMath::clamp<double>( x, 0.0, maxX );
    y = MiscMath::clamp<double>( y, 0.0, maxY );

    // Get integer and fractional coordinates
    int iX = (int) x;
    int iY = (int) y;
    double fX = x - iX;
    double fY = y - iY;

    int iNextX = (iX + 1);
    int iNextY = (iY + 1);

    // Wrap or clamp according to the flags
    iNextX = wrapHoriz ? (iNextX % width()) : min(iNextX, maxX);
    iNextY = wrapVert ? (iNextY % height()) : min(iNextY, maxY);

    // Get the 4 neighbor pixels
    RGBAPixel n00, n10, n01, n11;

    if (!getRGBAPixel(iX, iY, n00))
        return false;

    if (!getRGBAPixel(iX, iNextY, n10))
        return false;

    if (!getRGBAPixel(iNextX, iY, n01))
        return false;

    if (!getRGBAPixel(iNextX, iNextY, n11))
        return false;

    // Do the interpolation between the 4 neighbors
    int nDepth = channels();
    for (int i = 0; i < nDepth; ++i)
    {
        result[i] = fastftol(
                    (1-fY) * (1-fX) * n00[i]
                  +    fY  * (1-fX) * n10[i]
                  + (1-fY) *    fX  * n01[i]
                  +    fY  *    fX  * n11[i]
                  + 0.5f); // round
    }

    // If the image doesn't have an alpha channel, then assume opaque.
    if (nDepth < 4)
    {
        result[3] = 255;
        if (nDepth == 1)		// GrayScale
        {
            // getRGBAPixel will fill in all 4 channels, but
            // the above interpolation will skip all but R.
            result[2] = result[1] = result[0];
        }
        else if (nDepth == 2)	// GrayScale+Alpha
        {
            // getRGBAPixel will fill in all 4 channels, but
            // the above interpolation will skip all but R and A.
            result[3] = result[1];	// A is in the second channel
            result[2] = result[1] = result[0];	// fill in GB from R
        }
    }

    return true;
}


//-----------------------------------------------------------------------------
// interpolateImage
//
//-----------------------------------------------------------------------------
bool Bitmap::interpolateImage(double x, double y, double &intensity,
                              bool wrapHoriz, bool wrapVert) const
{
    FPChecker fpc;

    // If we're wrapping the image, we assume the lookup coordinates
    // can extend one additional pixel beyond the image extents 

    //double maxX = wrapHoriz ? width() - FLT_EPSILON : width() - 1;
    //double maxY = wrapVert ? height() - FLT_EPSILON : height() - 1;
    const double EPSILON = 1e-12;	// DBL_EPSILON = 2e-16
    double maxX = wrapHoriz ? ((double)width() - EPSILON) : width() - 1;
    double maxY = wrapVert ?  ((double)height() - EPSILON) : height() - 1;

    x = MiscMath::clamp<double>( x, 0.0, maxX );
    y = MiscMath::clamp<double>( y, 0.0, maxY );

    // Get integer and fractional coordinates
    int iX = (int) x;
    int iY = (int) y;
    double fX = x - iX;
    double fY = y - iY;

    int iNextX = (iX + 1);
    int iNextY = (iY + 1);

    // Wrap or clamp according to the flags
    iNextX = wrapHoriz ? (iNextX % width()) : min(iNextX, maxX);
    iNextY = wrapVert ? (iNextY % height()) : min(iNextY, maxY);

    if (m_colorFormat == CF_I16)	// channelDepth == 2 && channels == 1
    {
        // Get the 4 neighbor pixels
        I16Color *n00 = (I16Color *) getPixel(iX, iY);
        if (!n00) return false;
        I16Color *n10 = (I16Color *) getPixel(iX, iNextY);
        if (!n10) return false;
        I16Color *n01 = (I16Color *) getPixel(iNextX, iY);
        if (!n01) return false;
        I16Color *n11 = (I16Color *) getPixel(iNextX, iNextY);
        if (!n11) return false;

        // Do the interpolation between the 4 neighbor pixels
        double result;
        result    = (1-fY) * (1-fX) * *n00
                  +    fY  * (1-fX) * *n10
                  + (1-fY) *    fX  * *n01
                  +    fY  *    fX  * *n11;

        // produce an intensity in the range [0, 1]
        intensity = result / 65535.0;
    }
    else if ( channelDepth() == 2 )	// other 16b formats
    {
        // Get the 4 neighbor pixels
        I16Color *n00 = (I16Color *) getPixel(iX, iY);
        if (!n00) return false;
        I16Color *n10 = (I16Color *) getPixel(iX, iNextY);
        if (!n10) return false;
        I16Color *n01 = (I16Color *) getPixel(iNextX, iY);
        if (!n01) return false;
        I16Color *n11 = (I16Color *) getPixel(iNextX, iNextY);
        if (!n11) return false;

        // Up to 4 channels. In practice, it will usually be 2 channels.
        // The emboss code will convert RGB(A) to gray scale up front.
        double result[4];
        int nDepth = channels();

        // Do the interpolation between the 4 neighbor pixels
        for (int i = 0; i < nDepth; ++i)
        {
            result[i] = (1-fY) * (1-fX) * n00[i]
                      +    fY  * (1-fX) * n10[i]
                      + (1-fY) *    fX  * n01[i]
                      +    fY  *    fX  * n11[i];
        }

        // produce an intensity in the range [0, 1]
        if (nDepth <= 2)		// GrayScale
        {
            intensity = result[0] / 65535.0;

            if (nDepth == 2) {				// GrayScale+Alpha
                // attenuate by alpha
                intensity *= result[1] / 65535.0;
            }
        }
        else // RGB16, RGBA16 (future)
        {
            intensity = (RED_COEFF * result[0] +
                         GREEN_COEFF * result[1] +
                         BLUE_COEFF * result[2]) / 65535.0;

            if (nDepth == 4) {					// RGBA16
                // attenuate by alpha
                intensity *= result[1] / 65535.0;
            }
        }
    }
    else
    {
    assert( channelDepth() == 1 );

    // Get the 4 neighbor pixels
    I8Color *n00 = (I8Color *) getPixel(iX, iY);
    if (!n00)
        return false;

    I8Color *n10 = (I8Color *) getPixel(iX, iNextY);
    if (!n10)
        return false;

    I8Color *n01 = (I8Color *) getPixel(iNextX, iY);
    if (!n01)
        return false;

    I8Color *n11 = (I8Color *) getPixel(iNextX, iNextY);
    if (!n11)
        return false;

    // Up to 4 channels. In practice, it will usually be 2 channels.
    // The emboss code will convert RGB(A) to gray scale up front.
    int nDepth = channels();
    double result[4];

    // Do the interpolation between the 4 neighbor pixels
    for (int i = 0; i < nDepth; ++i)
    {
        result[i] = (1-fY) * (1-fX) * n00[i]
                  +    fY  * (1-fX) * n10[i]
                  + (1-fY) *    fX  * n01[i]
                  +    fY  *    fX  * n11[i];
    }

    if (format() == CF_I8)
    {
        // divide by 255 to produce an intensity in the range [0, 1]
        intensity = result[0] / 255.0;
    }
    else if (format() == CF_IA8)
    {
        // divide by 255 to produce an intensity in the range [0, 1]
        intensity = result[0] / 255.0;
        // attenuate by alpha
        intensity *= result[1] / 255.0;
    }
    else if (nDepth == 4 && result[3] == 0)
    {
        // Trivial case for zero alpha
        intensity = 0;
    }
    else	// RGB, RGBA
    {
        // Weight the RGB components based on eye-perceived weighting
        // and then divide by 255 to produce an intensity in the range [0, 1]
        intensity = (RED_COEFF * result[0] +
                     GREEN_COEFF * result[1] +
                     BLUE_COEFF * result[2]) / 255.0;

        // Mute the intensity by the alpha value
        if (nDepth == 4)	// RGBA
        {
            intensity *= result[3] / 255.0;
        }
    }
    }

    return true;
}

unsigned char floatToComponent(float f)
{
	int comp = (int)(f + 0.5); // round
	if (comp < 0)
		comp = 0;
	else if (comp > 255)
		comp = 255;
	return (unsigned char)comp;
}

void Bitmap::unPremultiplyAlpha()
{
	if (format() == CF_RGBA)
	{
		int sizeBytes = getSizeBytes();
        unsigned char* pByte;
        unsigned char alpha;

        // Start at first alpha
        for (int byteIndex = 3; byteIndex < sizeBytes; byteIndex +=4)
        {
            pByte = bits() + byteIndex;
            alpha = *pByte;
			if (alpha == 255)
			{
				// no-op
			}
			else if (alpha == 0)
			{
				// transparent black
				*(--pByte) = 0;
                *(--pByte) = 0;
                *(--pByte) = 0;
			}
			else // if (alpha < 255)
            {
				float alpha_normalized = (float)alpha / 255.0f;
		
				--pByte;
				*pByte = floatToComponent((float)(*pByte) / alpha_normalized);
				--pByte;
				*pByte = floatToComponent((float)(*pByte) / alpha_normalized);
				--pByte;
				*pByte = floatToComponent((float)(*pByte) / alpha_normalized);
            }
        }
    }
}

// Bugzilla 1437 In some images, like Photoshop, (semi) transparent pixels (alpha != 255)
// are premultiplied by alpha and blended with white, causing halos when used
// in other apps like ours.
void Bitmap::unPremultiplyAlphaWhite()
{
    if (format() == CF_RGBA)
    {
        int sizeBytes = getSizeBytes();
        unsigned char* pByte;
        unsigned char alpha;
        float term1, term2, alpha_f;

        // Start at first alpha
        for (int byteIndex = 3; byteIndex < sizeBytes; byteIndex +=4)
        {
            pByte = bits() + byteIndex;
            alpha = *pByte;
            if (alpha == 0)
            {
                // Original color was transparent, make it black
                *(--pByte) = 0;
                *(--pByte) = 0;
                *(--pByte) = 0;
            }
            else if (alpha < 255)
            {
                // Original color was sem-transparent, back-solve
                // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
                alpha_f = (float) alpha;
                term2 = 1.0f - 255.0f / alpha_f;

                term1 = (float) (*(--pByte)) / alpha_f;
                *pByte = (unsigned char) clamp(((term1 + term2) * 255.0f), 0.0f, 255.0f);
                term1 = (float) (*(--pByte)) / alpha_f;
                *pByte = (unsigned char) clamp(((term1 + term2) * 255.0f), 0.0f, 255.0f);
                term1 = (float) (*(--pByte)) / alpha_f;
                *pByte = (unsigned char) clamp(((term1 + term2) * 255.0f), 0.0f, 255.0f);
            }
            // else alpha = 255, original color was opaque, use unchanged
        }
    }
}

bool Bitmap::transparentWhiteToBlack( )
{
    if (format() == CF_RGBA)
    {
        unsigned char *out_ptr = bits();

        for ( int i = 0; i < width() * height(); ++i )
        {
            unsigned char *currPix = out_ptr;
            unsigned char red = *out_ptr++;
            unsigned char green = *out_ptr++;
            unsigned char blue = *out_ptr++;
            unsigned char alpha = *out_ptr++;
            // Map transparent white to transparent black.
            if ((255 == red) && (255 == green) && (255 == blue) && (0 == alpha))
            {
                *currPix++ = 0;	// r
                *currPix++ = 0;	// g
                *currPix++ = 0;	// b
            }
        }

        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// deinit
//
//-----------------------------------------------------------------------------
void Bitmap::deinit()
{
    // clear current state
    delete_array(m_pBits);
    m_width  = 0;
    m_height = 0;
}

void Bitmap::GetMaskAboveThreshold( int threshold, int minX, int maxX, int minY, int maxY, unsigned char* oMask) const
{
	int sizeX = maxX - minX + 1;
	int sizeY = maxY - minY + 1;

	memset( oMask, 0, sizeof( unsigned char)*sizeX*sizeY );

	uchar* pixAddr = 0;

	unsigned int cnt = 0;
	for(int tmpY = minY; tmpY <= maxY; tmpY++)
	{
		for(int tmpX = minX; tmpX <= maxX; tmpX++)
		{
			pixAddr = (uchar*)getPixel(tmpX,tmpY);
			if(pixAddr != 0 && *pixAddr > threshold )
			{
				oMask[cnt] = 1; 			
			}
			cnt++;
		}
	}
}

// Looks at the image, and considers only pixels that are above threshold.
// The seed is the geometric average of these points, provided that
// the seed also is above threshold.
// If it is not (e.g. non-convex shape), it looks at scanlines passing
// through the seed, in	the two directions.
// If it finds a direction where the scanline intersects the shape
// only in one segment, it returns the center of that segment.
// Otherwise, it returns the center of the biggest segment.
// Returns false if something went wrong.
bool Bitmap::FindSeedPointAboveThreshold(int threshold, int& seedX, int&seedY ) const
{

	int bmwidth = width();
	int bmheight = height();

	double seed[2] = {0.0, 0.0};
	double howMany = 0.0;

	for(int tmpX = 0; tmpX < bmwidth; tmpX++)
	{
		for(int tmpY = 0; tmpY < bmheight; tmpY++)
		{
			if(*((uchar*)getPixel(tmpX,tmpY)) > threshold )
			{
				seed[0] += (double)tmpX;
				seed[1] += (double)tmpY;
				howMany+=1.0;
			}
		}
	}
	if(howMany < 0.01)
		return false;

	seed[0] /= howMany;
	seed[1] /= howMany;

	if(*((uchar*)getPixel((int)seed[0],(int)seed[1])) > threshold)
	{
		seedX = (int)seed[0];
		seedY = (int)seed[1];

		//TBD could find segments here too...
	}
	else // transverse the segments and find the longest contiguous for now
	{
		std::vector<FS::Segment> mysegsX;
		std::vector<FS::Segment> mysegsY;
		//first in X
		int out = 1;
		for(int tmpX = 1; tmpX < bmwidth; tmpX++)
		{
			// got an 'in'
			if(*((uchar*)getPixel(tmpX,(int)seed[1])) > threshold)
			{
				if(out == 1) // it was out, now in, so new segment
				{
					mysegsX.push_back( FS::Segment(tmpX, 0, (int)seed[1]) );
					out = 0; // now in (=not out)
				}
				else // was in, still in
				{
					mysegsX.back().SetEnd(tmpX);
				}
			}
			else // got an out. No need to do anything special
			{
				out = 1;
			}
		}

		// now try in Y
		out = 1;
		for(int tmpY = 1; tmpY < bmheight; tmpY++)
		{
			// got an 'in'
			if(*((uchar*)getPixel((int)seed[0],tmpY)) > threshold)
			{
				if(out == 1) // it was out, now in, so new segment
				{
					mysegsY.push_back( FS::Segment(tmpY, 1, (int)seed[0]) );
					out = 0; // now in (=not out)
				}
				else // was in, still in
				{
					mysegsY.back().SetEnd(tmpY);
				}
			}

			else // got an out. No need to do anything special
			{
				out = 1;
			}
		}
		// X is unique
		if( mysegsX.size() == 1 && mysegsY.size() > 1 )
		{
			mysegsX.back().ToPoint(seedX, seedY);
		}
		else if( mysegsX.size() > 1 && mysegsY.size() == 1 ) // Y is unique
		{
			mysegsY.back().ToPoint(seedX, seedY);
		}
		else
		{
			int maxLen = -1;
			
			// get the biggest of all
			std::vector<FS::Segment>::iterator segmentIter;
			for( segmentIter = mysegsX.begin(); segmentIter != mysegsX.end(); segmentIter++)
			{
				if( segmentIter->GetLength() > maxLen)
				{
					segmentIter->ToPoint(seedX, seedY);
				}
			}
			for( segmentIter = mysegsY.begin(); segmentIter != mysegsY.end(); segmentIter++)
			{
				if( segmentIter->GetLength() > maxLen)
				{
					segmentIter->ToPoint(seedX, seedY);
				}
			}
		}
	}

	if(seedX < 0 || seedX >= bmwidth)
		seedX = bmwidth/2;
	if(seedY < 0 || seedY >= bmheight)
		seedY = bmheight/2;

	return true;
}

namespace FS
{

// gluScaleImage with the following restrictions ...
// 1) Doesn't do data type conversions
// 2) Only understands the formats supported by our Bitmap class.
//
// Advantages ...
// 1) gluScaleImage deals with the multiplicity of possible input formats
// by copying into a internal buffer. With large images, it was possible
// for the allocation of the buffer to fail. This version skips all that.
// 2) Fixes array wraparound bugs when upscaling.

void FFScaleImage(ColorFormat fmt, 
               int widthin, int heightin,
               const void *datain,
               int widthout, int heightout,
               void *dataout)
{
    double x, lowx, highx, convx, halfconvx;
    double y, lowy, highy, convy, halfconvy;
    double xpercent,ypercent;
    double percent;
    /* Max components in a format is 4, so... */
    double totals[4];
    double area;
    int i,j,k,yint,xint,xindex,yindex;
    int temp;

    GLenum dtype = Bitmap::getGLDataType( fmt );
    int components = Bitmap::channels( fmt );
    GLubyte *datain8 = (GLubyte *)datain;
    GLushort *datain16 = (GLushort *)datain;
    GLubyte *dataout8 = (GLubyte *)dataout;
    GLushort *dataout16 = (GLushort *)dataout;

    convy = (double) heightin/heightout;
    convx = (double) widthin/widthout;
    halfconvx = convx/2;
    halfconvy = convy/2;

    // Iterate over output image - i is output coord
    for (i = 0; i < heightout; i++) {
    y = convy * (i+0.5);
    if (heightin > heightout) {
        // Down-scaling - use variable size window in terms of *input* coords.
        // Works out to be fixed size (-0.5 to 0.5) in terms of *output* coords.
        highy = y + halfconvy;
        lowy = y - halfconvy;
    } else {
        // Up-scaling - use fixed size window in terms of *input* coords.
        highy = y + 0.5;
        lowy = y - 0.5;
    }
    // Avoid array overflow issues.
    // (modulo logic below turns potential overflows into wraparounds.)
    if (lowy < 0.0) {
        lowy = 0.0;
    }
    if (highy > heightin) {
        highy = heightin;
    }
    // Iterate over output image - j is output coord
    for (j = 0; j < widthout; j++) {
        x = convx * (j+0.5);
        if (widthin > widthout) {
        // Down-scaling - use variable size window in terms of *input* coords.
        // Works out to be fixed size (-0.5 to 0.5) in terms of *output* coords.
        highx = x + halfconvx;
        lowx = x - halfconvx;
        } else {
        // Up-scaling - use fixed size window in terms of *input* coords.
        highx = x + 0.5;
        lowx = x - 0.5;
        }
        // Avoid array overflow issues.
        // (modulo logic below turns potential overflows into wraparounds.)
        if (lowx < 0.0) {
        lowx = 0.0;
        }
        if (highx > widthin) {
        highx = widthin;
        }

        // Iterate over input image - x,y are input coords

        /*
        ** Ok, now apply box filter to box that goes from (lowx, lowy)
        ** to (highx, highy) on input data into this pixel on output
        ** data.
        */
        totals[0] = totals[1] = totals[2] = totals[3] = 0.0;
        area = 0.0;

        y = lowy;
        yint = floor(y);
        while (y < highy) {

        // The modulo logic stops crashes but may lead to weird wraparounds.
        //assert( (0 <= yint) && (yint < heightin) );

        yindex = (yint + heightin) % heightin;

        if (highy < yint+1) {
            ypercent = highy - y;
        } else {
            ypercent = yint+1 - y;
        }

        x = lowx;
        xint = floor(x);

        while (x < highx) {

            // The modulo logic stops crashes but may lead to weird wraparounds
            //assert( (0 <= xint) && (xint < widthin) );

            xindex = (xint + widthin) % widthin;

            if (highx < xint+1) {
            xpercent = highx - x;
            } else {
            xpercent = xint+1 - x;
            }

            percent = xpercent * ypercent;
            area += percent;
            temp = (xindex + (yindex * widthin)) * components;

            if (dtype == GL_UNSIGNED_BYTE)
            for (k = 0; k < components; k++) {
            totals[k] += datain8[temp + k] * percent;
            }
            else if (dtype == GL_UNSIGNED_SHORT)
            for (k = 0; k < components; k++) {
            totals[k] += datain16[temp + k] * percent;
            }

            xint++;
            x = xint;
        }
        yint++;
        y = yint;
        }

        temp = (j + (i * widthout)) * components;
        if (dtype == GL_UNSIGNED_BYTE)
        for (k = 0; k < components; k++) {
        dataout8[temp + k] = (totals[k]+0.5)/area;
        }
        else if (dtype == GL_UNSIGNED_SHORT)
        for (k = 0; k < components; k++) {
        dataout16[temp + k] = (totals[k]+0.5)/area;
        }
    }
    }
}

//-----------------------------------------------------------------------------
// dumpImageBMP
//
// Dump an Array2D<float> as greyscale bmp uchar (1 byte/pixel) image. 
//-----------------------------------------------------------------------------
void dumpImageBMP(Array2D<float>& image, const _TCHAR *filename)
{
    int numpixels = image.width()*image.height();
    Array2D<uchar> outImage( image.width(), image.height() );
    int i,j;
    int lt=0, gt=0;

    for (j=0;j<image.height();j++)
    for (i=0;i<image.width();i++)
    {
        float fval = image(i,j);
        uchar val;
        if (fval<0)
        {
            lt++;
            val = 0;
        }
        else if (fval>255)
        {
            gt++;
            val = 255;
        }
        else
        {
            val = fastround(fval);
        }
        outImage(i,j) = val;
    }

    PRINTLN( "dumpImage<float>, clamped "
        << lt << " vals below zero, "
        << gt << " above 255");
    dumpImageBMP(outImage, filename);
}

void dumpImageBMP(Array2D<float>& image, const _TCHAR *filename, float min, float max)
{
    int numpixels = image.width()*image.height();
    Array2D<uchar> outImage( image.width(), image.height() );
    int i,j;
    float range = max - min;

    for (j=0;j<image.height();j++)
    for (i=0;i<image.width();i++)
    {
        float fval = image(i,j);
        int val = 255.0f * (fval-min)/range;

        if (val<0)
            val = 0;
        else if (val>255)
            val = 255;

        outImage(i,j) = val;
    }

    dumpImageBMP(outImage, filename);
}

//-----------------------------------------------------------------------------
// dumpImageBMP
//
// Dump Array2D<uchar> as grayscale bmp 1 byte/pixel.
//-----------------------------------------------------------------------------
void dumpImageBMP(Array2D<uchar>& image, const _TCHAR *filename)
{
    Bitmap bmp;
    bmp.init(image.width(), image.height(), CF_RGB);
    for (int i = 0; i < bmp.width(); ++i)
    {
        for (int j = 0; j < bmp.height(); ++j)
        {
            RGBColor* p = (RGBColor*) bmp.getPixel(i,j);
            p->r = p->g = p->b = image(i,j);
        }
    }
    bmp.saveToFile(filename);
}

//-----------------------------------------------------------------------------
// dumpImageBMP
//
// Dump Array2D<bool> as grayscale bmp 1 byte/pixel.
//-----------------------------------------------------------------------------
void dumpImageBMP(Array2D<bool>& image, const _TCHAR *filename)
{
    Bitmap bmp;
    bmp.init(image.width(), image.height(), CF_RGB);
    for (int i = 0; i < bmp.width(); ++i)
    {
        for (int j = 0; j < bmp.height(); ++j)
        {
            RGBColor* p = (RGBColor*) bmp.getPixel(i,j);
            p->r = p->g = p->b = image(i,j) ? 255 : 0;
        }
    }
    bmp.saveToFile(filename);
}

void dumpImageBMP(Array2D<int>& image, const _TCHAR *filename, ColorFormat fmt)
{
    Bitmap bmp;
    bmp.init(image.width(), image.height(), fmt);
    for (int i = 0; i < bmp.width(); ++i)
    {
        for (int j = 0; j < bmp.height(); ++j)
        {
			if (fmt == CF_RGB)
			{
				RGBColor* p = (RGBColor*) bmp.getPixel(i,j);
				int val = image(i,j);
				RGBAPixel pix;
				intToRGBAPixel(val, pix);

				p->r = pix.r();
				p->g = pix.g();
				p->b = pix.b();
			}
			else if (fmt == CF_RGBA)
			{
				RGBAColor* p = (RGBAColor*) bmp.getPixel(i,j);
				int val = image(i,j);
				RGBAPixel pix;
				intToRGBAPixel(val, pix);

				p->r = pix.r();
				p->g = pix.g();
				p->b = pix.b();
				p->a = pix.a();
			}
        }
    }
    bmp.saveToFile(filename);
}

void Bitmap::CropBackgroundColor(const RGBAPixel &backgroundColor)
{
	// Find target extents
	int minX = m_width;
	int minY = m_height;
	int maxX = 0;
	int maxY = 0;

	int row = 0;
	int col = 0;

	bool foundMinX = false;
	bool foundMinY = false;
	bool foundMaxX = false;
	bool foundMaxY = false;

	// Scan left to right, top to bottom to find min y extent
	while (!foundMinY && row < m_height)
	{
		int x = 0;
		bool foundNonBGColor = false;
		while (!foundNonBGColor && x < m_width)
		{
			RGBAPixel pixel;
			getRGBAPixel(x, row, pixel);
			if (backgroundColor != pixel)
			{
				foundNonBGColor = true;
				break;
			}
			x++;
		}
		if (foundNonBGColor)
		{
			minY = row;
			foundMinY = true;
			break;
		}
		row++;
	}
	if (!foundMinY)
	{
		// Found only background color pixels... no crop to be done.. or resize to empty?
		return;
	}

	// Scan top to bottom, left to right to find min X extent
	while (!foundMinX && col < m_width)
	{
		int y = minY; // Can start from established minY
		bool foundNonBGColor = false;
		while (!foundNonBGColor && y < m_height)
		{
			RGBAPixel pixel;
			getRGBAPixel(col, y, pixel);
			if (backgroundColor != pixel)
			{
				foundNonBGColor = true;
				break;
			}
			y++;
		}
		if (foundNonBGColor)
		{
			minX = col;
			foundMinX = true;
			break;
		}
		col++;
	}
	// Since min y was found, it should be certain all others will be too
	assert(foundMinX);
	// Scan left to right, bottom to top to find maxY extent
	row = m_height - 1; // start at bottom
	while (!foundMaxY && row >= minY)
	{
		int x = minX; // Can start from established min X
		bool foundNonBGColor = false;
		while (!foundNonBGColor && x < m_width)
		{
			RGBAPixel pixel;
			getRGBAPixel(x, row, pixel);
			if (backgroundColor != pixel)
			{
				foundNonBGColor = true;
				break;
			}
			x++;
		}
		if (foundNonBGColor)
		{
			maxY = row;
			foundMaxY = true;
			break;
		}
		row--;
	}
	assert(foundMaxY);
	// Scan top to bottom, right to left to find maxX extent
	col = m_width - 1; // start at right
	while (!foundMaxX && col >= minX)
	{
		int y = minY; // Can start from established min Y
		bool foundNonBGColor = false;
		while (!foundNonBGColor && y < m_height)
		{
			RGBAPixel pixel;
			getRGBAPixel(col, y, pixel);
			if (backgroundColor != pixel)
			{
				foundNonBGColor = true;
				break;
			}
			y++;
		}
		if (foundNonBGColor)
		{
			maxX = col;
			foundMaxX = true;
			break;
		}
		col--;
	}
	assert(foundMaxY);

	if (minX == 0 && minY == 0 && maxX == m_width - 1 && maxY == m_height - 1)
	{
		// Already has perfect extents
		return;
	}

	int deltaLeft = minX;
	int deltaTop = minY;
	int deltaRight = m_width - (maxX + 1);
	int deltaBottom = m_height - (maxY + 1);
	// Do the Crop
	resizeCanvas(deltaLeft, deltaRight, deltaTop, deltaBottom, 0); // should be no fill
}

namespace
{

//-----------------------------------------------------------------------------
// applyPixelProcessor
//
// Iterate throught the given bitmap, calling the given
// PixelProcessor on each pixel.
//-----------------------------------------------------------------------------
void applyPixelProcessor(HBITMAP hBmp, PixelProcessor &proc)
{
    // retrieve bitmap values
    BITMAPINFO bmpInfo;
    memset(&bmpInfo, 0, sizeof(BITMAPINFO));
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    HDC hDC = ::CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
    int iRet = ::GetDIBits(hDC, hBmp, 0, 0, NULL, &bmpInfo, DIB_RGB_COLORS);
    int iWidth = bmpInfo.bmiHeader.biWidth;
    int iHeight = bmpInfo.bmiHeader.biHeight;

    if (bmpInfo.bmiHeader.biBitCount != 32)
    {
        assert(!"expected 32-bit bitmap");

        // This routine is (so far) only written to handle
        // 32-bit bitmaps, so we just abort if not 32 bits
        return;
    }

    int iBPP = bmpInfo.bmiHeader.biBitCount / 8;
    int bmpSize = iWidth *
                  iHeight *
                  iBPP;
    BYTE *pBits = new BYTE[ bmpSize ];

    // get a pointer to the bits
    memset(&bmpInfo, 0, sizeof(BITMAPINFO));
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = iWidth;
    bmpInfo.bmiHeader.biHeight = iHeight;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = iBPP * 8;
    bmpInfo.bmiHeader.biCompression = BI_RGB;
    iRet = ::GetDIBits(hDC, hBmp, 0,
                       bmpInfo.bmiHeader.biHeight,
                       pBits, &bmpInfo, DIB_RGB_COLORS);
    bmpSize = iWidth *
              iHeight *
              iBPP;

    // set pointers to bitmap bits
    unsigned char *out_ptr = pBits;
    unsigned char *in_ptr = pBits;

    for ( int iIndex = 0; iIndex != bmpSize; iIndex+=iBPP )
    {
        const uchar b = *in_ptr++;
        const uchar g = *in_ptr++;
        const uchar r = *in_ptr++;

        // Assume opaque, even those this is a 32-bit bitmap, it
        // doesn't (always) appear to have valid alpha values, 
        // I'm seeing 0's for images from 24-bit files which should
        // have 255 alpha.  Not sure if we're doing something 
        // wrong somewhere else, or if there's a way to detect
        // if alpha is valid?
        const uchar a = 255;
         *in_ptr++;

        RGBAPixel input(r, g, b, a);

        RGBAPixel output = proc.process(input);

        // write the new color
        *out_ptr++ = output.b();
        *out_ptr++ = output.g();
        *out_ptr++ = output.r();
        *out_ptr++ = output.a();
    }

    // reset the bitmap bits
    ::SetDIBits( hDC,
                 hBmp,
                 0,
                 iHeight,
                 pBits,
                 &bmpInfo,
                 DIB_RGB_COLORS );

    // cleanup
    delete_array(pBits);
    ::DeleteDC(hDC);
}

} // anon namespace


//-----------------------------------------------------------------------------
// BitmapReplaceColor
//
// Replace all pixles of one color in a bitmap
// with pixels of another color.
//-----------------------------------------------------------------------------
void BitmapReplaceColor(HBITMAP hBmp, DWORD orig, DWORD repl, int tolerance255)
{
    RGBAPixel origPixel(GetRValue(orig),
                        GetGValue(orig),
                        GetBValue(orig));

    RGBAPixel replPixel(GetRValue(repl),
                        GetGValue(repl),
                        GetBValue(repl));

    ReplaceColorProc proc(origPixel, replPixel, tolerance255);

    applyPixelProcessor(hBmp, proc);
}

//-----------------------------------------------------------------------------
// BitmapReplaceSystemGrey
//
// replace standard grey (192, 192, 192) with current system equivalent
// (COLOR_3DFACE).  Need to this when loading in 32 bit bitmaps from
// resources in order to get the background colors to map correctly.
//-----------------------------------------------------------------------------
void BitmapReplaceSystemGrey(HBITMAP hBmp, bool useColorScheme, COLORREF cTransparent, int tolerance255)
{
	COLORREF bgColor = GetSysColor(COLOR_3DFACE);

	if (useColorScheme && getGUIColorRegistry()->isEnabled()) // WorkflowColorScheme
		bgColor = getGUIColorRegistry()->getColorRef(Symbol(_T("DialogBar_Bkgd_Color"))); // WorkflowColorScheme

	BitmapReplaceColor(hBmp, cTransparent, bgColor, tolerance255);
}

//-----------------------------------------------------------------------------
// ColorByIntensity
//
//-----------------------------------------------------------------------------
void ColorByIntensity(HBITMAP hBmp, 
                      const RGBAPixel &foreground,
                      const RGBAPixel &background)
{
    ColorByIntensityProc proc(foreground, background);

    applyPixelProcessor(hBmp, proc);
}

bool Bitmap::load(LPCTSTR filename, bool flipImage)
{
    //LIMBO: flip?
    bool ok = loadBmp( filename, CF_RGBA );
    if(ok)
        makeSizePowerOf2();

    return ok;
}

void Bitmap::unload()
{
    clear();
}

void Bitmap::setGlTypeTarget(int type)
{
    //LIMBO : add support for cubemaps: GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB

    assert( type == GL_TEXTURE_2D || type == GL_TEXTURE_RECTANGLE_ARB );
    m_GlType = type;
}

int Bitmap::get_type() const /// equivalent to gl target
{
    return m_GlType;
}

bool Bitmap::upload_texture()
{
    int components = depth();
    int rowbytes = bytesPerRow();

	GLint internalFrmt = 0;
	switch( components )
    {
        case 1: internalFrmt = GL_LUMINANCE8; break;
        case 3: internalFrmt = GL_RGB8;		  break;
        case 4: internalFrmt = GL_RGBA8;	  break;
        default:
            assert(!"unknown format");
            return 0;
    }

    //glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, rowbytes / components );
    glTexImage2D(get_type(), 0, internalFrmt, width(),  
        height(), 0, getGLFormat(), GL_UNSIGNED_BYTE, bits());

    //glPopClientAttrib();

    return true;
}

bool Bitmap::is_loaded() const
{
    return bits() ? true : false;
}

//Finds the centroid of the greyscale bitmap
vector<float> Bitmap::findCentroid( float zLevel) 
{
	vector<vector<float>> selPixels;
	for(int i=0; i<m_width; i++)
	{
		for(int j=0;j<m_height;j++)
		{
			I8Color *p = (I8Color *)getPixel(i,j);
			unsigned char val = *p;
			if(val>0)
			{
				vector<float> selPix;
				selPix.push_back(i);
				selPix.push_back(j);
				selPixels.push_back(selPix);
			}
		}
	}

	float width = 0;
	float height = 0;
	for(int k = 0; k < selPixels.size(); k++ )
	{
		vector<float> selPix = selPixels.at(k);
		width += selPix.at(0);
		height += selPix.at(1);
	}

	if(selPixels.size() != 0)
	{
		width = width/selPixels.size();
		height = height/selPixels.size();
	}

	vector<float> selPix;
	selPix.push_back(width);
	selPix.push_back(height);
	selPix.push_back(zLevel);

	float numPix = selPixels.size();
	
	selPix.push_back(numPix); // num of pixels which are 

	RGBColor *p = (RGBColor *)getPixel(width,height);
	assert(p);
	if(p)
	{
		p->r = 255;
		p->g = 10;
		p->b = 10;
	}
	return selPix;
}

double Bitmap::getPixelValueAt(double x, double y, bool interpolate)
{
	double val;
	if(interpolate)
	{
		int xFloor, xCeil;
		int yFloor, yCeil;
		xFloor = (int) floor(x);
		xCeil = (int) ceil(x);

		yFloor = (int)floor(y);
		yCeil = (int)ceil(y);

		if(!(xCeil < width() && yCeil < height() ))
		{
			return 0;
		}
		double v0,v1,v2,v3;

		double xHeight = xCeil - xFloor;
		double yHeight = yCeil - yFloor;

		double denom = xHeight*yHeight;

		v0 = *((I8Color *)getPixel(	xFloor, yFloor )); //f(0,0)
		v1 = *((I8Color *)getPixel(	xCeil, yFloor )); //f(1,0)
		v2 = *((I8Color *)getPixel(	xFloor,yCeil )); //f(0,1)
		v3 = *((I8Color *)getPixel(	xCeil,yCeil )); //f(1,1)

		val = ( v0*(xCeil-x)*(yCeil-y) + v1*(x-xFloor)*(yCeil-y) + v2*(xCeil-x)*(y-yFloor) + v3*(x-xFloor)*(y-yFloor) )/ denom;
		
		return val;
	}
	else
	{
		val = *((I8Color *)getPixel(x,y));
		return val;
	}
}

#ifndef _RELEASE
// Debugging utility.  Only really practical for small images,
// given the potentially enormous size of these files.
void Bitmap::savePPMAscii( const char *filename ) const
{
    FILE *fp = fopen( filename, "wb" );
    if( !fp ) return;

    ColorFormat frmt = format();
    if( frmt == CF_I8 || frmt == CF_I16 )
        fprintf( fp, "P2\n" );	// "Portable Gray Map"
    else
        fprintf( fp, "P3\n" );	// "Portable Pixel Map"

    fprintf( fp, "# Width, Height\n%d %d\n", width(), height() );
    //if( frmt == CF_I16 )
    //fprintf( fp, "# Max Color\n65536\n" );
    //else
    fprintf( fp, "# Max Color\n256\n" );

    RGBAPixel pixel;

    // PPM/PGM doc:
    // A raster of Height rows, in order from top to bottom. 
    // Each row consists of Width pixels, in order from left to right.

    // FF Bitmaps seem to follow the Microsoft bmp bottom-up convention.
    // To display properly, we need to convert to top-down.
    //for( int j=0; j<height(); ++j )
    for( int j=height()-1; j>=0; --j )
	{
    fprintf( fp, "# Row %d\n", j );
    for( int i=0; i<width(); ++i )
    {
        bool col = false;
        if( width() > 64)       col = (0==i%64);
		else if( width() > 16)  col = (0==i%16);
        if( col ) fprintf( fp, "# Column %d\n", i );

        getRGBAPixel( i, j, pixel );
        if( frmt == CF_I8 || frmt == CF_I16 )
            fprintf( fp, "%d\n", pixel.r() );	// All channels have same value
        else
            fprintf( fp, "%d %d %d\n", pixel.r(), pixel.g(), pixel.b() );
    }
    }

    fclose( fp );
    return;
}
#endif

//constructor
Sheaf::Sheaf()
:m_slices(0)
{
}

Sheaf::~Sheaf()
{
	clear();
}
void Sheaf::addBitmap( const Bitmap bmp)
{
	m_slices.push_back(bmp);
}

int Sheaf::getNumSlices()
{
	return m_slices.size();
}

void Sheaf::clear(void)
{
	if(m_slices.size() != 0)
		m_slices.clear();
}

bool Sheaf::isEmpty(void)
{
	if(m_slices.size() == 0)
		return true;
	else
		return false;
}

Bitmap& Sheaf::getBitmapAtLoc(int loc)
{
	//bounds check
	if(loc <0)
		loc = 0;
	if (loc >= m_slices.size())
		loc = m_slices.size() -1 ;

	return m_slices.at(loc);
}

//assignment operator
const Sheaf& Sheaf::operator=(Sheaf& right)
{
	for(int i =0; i<right.getNumSlices();)
	{
		m_slices.push_back(right.getBitmapAtLoc(i) );
		i++;
	}
	return *this;
}

} // namespace FS
