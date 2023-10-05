#pragma once

#include <string>

#ifdef WIN32
#include <Windows.h>
#include <GL/gl.h>

#ifndef __glext_h_
#include <glext.h>
#endif

#ifndef __wglext_h_
//#include <wglext.h>
#endif

#endif

//this macro is for use in COglShader derived classes
#define CHECK_GLSL_STATE   Assert( wglGetCurrentContext() ); GL_ASSERT; if( !isEnabled() || !hasShaderSupport() || m_Error )  return 0
#define HAS_SHADER_SUBROUTINES 1

class COglTexture;
class ActiveTextureUnits;

/** Class that encapsulates glsl shader functionality
This is a base class to store a shader program
A shader program consists of a vertex and a fragment component
Currently this is set up to load shaders from a resource
 */
class COglShader 
{
public:
	COglShader();
	virtual ~COglShader();

	static bool hasShaderSupport();
	static bool hasOGL3Support();
    static bool isEnabled();
    static void enable(bool set=true);
    static std::string getDataDir();

    virtual int shaderResID() { return 0; }

    bool bind();
    bool unBind();
    bool bound() { return m_Bound; }

    void loadDefaultVariables(); ///< Loads defaults (resets to default) variables defined in the shader file

    void setVariablei( LPCTSTR name, int  value );
    void setVariable( LPCTSTR name, float  value );
    void setVariable( LPCTSTR name, col4f& value );
    void setVariable( LPCTSTR name, m44f&  value );
    void setVariable( LPCTSTR name, p4f&   value );
    void setVariable( LPCTSTR name, p3f&   value );
    void setVariable( LPCTSTR name, p2f&   value );

#if HAS_SHADER_SUBROUTINES
	void setVertSubRoutine(const char* name);
	void setFragSubRoutine(const char* name);
#endif

    bool hasTexture ( LPCTSTR textureShaderName);
    bool setTexture ( LPCTSTR textureShaderName, LPCTSTR filename, bool flipImage = true );
    bool setTexture ( LPCTSTR textureShaderName, COglTexture* hwBuffer, bool takeOwnerShip );
    
    COglTexture* getTexture( LPCTSTR textureShaderName );

	const CString getName() const
	{
		return m_Name;
	}

    bool getVariable( LPCTSTR name, CString& type, CString& value );
    bool getVariablei( LPCTSTR name, int& value );
    bool getVariable( LPCTSTR name, float& value );
    bool getVariable( LPCTSTR name, col4f& value );
    bool getVariable( LPCTSTR name, m44f&  value );
    bool getVariable( LPCTSTR name, p4f&   value );
    bool getVariable( LPCTSTR name, p3f&   value );
    bool getVariable( LPCTSTR name, p2f&   value );

    void setGeomShaderIOtype( int intype, int outtype);
	bool load();

protected: 
	virtual void initUniform() = 0;
    char* loadShaderFromResource( int shaderID );
    bool unLoad();

    bool hasShaderError (int shaderID);
	bool hasProgramError(int programID);

    bool    m_Error;
    CString m_Log, m_Name;

private:
    friend UINT __cdecl LoadShaderBackgroundThread( LPVOID pParam );

    virtual TCHAR* getResourceLibrary()     = 0; /// module where the shader resource exists

    virtual char* getShaderIncludeSource()  = 0;
    virtual char* getVertexShaderSource()   = 0;
    virtual char* getFragmentShaderSource() = 0;
    virtual char* getGeometryShaderSource() = 0;    

    virtual int& programID() = 0;
    virtual int& vertexID()  = 0;
    virtual int& fragmentID() = 0;
    virtual int& geometryID() = 0;

    int m_GeomShaderInType;  /// if we have a geometry shader we're required to setup the in and out type here
    int m_GeomShaderOutType; /// if we have a geometry shader we're required to setup the in and out type here

    void*  m_TextureMap;  /// map of images keyed to shader inputs
    void*  m_ArgumentMap; /// map of arguments matched to shader inputs

