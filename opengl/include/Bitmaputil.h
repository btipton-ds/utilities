//-----------------------------------------------------------------------------
//
// Copyright 1999, SensAble Technologies, Inc.
//
// File: BitmapUtil.h
//
// Created: 8/21/2000
//
//-----------------------------------------------------------------------------

#ifndef BITMAPUTIL_H
#define BITMAPUTIL_H

#include <xapi/Util/Pixel.h>
#include <Kernel/Util/Array2D.h>
#include <Kernel/OpenGLib/OGLExport.h>
#include <Kernel/OpenGLib/glAssert.h>
#include <Kernel/OpenGLib/OglTexture.h>

class CBitmap;

namespace FS
{

//-----------------------------------------------------------------------------
// ColorFormat
//
// CF_RGB and CF_RGBA are compatible with the OpenGL
// formats GL_RGB and GL_RGBA.  Their in-memory byte order is
// R then G then B.
//
// Note that this is "backwards" from Windows BMP's which are
// stored BGR.  Windows uses this "backwards" order because
// it's useful on a little-endian machine (like x86).  On
// little endian, BGRA read in one step as an integer puts
// blue into the low-order byte which is where you'd expect it.
//
// We use the RGB ordering because we want to be able to
// pass bitmaps to OpenGL without converting (and without
// relying on GL_BGR_EXT or GL_BGRA_EXT). 
//-----------------------------------------------------------------------------
enum ColorFormat
{
    CF_UNSPECIFIC = -2,	// Used when requesting that a file read keep
						// the native format of the image file.
    CF_INVALID = -1,
    CF_RGB = 0, // Packed 3-byte per-pixel RGB, same as GL_RGB
    CF_I8,      // 8 bit intensity
    CF_RGBA,    // 4-byte per-pixel RGBA, same as GL_RGBA
    CF_I16,     // 16 bit intensity
    CF_IA8,     // 8 bit intensity + alpha
    CF_IA16,    // 16 bit intensity + alpha

    NUM_COLOR_FORMATS
};

//-----------------------------------------------------------------------------
// RGBColor
//
// For CF_RGB format images.
//-----------------------------------------------------------------------------
struct OGLAPI RGBColor
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

//-----------------------------------------------------------------------------
// RGBAColor
//
// For CF_RGBA format images.
//-----------------------------------------------------------------------------
struct OGLAPI RGBAColor
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

//-----------------------------------------------------------------------------
// I8Color (Grayscale)
//
// For CF_I8 format images.
//-----------------------------------------------------------------------------
typedef unsigned char I8Color;

//-----------------------------------------------------------------------------
// I16Color (Grayscale)
//
// For CF_I16 format images.
//-----------------------------------------------------------------------------
typedef unsigned short I16Color;

//-----------------------------------------------------------------------------
// PixelProcessor
//
// Used by applyPixelProcessor to process each pixel in a bitmap
//-----------------------------------------------------------------------------
struct PixelProcessor
{
    virtual RGBAPixel process(const RGBAPixel &input) = 0;
};


//-----------------------------------------------------------------------------
// Segment
//
//-----------------------------------------------------------------------------
class Segment
{
public:
	Segment(int s, int axis, int sliceAt);

	void SetEnd(int e);

	//returns length
	int ToPoint(int& oX, int& oY);
	int GetLength(){return m_len;}

private:
	int m_changingOnAxis;
	int m_start; 
	int m_end; 
	int m_len; 
	int m_constant;
};

//-----------------------------------------------------------------------------
// Bitmap
//
//-----------------------------------------------------------------------------
class OGLAPI Bitmap : public CTextureLoader
{
public:
    // constructor, destructor
    Bitmap();
    Bitmap(int width, int height, ColorFormat fmt);
	Bitmap(const Bitmap& Bmp);
    virtual ~Bitmap();

	// Move constructor
	Bitmap( Bitmap&& other );
	// Move assignment operator
	Bitmap& operator=( Bitmap&& other );

