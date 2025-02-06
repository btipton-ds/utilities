#pragma once

#include <string>
#include <map>
#include <memory>

#ifdef WIN32
#include <Windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>

#include <OGLExtensions.h>
#define CHECK_GLSL_STATE   assert( wglGetCurrentContext() ); GL_ASSERT; if( !isEnabled() || m_error )  return 0
#else

#define __cdecl
#define UINT unsigned int
#define LPVOID void*
#define CHECK_GLSL_STATE
#include "/usr/include/GL/gl.h"
#include "/usr/include/GL/glext.h"
#endif

#include <OGLCol4f.h>

#ifndef GL_ASSERT
#ifdef _DEBUG
#define GL_ASSERT ::OGL::ShaderBase::dumpGlErrors(__FILE__, __LINE__);
#else
#define GL_ASSERT
#endif // _DEBUG
#endif // !GL_ASSERT


namespace OGL
{
    class Arg;
    class Texture;
    class ActiveTextureUnits;

#define TEXTURE_SUPPORT 0

    /** Class that encapsulates glsl shader functionality
    This is a base class to store a shader program
    A shader program consists of a vertex and a fragment component
    Currently this is set up to load shaders from a resource
     */
#ifdef WIN32
    class ShaderBase : public Extensions
#else
    class ShaderBase
#endif
    {
    public:
        using ArgMapType = std::map<const std::string, std::shared_ptr<Arg>>;

        static void dumpGlErrors(const char* filename, int lineNumber);

#if TEXTURE_SUPPORT
        using TextureMapType = std::map<const std::string, std::shared_ptr<Texture>>;
#endif

        ShaderBase();
        virtual ~ShaderBase();

        static bool isEnabled();
        static void enable(bool set = true);
        static std::string getDataDir();

        virtual int shaderResID() { return 0; }
        void setShaderVertexAttribName(const std::string& name);
        void setShaderNormalAttribName(const std::string& name);
        void setShaderTexParamAttribName(const std::string& name);
        void setShaderColorAttribName(const std::string& name);


        bool bind(); // Sets uniforms on bind
        bool unBind();
        bool bound() const;

        void loadDefaultVariables(); ///< Loads defaults (resets to default) variables defined in the shader file

        void setVariablei(const std::string& name, int  value);
        void setVariable(const std::string& name, float  value);
        void setVariable(const std::string& name, const col4f& value);
        void setVariable(const std::string& name, const m44f& value);
        void setVariable(const std::string& name, const p4f& value);
        void setVariable(const std::string& name, const p3f& value);
        void setVariable(const std::string& name, const p2f& value);

#if HAS_SHADER_SUBROUTINES
        void setVertSubRoutine(const char* name);
        void setFragSubRoutine(const char* name);
#endif

        bool hasTexture(const std::string& textureShaderName) const;
        bool setTexture(const std::string& textureShaderName, const std::string& filename, bool flipImage = true);
        bool setTexture(const std::string& textureShaderName, Texture* hwBuffer, bool takeOwnerShip);

        Texture* getTexture(const std::string& textureShaderName);

        const std::string getName() const;

        bool getVariable(const std::string& name, std::string& type, std::string& value) const;
        bool getVariablei(const std::string& name, int& value) const;
        bool getVariable(const std::string& name, float& value) const;
        bool getVariable(const std::string& name, col4f& value) const;
        bool getVariable(const std::string& name, m44f& value) const;
        bool getVariable(const std::string& name, p4f& value) const;
        bool getVariable(const std::string& name, p3f& value) const;
        bool getVariable(const std::string& name, p2f& value) const;

        void setGeomShaderIOtype(int intype, int outtype);
        bool load();

        int programID() const;
        int vertexID() const;
        int fragmentID() const;
        int geometryID() const;

        GLuint getVertexLoc() const;
        GLuint getNormalLoc() const;
        GLuint getTexParamLoc() const;
        GLuint getColorLoc() const;

    protected:
        virtual void initUniform() = 0;
        bool unLoad();

        bool hasShaderError(int shaderID) const;
        bool hasProgramError(int programID) const;

        bool    m_error;
        std::string m_log, m_name;

    private:
        friend UINT __cdecl LoadShaderBackgroundThread(LPVOID pParam);

        virtual const char* getShaderIncludeSource() const = 0;
        virtual const char* getVertexShaderSource() const = 0;
        virtual const char* getFragmentShaderSource() const = 0;
        virtual const char* getGeometryShaderSource() const = 0;

        int m_geomShaderInType;  /// if we have a geometry shader we're required to setup the in and out type here
        int m_geomShaderOutType; /// if we have a geometry shader we're required to setup the in and out type here

#if TEXTURE_SUPPORT
        TextureMapType  m_TextureMap;  /// map of images keyed to shader inputs
#endif
        void   clearTextures();

        static bool mEnabled;
        bool m_defaultsLoaded;
        bool m_bound;

        GLuint m_vertLoc = -1, m_normLoc = -1, m_texParamLoc = -1, m_colorLoc = -1;
        std::string _vertAttribName, _normalAttribName, _texParamAttribName, _colorAttribName;

        int _programId = 0, _vertexId = 0, _fragmentId = 0, _geometryId = 0;

        ArgMapType  m_argumentMap; /// map of arguments matched to shader inputs

        ActiveTextureUnits* m_textureUnitStates; /// texture units bound to the card, primary use is gl state management
    };