    ActiveTextureUnits* m_TextureUnitStates; /// texture units bound to the card, primary use is gl state management

    void   clearTextures();

    static bool loadGlFuncPointers();
    static bool mEnabled;

    bool m_DefaultsLoaded;
    bool m_Bound;

public:
    static PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
	static PFNGLGETSHADERIVPROC				glGetShaderiv;
    static PFNGLGETINFOLOGARBPROC           glGetInfoLogARB;
	static PFNGLGETPROGRAMIVPROC			glGetProgramiv;
	static PFNGLGETPROGRAMINFOLOGPROC		glGetProgramInfoLog;
    static PFNGLCREATEPROGRAMOBJECTARBPROC  glCreateProgramObjectARB;
    static PFNGLCREATESHADEROBJECTARBPROC   glCreateShaderObjectARB;
    static PFNGLSHADERSOURCEARBPROC         glShaderSourceARB;
    static PFNGLCOMPILESHADERARBPROC        glCompileShaderARB;
    static PFNGLATTACHOBJECTARBPROC         glAttachObjectARB;
    static PFNGLLINKPROGRAMARBPROC          glLinkProgramARB;
    static PFNGLUSEPROGRAMOBJECTARBPROC     glUseProgramObjectARB;
    static PFNGLGETUNIFORMLOCATIONARBPROC   glGetUniformLocationARB;
    static PFNGLUNIFORM1IARBPROC            glUniform1iARB;
    static PFNGLUNIFORM1FARBPROC            glUniform1fARB;
    static PFNGLUNIFORM2FARBPROC            glUniform2fARB;
    static PFNGLUNIFORM3FARBPROC            glUniform3fARB;
    static PFNGLUNIFORM4FARBPROC            glUniform4fARB;
    static PFNGLUNIFORMMATRIX4FVARBPROC     glUniformMatrix4fv;
    static PFNGLPROGRAMPARAMETERIEXTPROC    glProgramParameteriEXT;
	static PFNGLACTIVETEXTUREPROC			glActiveTexture;
	static PFNGLTEXIMAGE3DEXTPROC			glTexImage3D;
#if HAS_SHADER_SUBROUTINES
	typedef GLuint (APIENTRYP PFNGLGETSUBROUTINEINDEXPROC) (GLuint program, GLenum shadertype, const GLchar *name);
	typedef void (APIENTRYP PFNGLUNIFORMSUBROUTINESUIVPROC) (GLenum shadertype, GLsizei count, const GLuint *indices);
	static PFNGLGETSUBROUTINEINDEXPROC			glGetSubroutineIndex;
	static PFNGLUNIFORMSUBROUTINESUIVPROC			glUniformSubroutinesuiv;
#endif
};

class OGLAPI COglArg
{
public:
    enum argtype { eInt, eFloat, eFloat2, eFloat3, eFloat4, eFloat16 };

    COglArg(int val);
    COglArg(float val);
    COglArg(float* val, int numFloats=3);
    virtual ~COglArg();

    int    getInt();
    float  getFloat();
    float  getFloatAt(int idx); 
    float* getFloatPtr(); 

    void  set(int val);
    void  set(float val);
    void  set(float* val); ///num floats determined by type

    argtype getType() { return type; }

private:
    void allocFloat(float* val, int numFloats);
    argtype type;

    union
    {
        int    ival;
        float  fval;
        float* fvalptr;
    };
};