	Bitmap& operator = (const Bitmap& rhs);

    // initializes a bitmap
    bool init( int width, int height, ColorFormat fmt );
    bool loadBmp( TCHAR const* pFileName, ColorFormat fmt = CF_RGB,
                  bool convertFmt = true );

    // Load bitmap from the given bitmap handle.  This load maintains alpha
    // from the given bitmap handle if it has it and the requested format
    // is CF_RGBA, otherwise the alpha is removed.  If the given bitmap handle
    // doesn't contain an alpha channel but one is requested, it is synthesized
    // (i.e. all opaque).  Formats other than CF_RGBA and CF_RGB are not 
    // supported. 
    bool loadBmp( HBITMAP hBitmap, ColorFormat fmt = CF_RGBA );
    
    bool loadBmp( unsigned int nIDResource, ColorFormat fmt = CF_RGBA );
    bool loadBmp( 
        int widthIn, 
        int heightIn, 
        unsigned char *pBitsIn,   // pointer is to OpenGL bits data
        int colorFormatIn = -1
    );

    bool loadBmpFromActiveRenderBuffer( ColorFormat fmt = CF_RGBA ); // can be screen

	//get the pixel value at corresponding x and y location.
	//set interpolate to get the bi-linear interpolated value.
	double getPixelValueAt(double x, double y, bool interpolate);

    bool clear();
    
    // accessors
    int width() const;
    int height() const;
    int get_type() const; /// equivalent to gl target
    void setGlTypeTarget(int type); //e.g. GL_TEXTURE_2D GL_TEXTURE_RECTANGLE_ARB

    int getSizeBytes() const;
    ColorFormat format() const;
    // This is best used for storage allocation as there is a potential
    // for confusion between RGBA and IA16 - both 4 bytes.
    int depth() const;
    static int depth(int colorFormatIn);
    // Either 1 or 2 (bytes)
    int channelDepth() const;
    static int channelDepth(ColorFormat fmt);
    // 1 - 4
    int channels() const;
    static int channels(ColorFormat fmt);
    int depthInBits() const;
    int bytesPerRow() const;
    unsigned char* bits() const;
    void* getPixel( int x, int y ) const;

    // Format such as GL_RGB, GL_RGBA, GL_LUMINANCE
    GLenum getGLFormat() const;
    static GLenum getGLFormat(ColorFormat fmt);
    // Data type enum such as GL_UNSIGNED_BYTE (for most image types)
    // or GL_UNSIGNED_SHORT (for 16-bit gray scale).
    GLenum getGLDataType() const;
    static GLenum getGLDataType(ColorFormat fmt);
    // OpenGL docs call this the texture "Sized Internal Format" which
    // is used for the 'internalformat' parameter to GL texture calls
    // such as glTexImage2D.
    GLint getGLTextureFormat() const;
    static GLint getGLTextureFormat(ColorFormat fmt);

    // Lookup a pixel from the bitmap
    bool getRGBAPixel( int x, int y, RGBAPixel &result ) const;

    // copies a bitmap, perhaps color converting...
    // (x,y) is the upper left coordinate of where the src will be
    // copied into the destination.
    void blit( const Bitmap* pSrc, int x, int y );
    // Bugzilla 872 Added. Copies and converts as needed to desired format in 
    // generic destination buffer. Size of buffer must be 
    // image width * height * bytes per pixel.
    void copyToBuffer(unsigned char *imageBits, ColorFormat format) const;

    // replace the indicated color and make it transparent
    void alphaKey( const RGBPixel &keyColor,
                   const RGBPixel &replaceColor,
		           int tolerance255 = 0);

    // scale methods
    void scale( double percent );
    void scale( int newWidth, int newHeight );
    void scale( int newWidth, int newHeight,
                const RGBAPixel& color,
                bool isAspectPreserving = true ); 