    inline bool ShaderBase::bound() const {
        return m_bound;
    }

    inline const std::string ShaderBase::getName() const
    {
        return m_name;
    }

    inline void ShaderBase::setShaderVertexAttribName(const std::string& name)
    {
        _vertAttribName = name;
    }

    inline void ShaderBase::setShaderNormalAttribName(const std::string& name)
    {
        _normalAttribName = name;;
    }

    inline void ShaderBase::setShaderTexParamAttribName(const std::string& name)
    {
        _texParamAttribName = name;
    }

    inline void ShaderBase::setShaderColorAttribName(const std::string& name)
    {
        _colorAttribName = name;
    }

    inline GLuint ShaderBase::getVertexLoc() const
    {
        return m_vertLoc;
    }

    inline GLuint ShaderBase::getNormalLoc() const
    {
        return m_normLoc;
    }

    inline GLuint ShaderBase::getTexParamLoc() const
    {
        return m_texParamLoc;
    }

    inline GLuint ShaderBase::getColorLoc() const
    {
        return m_colorLoc;
    }

    class Shader : public ShaderBase
    {
    public:
        void setIncludeSrcFile(const std::string& filename);
        void setVertexSrcFile(const std::string& filename);
        void setFragmentSrcFile(const std::string& filename);
        void setGeometrySrcFile(const std::string& filename);

        void setIncludeSrc(const std::string& src);
        void setVertexSrc(const std::string& src);
        void setFragmentSrc(const std::string& src);
        void setGeometrySrc(const std::string& src);

        void initUniform() override;

        const char* getShaderIncludeSource() const override;
        const char* getVertexShaderSource() const override;
        const char* getFragmentShaderSource() const override;
        const char* getGeometryShaderSource() const override;
    private:
        std::string
            _shaderIncSrc,
            _vertSrc,
            _fragSrc,
            _geomSrc;
    };

    class Arg
    {
    public:
        enum argtype { eInt, eFloat, eFloat2, eFloat3, eFloat4, eFloat16 };

        Arg(int val);
        Arg(float val);
        Arg(const float* val, int numFloats = 3);
        virtual ~Arg();

        int    getInt();
        float  getFloat();
        float  getFloatAt(int idx);
        float* getFloatPtr();

        void  set(int val);
        void  set(float val);
        void  set(const float* val); ///num floats determined by type

        argtype getType() { return type; }

    private:
        void allocFloat(const float* val, int numFloats);
        argtype type;

        union
        {
            int    ival;
            float  fval;
            float  fvalArr[16]; // Size of a 4x4 matrix
        };
    };

    // two macros to help roll resource shaders into their own class wrappers
    // macro for header
#define DERIVED_OGLSHADER(name,str,exportapi) \
class exportapi name : public Shader \
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
}