// two macros to help roll resource shaders into their own class wrappers
// macro for header
#define DERIVED_OGLSHADER(name,str,exportapi) \
class exportapi name : public COglShader \
{                                    \
public:                              \
	name()  { m_Name = str; }        \
    virtual ~name() {}               \
protected:							 \
	virtual void initUniform();		 \
private:                             \
    char* getShaderIncludeSource();  \
    char* getVertexShaderSource();   \
    char* getFragmentShaderSource(); \
    char* getGeometryShaderSource(); \
    int shaderResID();               \
                                     \
    TCHAR* getResourceLibrary(){ return mLibrary; } \
    int& programID()   { return m_programID; }    \
    int& vertexID()    { return m_vertexID;  }    \
    int& fragmentID()  { return m_framentID; }    \
    int& geometryID()  { return m_geometryID; }   \
                                     \
    static TCHAR* mLibrary;          \
    static char* isource;            \
    static char* vsource;            \
    static char* fsource;            \
    static char* gsource;            \
    static int m_programID;          \
    static int m_vertexID;           \
    static int m_framentID;          \
    static int m_geometryID;         \
};

// macro for cpp
#define DERIVED_IMP_OGLSHADER(name, lib, residVert, residFrag, residInclude)   \
TCHAR* name::mLibrary = lib;                                \
int name::m_programID = 0;                                  \
int name::m_vertexID  = 0;                                  \
int name::m_framentID = 0;                                  \
int name::m_geometryID = 0;                                 \
char* name::isource   = 0;                                  \
char* name::vsource   = 0;                                  \
char* name::fsource   = 0;                                  \
char* name::gsource   = 0;                                  \
char* name::getVertexShaderSource() {                       \
    static bool first = true;                               \
    if(!first) return vsource;                              \
    first = false;                                          \
    vsource = loadShaderFromResource(residVert);            \
    if(m_Error) AfxMessageBox(m_Log);                       \
    return vsource; }                                       \
char* name::getFragmentShaderSource(){                      \
    static bool first = true;                               \
    if(!first) return fsource;                              \
    first = false;                                          \
    fsource = loadShaderFromResource(residFrag);            \
    if(m_Error) AfxMessageBox(m_Log);                       \
    return fsource;}                                        \
char* name::getGeometryShaderSource()    { return 0;}       \
char* name::getShaderIncludeSource(){                             \
    static bool first = true;                               \
    if(!first) return isource;                              \
    first = false;                                          \
    isource = residInclude ? loadShaderFromResource(residInclude) : 0;         \
    if(m_Error) AfxMessageBox(m_Log);                       \
    return isource;}                                        \
int name::shaderResID() { return residFrag; }

// macro for cpp
#define DERIVED_IMP_OGLGEOMSHADER(name, lib, residVert, residFrag, residGeom, residInclude)   \
TCHAR* name::mLibrary = lib;                                \
int name::m_programID = 0;                                  \
int name::m_vertexID  = 0;                                  \
int name::m_framentID = 0;                                  \
int name::m_geometryID = 0;                                 \
char* name::isource   = 0;                                  \
char* name::vsource   = 0;                                  \
char* name::fsource   = 0;                                  \
char* name::gsource   = 0;                                  \
char* name::getVertexShaderSource() {                       \
    static bool first = true;                               \
    if(!first) return vsource;                              \
    first = false;                                          \
    vsource = loadShaderFromResource(residVert);            \
    if(m_Error) AfxMessageBox(m_Log);                       \
    return vsource; }                                       \
char* name::getFragmentShaderSource(){                      \
    static bool first = true;                               \
    if(!first) return fsource;                              \
    first = false;                                          \
    fsource = loadShaderFromResource(residFrag);            \
    if(m_Error) AfxMessageBox(m_Log);                       \
    return fsource;}                                        \
char* name::getGeometryShaderSource(){                      \
    static bool first = true;                               \
    if(!first) return gsource;                              \
    first = false;                                          \
    gsource = loadShaderFromResource(residGeom);            \
    if(m_Error) AfxMessageBox(m_Log);                       \
    return gsource;}                                        \
char* name::getShaderIncludeSource(){                             \
    static bool first = true;                               \
    if(!first) return isource;                              \
    first = false;                                          \
    isource = residInclude ? loadShaderFromResource(residInclude) : 0;         \
    if(m_Error) AfxMessageBox(m_Log);                       \
    return isource;}                                        \
int name::shaderResID() { return residFrag; }