    // finds the centroid of the bmp
    std::vector<float> findCentroid(float zLevel = 0.0); 

    void makeSizePowerOf2();
    int getSizePowerOf2(int size);
    void deinit();

    // scale the image to the given size
    void resize(int widthIn, int heightIn, Bitmap *target = NULL );
    // If a target/dest image is supplied does not alter the src image.
    void resize(int widthIn, int heightIn, Bitmap *target ) const {
        const_cast<Bitmap *>(this)->resize(widthIn, heightIn, target);
    }

    void halve( Bitmap *target = NULL );
    // If a target/dest image is supplied does not alter the src image.
    void halve( Bitmap *target ) const {
        const_cast<Bitmap *>(this)->halve(target);
    }

    // Can be used to add or remove from the image dimensions. Specify
    // the number of pixels to add or remove in each direction
    void resizeCanvas(int deltaLeft, int deltaRight, int deltaTop, 
                      int deltaBottom, unsigned long fillValue);

	// new corner coordinates
	bool copySection(int x1, int y1, int x2, int y2, Bitmap &bm_cropped);

    // Fills the contents of the image with the value specified. Depending
    // on the bit depth, only some of the value will actually be used. The
    // fill value format follows the ColorFormat spec.
    void fill(unsigned long fillValue);
    void fill(const RGBAPixel &fillColor);

    // Normal save (writes as a .bmp)
    bool saveToFile( const TCHAR *pszFilename ) const;

    // Write to an already open file, using in writing a thumbnail
    // snapshot to the .cly file
    bool writeToFile( FILE *file ) const;

    // Mirror the image about it's center either left to right or top to bottom.
    void mirror(bool leftToRightIn = true);

    // Calls CreateBitmap on the given CBitmap, to initialize the
    // CBitmap with this bitmaps data
    void createCBitmap(CBitmap *pBitmap) const;

    // Replace the intensity of each pixel with an interpolated value
    // between the background and foreground.  Zero intensity (black)
    // becomes background, max intensity (white) is foreground.
    void colorByIntensity(const RGBAPixel &foreground,
                          const RGBAPixel &background);

	void sharpen(int level, RGBAPixel *pIgnoreColor = NULL);
    void blur(int level);
	void blurBlend(int r_bcolor, int g_bcolor, int b_bcolor, int nPasses, bool bBlur, bool bRadialBlend);
    void applyPixelProcessor(PixelProcessor &proc);

    // Module the image by the given [0..1] alpha value.  Combines
    // this given alpha with the bitmaps own alpha (if any).  Only
    // valid for CF_RGBA imagess
    void modulateAlpha(double alpha);

    // Determines the color at a floating-point pixel location
    // using quadBilinear interpolation between the four
    // integer pixels around that location.  x,y should be in
    // the range of [0...width()], [0..height()].  Interpolation
    // will wrap (for tiling images) according to the given wrap
    // flags.
    bool interpolateImage(double x, double y, RGBAPixel &result,
                          bool wrapHoriz = true, bool wrapVert = true) const;

    // Similar to regular image interpolation except it produces a floating
    // point intensity value in the range [0, 1]
    bool interpolateImage(double x, double y, double &intensity,
                          bool wrapHoriz = true, bool wrapVert = true) const;

    // Bugzilla 1437 In some images, like Photoshop, (semi) transparent pixels (alpha != 255)
    // are premultiplied by alpha and blended with white, causing halos when used
    // in other apps like ours.
    void unPremultiplyAlphaWhite();

	void unPremultiplyAlpha();

    bool transparentWhiteToBlack();

    virtual bool load(LPCTSTR filename, bool flipImage=true);   /// non-gl: load file to RAM memory
    virtual void unload();                                      /// non-gl: remove texture data from RAM memory (data may remain bound on the card)   


	// Looks at the image, and considers only pixels that are above threshold.
	// The seed is the geometric average of these points, provided that
	// the seed also is above threshold.
	// If it is not (e.g. non-convex shape), it looks at scanlines passing
	// through the seed, in	the two directions.
	// If it finds a direction where the scanline intersects the shape
	// only in one segment, it returns the center of that segment.
	// Otherwise, it returns the center of the biggest segment.
	// Returns false if something went wrong.
	bool FindSeedPointAboveThreshold(int threshold, int& x, int&y ) const;
	
	void GetMaskAboveThreshold( int threshold, int minX, int maxX, int minY, int maxY, unsigned char* oMask) const; 

	void CropBackgroundColor(const RGBAPixel &backgroundColor);

#ifndef _RELEASE
	void savePPMAscii( const char *filename ) const;
#endif

private:
    virtual bool upload_texture();                              /// moves a loaded texture to the graphics card
    virtual bool is_loaded() const;

    // return true on success
    bool loadImageFromFile(LPCTSTR szFile, ColorFormat fmt);

    int m_GlType;
    int m_width;
    int m_height;
    int m_colorFormat;
    unsigned char* m_pBits;
};

// gluScaleImage with the following restrictions ...
// 1) Doesn't do data type conversions
// 2) Only understands the formats supported by our Bitmap class.
//
// Advantages ...
// 1) gluScaleImage deals with the multiplicity of possible input formats
// by copying into a internal buffer. With large images, it was possible
// for the allocation of the buffer to fail. This version skips all that.
// 2) Fixes array wraparound bugs when upscaling.
//
OGLAPI void FFScaleImage(ColorFormat fmt, 
               int widthin, int heightin, const void *datain,
               int widthout, int heightout, void *dataout);

// Simplistic routine to dump image to ".bmp" file.
// Used for testing during development only.
OGLAPI void dumpImageBMP(Array2D<bool> & image, const _TCHAR *filename);
OGLAPI void dumpImageBMP(Array2D<uchar>& image, const _TCHAR *filename);
OGLAPI void dumpImageBMP(Array2D<int>  & image, const _TCHAR *filename, ColorFormat fmt = CF_RGB);
OGLAPI void dumpImageBMP(Array2D<float>& image, const _TCHAR *filename);
OGLAPI void dumpImageBMP(Array2D<float>& image, const _TCHAR *filename, float min, float max);

// Replace one color in bitmap with another
// only supports 32 bitmaps
OGLAPI void BitmapReplaceColor(HBITMAP hBmp, DWORD orig, DWORD repl, int tolerance255 = 0);

// Replace standard grey (192, 192, 192) with current system equivalent
// (COLOR_3DFACE).  Need to this when loading in 32 bit bitmaps from
// resources in order to get the background colors to map correctly.
OGLAPI void BitmapReplaceSystemGrey(HBITMAP hBmp, bool useColorScheme = true, COLORREF cTransparent = RGB(192, 192, 192), int tolerance255 = 0);

// Replace the intensity of each pixel with an interpolated value
// between the background and foreground.  Zero intensity (black)
// becomes background, max intensity (white) is foreground.
OGLAPI void ColorByIntensity(HBITMAP hBmp, 
                             const RGBAPixel &foreground,
                             const RGBAPixel &background);

//-----------------------------------------------------------------------------
// Sheaf
//
// For adding a vector of bitmaps
//-----------------------------------------------------------------------------
class OGLAPI Sheaf{
public:
	Sheaf();
	~Sheaf();

	//add bmp to sheaf
	void addBitmap(const FS::Bitmap );

	//get bitmap at particular location in the sheaf
	FS::Bitmap& getBitmapAtLoc(int loc);
	
	//get total number of slices
	int getNumSlices(void);

	//clear the sheaf
	void clear();

	//checks if the sheaf is empty
	bool isEmpty(void);

	//assignment operator
	const Sheaf& operator=(Sheaf& );

private:
	std::vector<FS::Bitmap> m_slices;
};

} // end namespace FS

#endif  // BITMAPUTIL_H
