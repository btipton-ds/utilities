// OglShader.cpp: implementation of the COglMesh class.
//
//////////////////////////////////////////////////////////////////////


#ifdef WIN32
#include <Windows.h>
#include <gl/GL.h>
#else
#define GL_GLEXT_PROTOTYPES
#include </usr/include/GL/gl.h>
#include </usr/include/GL/glext.h>
#endif

//#include <Kernel/OpenGLib/glu.h> // LIMBO not picking up from the precompiled header in 64 bit
#include <string.h> 
#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <OGLShader.h>

using namespace std;

#define	RETURN_IF_NOT_FIRST_TIME(a) static bool first=true; if(!first) return a; first = false;

void COglShaderBase::dumpGlErrors(const char* filename, int lineNumber)
{
    GLenum err;
    while ((err = glGetError()) && err != GL_NO_ERROR) {
    	cout << "glErr (" << filename << ":" << lineNumber << "): ";
        switch (err) {
        case GL_INVALID_ENUM:
            cout << "GL_INVALID_ENUM\n"; break;
        case GL_INVALID_VALUE:
            cout << "GL_INVALID_VALUE\n"; break;
        case GL_INVALID_OPERATION:
            cout << "GL_INVALID_OPERATION\n"; break;
        case GL_STACK_OVERFLOW:
            cout << "GL_STACK_OVERFLOW\n"; break;
        case GL_STACK_UNDERFLOW:
            cout << "GL_STACK_UNDERFLOW\n"; break;
        case GL_OUT_OF_MEMORY:
            cout << "GL_OUT_OF_MEMORY\n"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            cout << "GL_INVALID_FRAMEBUFFER_OPERATION\n"; break;
        case GL_CONTEXT_LOST:
            cout << "GL_CONTEXT_LOST\n"; break;
        case GL_TABLE_TOO_LARGE:
            cout << "GL_TABLE_TOO_LARGE\n"; break;
        default:
            cout << " Unknown err(" << err << ")\n"; break;
        }

        assert(!"glError");
    }
}

#define HDTIMELOG(X)
//#define CHECK_GLSL_STATE
#define GL_IGNORE_ERROR

//#define GLSL_VERBOSE

#ifdef OPENGLIB_EXPORTS //meaning we're building this as part of the FF OpenGLib.dll
#include <Kernel/OpenGLib/BitmapUtil.h>
#include <Kernel/WinWrapper/IGUIWrapper.h>
#include <Kernel/Config/PreferencesUtil.h>
#include <Kernel/MemoryManager/ApplicationMemoryApi.h>

// Avoid compiler warnings about the difference between
// size_t (unsigned int64) and unsigned int
#ifndef COUNTOF
#define COUNTOF( array ) ((unsigned int)(_countof( array )))
#endif

using namespace FS;

bool getPrimitiveOglModeDefault()
{
	//if (ApplicationMemoryStateGetRunningName() == ST_RUNNING_APPLICATION_NAME_VIEWER) // VIEWER_FIXES
	//	return true;

	return false;
}

ADD_PREF_SERIALIZED(RunPrimitiveOglMode, (bool)getPrimitiveOglModeDefault());
ADD_PREF_SERIALIZED(HighQualityEnvFileName, StringT(_T("Patterns\\seaworld.dds")))

#endif


#define assert_variable_not_found //assert(!"variable not found")

COglArg::COglArg(int val)
{
    type = eInt;
    ival = val;
}

COglArg::COglArg(float val)
{
    type = eFloat;
    fval = val;
}

COglArg::COglArg(const float* val, int numFloats)
{
    switch(numFloats)
    {
    case 1 : type = eFloat ;  fval = *val;                break;
    case 2 : type = eFloat2;  allocFloat(val,numFloats);  break;
    case 3 : type = eFloat3;  allocFloat(val,numFloats);  break;
    case 4 : type = eFloat4;  allocFloat(val,numFloats);  break;
    case 16: type = eFloat16; allocFloat(val,numFloats);  break;
    default: assert(0); 
    }
}

void  COglArg::set(int val)
{
    if( type != eInt )
    {
        assert(!"type error");
        return;
    }
    ival = val;
}

void  COglArg::set(float val)
{
    if( type != eFloat )
    {
        assert(!"type error");
        return;
    }
    fval = val;
}

void  COglArg::set(const float* val)
{
    switch(type)
    {
    case eFloat :  fval = *val;                            break;
    case eFloat2:  memcpy( fvalArr, val, sizeof(float)*2) ; break;
    case eFloat3:  memcpy( fvalArr, val, sizeof(float)*3) ; break;
    case eFloat4:  memcpy( fvalArr, val, sizeof(float)*4) ; break;
    case eFloat16: memcpy( fvalArr, val, sizeof(float)*16); break;
    default: assert(0);
    }
}

int COglArg::getInt()
{
    if( type != eInt )
    {
        assert(0);
        return 0;
    }
    return ival;
}

float COglArg::getFloat()
{
    if( type != eFloat )
    {
        assert(0);
        return 0.0f;
    }
    return fval;
}

float* COglArg::getFloatPtr()
{
    if( type < eFloat2 || type > eFloat16 )
    {
        assert(0);
        return 0;
    }

    return fvalArr;
}

float COglArg::getFloatAt(int idx)
{
    switch( type )
    {
    case eFloat2:  if( idx >= 0 && idx < 2 ) return fvalArr[idx]; else { assert(!"range error"); }  break;
    case eFloat3:  if( idx >= 0 && idx < 3 ) return fvalArr[idx]; else { assert(!"range error"); }  break;
    case eFloat4:  if( idx >= 0 && idx < 4 ) return fvalArr[idx]; else { assert(!"range error"); }  break;
    case eFloat16: if( idx >= 0 && idx < 16) return fvalArr[idx]; else { assert(!"range error"); }  break;
    default: assert(0); 
    } 

    return 0.0f;
}

void COglArg::allocFloat(const float* val, int numFloats)
{
    assert(numFloats <= 4);
    memcpy( fvalArr, val, sizeof(float)*numFloats);
}

COglArg::~COglArg()
{
}


bool COglShaderBase::mEnabled = true; //user override of global shader usage

#ifdef WIN32
COglShaderBase::COglShaderBase()
    : COglExtensions()
    , m_error(false)
	, m_textureUnitStates(0)
    , m_defaultsLoaded(false)
    , m_bound(false)
    , m_geomShaderInType(GL_TRIANGLES)
    , m_geomShaderOutType(GL_TRIANGLES)
{
}
#else
COglShaderBase::COglShaderBase()
    : m_error(false)
	, m_textureUnitStates(0)
    , m_defaultsLoaded(false)
    , m_bound(false)
    , m_geomShaderInType(GL_TRIANGLES)
    , m_geomShaderOutType(GL_TRIANGLES)
{
}
#endif

COglShaderBase::~COglShaderBase()
{
    HLOG(_T("~COglShaderBase()"));
#if TEXTURE_SUPPORT
    clearTextures();
#endif
}

bool COglShaderBase::isEnabled()
{
    return mEnabled;
}

void COglShaderBase::enable(bool set)
{
    mEnabled = set;
}

void COglShaderBase::setVariablei( const string& name, int  value )
{
    if( !m_defaultsLoaded ) 
        loadDefaultVariables();

    auto it = m_argumentMap.find( name );
    if( it == m_argumentMap.end() )
        m_argumentMap.insert(make_pair(name, make_shared<COglArg>(value)));
    else
        it->second->set(value);

    if(m_bound)
    {
		int progId = programID();
		int dbg = glGetUniformLocationARB(progId, "DepthPeel");
        int argLocation = glGetUniformLocationARB( progId, name.c_str()); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform1i( argLocation, value ); GL_ASSERT;
        }
    }
}

void COglShaderBase::setVariable( const string& name, float  value )
{
    if( !m_defaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = m_argumentMap.find( name );
    if( it == m_argumentMap.end() )
        m_argumentMap.insert( ArgMapType::value_type( name, new COglArg( value ) ) );
    else
        it->second->set(value);

    if(m_bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), name.c_str()); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform1f( argLocation, value ); GL_ASSERT;
        }
    }
}

void COglShaderBase::setVariable( const string& name, const col4f& value )
{
    if( !m_defaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = m_argumentMap.find( name );
    if( it == m_argumentMap.end() )
        m_argumentMap.insert( ArgMapType::value_type( name, new COglArg( value, 4 ) ) );
    else
        it->second->set(value);

    if(m_bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), name.c_str()); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform4fv(argLocation, 1, value); GL_ASSERT;
        }
    }
}

void COglShaderBase::setVariable( const string& name, const m44f&  value )
{
    if( !m_defaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = m_argumentMap.find( name );
    if( it == m_argumentMap.end() )
        m_argumentMap.insert( ArgMapType::value_type( name, new COglArg( value.transposef(), 16 ) ) );
    else
        it->second->set( value.transposef() );

    if(m_bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), name.c_str()); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniformMatrix4fv( argLocation, 1, false, value.transposef() );  GL_ASSERT;
        }
    }

}

void COglShaderBase::setVariable( const string& name, const p4f&   value )
{
    if( !m_defaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = m_argumentMap.find( name );
    if( it == m_argumentMap.end() )
        m_argumentMap.insert( ArgMapType::value_type( name, new COglArg( value, 4 ) ) );
    else
        it->second->set(value);

    if(m_bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), name.c_str()); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform4fv(argLocation, 1, value); GL_ASSERT;
        }
    }
}

void COglShaderBase::setVariable( const string& name, const p3f&   value )
{
    if( !m_defaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = m_argumentMap.find( name );
    if( it == m_argumentMap.end() )
        m_argumentMap.insert( ArgMapType::value_type( name, new COglArg( value, 3 ) ) );
    else
        it->second->set(value);

    if(m_bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), name.c_str()); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform3fv(argLocation, 1, (const float*)value); GL_ASSERT;
        }
    }
}

void COglShaderBase::setVariable( const string& name, const p2f& value )
{
    if( !m_defaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = m_argumentMap.find( name );
    if( it == m_argumentMap.end() )
        m_argumentMap.insert( ArgMapType::value_type( name, new COglArg( value, 2 ) ) );
    else
        it->second->set(value);

    if(m_bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), name.c_str()); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform2fv(argLocation, 1, value); GL_ASSERT;
        }
    }
}

#if HAS_SHADER_SUBROUTINES
void COglShaderBase::setFragSubRoutine(const char* name)
{
	GLuint subIdx = glGetSubroutineIndex(programID(), GL_FRAGMENT_SHADER, name);
	if (subIdx >= 0) {
		glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subIdx);
	}
}

void COglShaderBase::setVertSubRoutine(const char* name)
{
	GLuint subIdx = glGetSubroutineIndex(programID(), GL_VERTEX_SHADER, name);
	if (subIdx >= 0) {
		glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &subIdx);
	}
}
#endif

void   clearTextures();
void COglShaderBase::clearTextures()
{
#if TEXTURE_SUPPORT
    m_TextureMap.clear();
#endif
}

bool COglShaderBase::setTexture( const string& textureShaderName, const string& filename, bool flipImage )
{
#if TEXTURE_SUPPORT
    HLOG(Format(_T("COglShaderBase::setTexture %s %s"),textureShaderName,filename));

    auto iter = m_TextureMap.find( textureShaderName );
    if(iter != m_TextureMap.end() )
    {
        if(iter->second && (iter->second->mOwner == this) )
        {
            HLOG(Format(_T("deleting texture %s, id %d in in shader"),textureShaderName,map->second->m_hwID));
            iter->second = 0;
        }
    }

	if( filename.empty() )
	{
		m_TextureMap.insert( TextureMapType::value_type( textureShaderName, (COglTexture *)0 ) );
		return true;
	}

    bool dds=false;
    {
        //LIMBO use check extension utility
        string name( filename );
//        name = name.MakeLower();
        dds = name.find(".dds") > 0;
    }
#if 0
    CTextureLoader* img = 0;
    if(dds)
        img = new CDDSImage;

#ifdef OPENGLIB_EXPORTS
    else
        img = new Bitmap;
#endif

    if( !img || !img->load( filename, flipImage) )
    {
        delete img;
        return false;
    }

    img->mOwner = this; // this shader will delete this texture

    m_TextureMap.insert( TextureMapType::value_type( textureShaderName, img ) );
#endif
#endif
    return true;
}

bool COglShaderBase::hasTexture ( const string& textureShaderName) const
{
#if TEXTURE_SUPPORT
    TextureMapType::iterator target = m_TextureMap.find( textureShaderName );
    return target != m_TextureMap.end();
#endif
    return false;
}

bool COglShaderBase::setTexture ( const string& textureShaderName, COglTexture* texture, bool takeOwnerShip )
{
#if TEXTURE_SUPPORT
    // HLOG(Format(_T("COglShaderBase::setTexture %s %s"),textureShaderName, takeOwnerShip ? _T("shader will delete") : _T("shader will NOT delete")));

    if( !m_defaultsLoaded ) 
        loadDefaultVariables();

    if( !texture )
    {
        TextureMapType::iterator target = m_TextureMap.find( textureShaderName );
        if( target != m_TextureMap.end() )
        {
            if( target->second && (target->second->mOwner == this ))
            {
                HLOG(Format(_T("deleting texture %s, id %d in in shader"),textureShaderName,target->second->m_hwID));
                delete target->second;
				target->second = 0;
            }
        }

		m_TextureMap.insert( TextureMapType::value_type( textureShaderName, (COglTexture *)0 ) );
    }
    else
    {
        TextureMapType::iterator target = m_TextureMap.find( textureShaderName );
        if( target != m_TextureMap.end() )
        {
            if( target->second && (target->second->mOwner == this) )
            {
                HLOG(Format(_T("deleting texture %s, id %d in in shader"),textureShaderName,target->second->m_hwID));
                delete target->second;
            }

            target->second = texture;
        }
        else
        {
            if( takeOwnerShip )
            {
                assert( !texture->mOwner );
                texture->mOwner = this; // this shader will delete this texture
            }

            m_TextureMap.insert( TextureMapType::value_type( textureShaderName, texture ) );
        }
    }

    return true;
#else
    return false;
#endif
}

COglTexture* COglShaderBase::getTexture( const string& textureShaderName )
{
#if TEXTURE_SUPPORT
    if( !m_defaultsLoaded )
        loadDefaultVariables();

    TextureMapType::iterator target = m_TextureMap.find( textureShaderName );
    assert(target != m_TextureMap.end() && "shader not initialized or texture name incorrect");

    if( target != m_TextureMap.end() )
        return target->second;     
#endif
    return 0;
}

#define GET_EXT_POINTER(name, type) name = (type)wglGetProcAddress(#name)

namespace
{

    bool checkOglExtension(const char* name)
    {
#ifdef WIN32
        static GLint numExt;
        static set<string> names;
        if (numExt == 0) {

            glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
            for (GLint i = 0; i < numExt; i++) {
                const GLubyte* pName = COglShaderBase::glGetStringi(GL_EXTENSIONS, i);
            }
        }
#endif
	return true;
    }
}

bool COglShaderBase::hasShaderError(int obj) const
{
#ifdef GLSL_VERBOSE
    int infologLength = 0;
	glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

	if( infologLength > 0 )
	{
		int charsWritten  = 0;
		char* err = new char[infologLength];
	 
		glGetInfoLogARB( obj, infologLength, &charsWritten, err );
		err[charsWritten] = '\0';

		string wErr = string(err);

//		cout << err << endl;
		HLOG( Format( _T("compiling shader \"%s\":\n%s\n\n"),(const string&)m_Name, wErr));
		delete [] err;
	}
#endif
	return false;
}

bool COglShaderBase::hasProgramError(int obj) const
{
#ifdef GLSL_VERBOSE
    int infologLength = 0;	
	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

	if( infologLength > 0 )
	{
		int charsWritten  = 0;
		char* err = new char[infologLength];
		glGetProgramInfoLog(obj, infologLength, &charsWritten, err);
		err[charsWritten] = '\0';

		string wErr = string(err);
		HLOG( Format( _T("linking shader \"%s\":\n%s"),(const string&)m_Name, wErr) );

		delete [] err;
	}
#endif
	return false;
}

string COglShaderBase::getDataDir()
{
#ifdef OPENGLIB_EXPORTS
    return getGUIWrapper()->getExeDirectory().getBuffer();
#else
    return "";
#endif
}


//assumes an array of strings delimited with multiple newlines
static char* getNextLine(char* str, char* max)
{
    assert(str && str < max);
    if(!str || (str >= max))
        return 0;

    size_t offset = strlen(str);

    // in case we are on a line of no length
    // skip multiple nulls (which came from previous multiple endlines \r\n etc.)
    while( offset < 1 && ++str < max)
        offset = strlen( str );

    // if the end of this line is a null move forward
    // until it isn't... that will give us the next line
    while( !*(str + offset) && (++str + offset) < max );
        
    return (str + offset) >= max ? 0 : str + offset;
}

//assumes an array of strings delimited with multiple newlines
static char* getVariableName(char* str)
{
    assert(str && strlen(str));
    const unsigned int bufsize = 24;
    static char name[256];
    name[0] = '\0';
    sscanf(str, "%s", name);
    
    size_t sz = strlen(name);
    if(name[sz-1]==';')
        name[sz-1] = '\0';

    return name;
}

static bool doesFileExist( const string& pathFile )
{
	if (pathFile.empty())
		return false;

    bool ok = filesystem::exists(pathFile);

	return ok;
}

static string getCheckedImagePath(const string& str)
{
    string fname = COglShaderBase::getDataDir();

    if( !fname.empty() )
    {
        fname += string( str );

        if( !doesFileExist(fname) )
        {
            assert(!"default shader image file not found");
            fname = "";
        }
    }
    return fname;
}

void COglShaderBase::loadDefaultVariables()
{
    static bool firstCall = true;
    if (!firstCall)
        return;
    firstCall = false;
    // LIMBO need to add vertex shader parsing? 
    // LIMBO Do we need more info in the header?
    // LIMBO Should set up a static texture cache for default textures
    // so we only have one default instance of each texture on the card
    for(int i=0; i<3; i++)
    {
        const char* orgSource = 0;
        
        switch(i)
        {
        case 0: orgSource = getVertexShaderSource();   break;
        case 1: orgSource = getGeometryShaderSource(); break;
        case 2: orgSource = getFragmentShaderSource(); break;
        };
        
        if(!orgSource || !strlen(orgSource)) 
            continue;

        size_t end = strlen(orgSource);
        char* sourceCpy = new char[ end + 1 ];
        strcpy(sourceCpy, orgSource);
        sourceCpy[end] = '\0';
        char* source  = sourceCpy; // don't change the original source
        char* endChar = &source[end];

	    //make all newlines endlines
	    for(int i=0; i<end; i++)
		    if(source[i] == '\n' || source[i] == '\r')
			    source[i] = '\0';

        do
        {
            // check for uniform variable declaration containing the STI //* comment tag
			bool stiDefaultVariable = strstr( source, "//*") ? true : false;
            int inIdx = strncmp(source, "in", 2);
            int uniformIdx = strncmp(source, "uniform", 7);
            if( (uniformIdx != -1) || (inIdx != -1))
            {
                char* tname  = 0;
				char* stiArg = 0;

                string vname;

				if( stiDefaultVariable )
				{
					stiArg = strstr( source, "//*" ) + 3;

					while( stiArg[0] == ' ' || stiArg[0] == '\t' )    
						stiArg++;
				}

                if (tname = strstr( source, "float" ))
                {
                    vname = getVariableName( tname + 5 );
                    if (stiDefaultVariable)
                        setVariable( vname, (float)atof( stiArg ));
                    else
                        setVariable(vname, 0);
                }
                else if (tname = strstr( source, "vec2" ))
                {
                    vname = getVariableName( tname + 4 );
                    p2f val;
                    if (stiDefaultVariable) {
                        int num = sscanf(stiArg, "%f %f", &val.x, &val.y);
                        assert(num == 2 && "missing shader defaults");

                    }
                    setVariable(vname, val);
                }
                else if (tname = strstr( source, "vec3" ))
                {
                    vname = getVariableName( tname + 4 );
                    p3f val;
                    if (stiDefaultVariable) {
                        int num = sscanf(stiArg, "%f %f %f", &val.x, &val.y, &val.z);
                        assert(num == 3 && "missing shader defaults");
                    }
                    setVariable( vname, val);
                }
                else if (tname = strstr( source, "vec4" ))
                {
                    vname = getVariableName( tname + 4 );
                    col4f col;
                    if (stiDefaultVariable) {
                        int num = sscanf(stiArg, "%f %f %f %f", &col.r, &col.g, &col.b, &col.o);
                        assert(num == 4 && "missing shader defaults");
                    }
                    setVariable( vname, col);
                }
                else if( tname = strstr( source, "sampler2DRect" ) )
                {
                    vname = getVariableName( tname + 14 );
					if( !stiArg )
					{
                        COglTexture* pDummy = nullptr;
						setTexture( vname, pDummy, false);
					}
                    else if( strstr( stiArg, "GraphicsRGBABuffer") )
                    {
#if TEXTURE_SUPPORT
                        setTexture( vname, new COglGraphicsColorBuffer, true );
#endif
                    }
                    else if( strstr( stiArg, "GraphicsDepthBuffer") )
                    {
#if TEXTURE_SUPPORT
                        setTexture( vname, new COglGraphicsDepthBuffer(), true );
#endif
                    }
                    else if( strstr( stiArg, "SystemRGBABuffer") )
                    {
#if TEXTURE_SUPPORT
                        setTexture( vname, new COglSystemImageBuffer(GL_BGRA_EXT), true );
#endif
                    }
                    else if( strstr( stiArg, "SystemAlphaBuffer") )
                    {
#if TEXTURE_SUPPORT
                        setTexture( vname, new COglSystemImageBuffer(GL_ALPHA), true );
#endif
                    }
                    else
                    {
                        string path = getCheckedImagePath( stiArg );
                        if( path.length() )
                            setTexture( string(vname), path );
						else
							setTexture( string(vname), 0 ); // need to make sure the texture is present and gets a slot for ATI
                    }
                }
                else if( tname = strstr( source, "sampler2D" ) )
                {
                    // we may be using pow2 image buffers in the future
                    // also we should verify the po2ness of the file and trigger a resize if needed
                    vname = getVariableName( tname + 9 );

					if( !stiArg )
					{
						setTexture( string(vname), 0 );
					}
					else
					{
						string path = getCheckedImagePath( stiArg );
						if( path.length() )
							setTexture( string(vname), path );
						else
							setTexture( string(vname), 0 ); // need to make sure the texture is present and gets a slot for ATI
					}
                }
                else if( tname = strstr( source, "samplerCube" ) )
                {
                    vname = getVariableName( tname + 11 );

					if( !stiArg )
					{
						setTexture( string(vname), 0 );
					}
					else
					{
						string path;
#ifdef OPENGLIB_EXPORTS // e.g. FF not StlViewer
						StringT prefName = getPrefEntry<StringT>(_T("HighQualityEnvFileName"));
						if (prefName.isEmpty())
							path = getCheckedImagePath( stiArg );
						else {
							StringC str = toStringC(prefName);
							path = getCheckedImagePath( str.getBuffer() );
						}
#else
						path = getCheckedImagePath( stiArg );
#endif
						if( path.length() )
							setTexture( string(vname), path );
						else
							setTexture( string(vname), 0 ); // need to make sure the texture is present and gets a slot for ATI
					}
				}
                else if( stiDefaultVariable )
                {
                    assert(!"unknown type");
                    continue;
                } 
            }
        } while( source = getNextLine(source, endChar) );

        delete [] sourceCpy;
    }
    m_defaultsLoaded = true;
    firstCall = false;
}

#define LOG_AND_RETURN(a,b) if(a) { m_error = true; m_log = b; return false;}

bool COglShaderBase::load()
{
    HDTIMELOG("Entering COglShaderBase::load()");
    CHECK_GLSL_STATE;

    // Get Vertex And Fragment Shader Sources
    const char * isource = getShaderIncludeSource();
    const char * fsource = getFragmentShaderSource();
    const char * vsource = getVertexShaderSource();
    const char * gsource = getGeometryShaderSource();

    // insert the 'shader include source' in the fragment source
    // we may want to extend this in the future
    size_t len = isource ? strlen(isource)+strlen(fsource)+1 : strlen(fsource);

#pragma warning(push)
#pragma warning(disable: 4996)

#pragma warning(pop)

    LOG_AND_RETURN( !vsource, "Failed to access shader code" )

    // Create Shader And Program Objects

    _programId = glCreateProgram(); GL_ASSERT;
    _vertexId  = glCreateShader(GL_VERTEX_SHADER_ARB); GL_ASSERT;
    _fragmentId = glCreateShader(GL_FRAGMENT_SHADER_ARB); GL_ASSERT;
    _geometryId = gsource ? glCreateShader(GL_GEOMETRY_SHADER_EXT) : 0; GL_ASSERT;

    LOG_AND_RETURN( !_programId || !_vertexId || !_fragmentId, "Shader compilation failed" )

    if(gsource && !_geometryId)
        LOG_AND_RETURN( gsource && !_geometryId, "Your card and or driver does not support geometry shaders")

    // Load Shader Sources
    glShaderSource(_vertexId, 1, (const GLchar**)&vsource, NULL); GL_ASSERT;
    glShaderSource(_fragmentId, 1, (const GLchar**) &fsource, NULL); GL_ASSERT;

    if (gsource && _geometryId) {
        glShaderSource(_geometryId, 1, (const GLchar**)&gsource, NULL); GL_ASSERT;
    }

    // Compile The Shaders
    glCompileShader(_vertexId); GL_ASSERT;
	hasShaderError(_vertexId);

    glCompileShader(_fragmentId); GL_ASSERT;
	hasShaderError(_fragmentId);

    if( gsource && _geometryId)
	{
        glCompileShader(_geometryId); GL_ASSERT;
		hasShaderError(_geometryId);
	}

    // Attach The Shader Objects To The Program Object
    glAttachShader(_programId, _vertexId);   GL_ASSERT;
    glAttachShader(_programId, _fragmentId);   GL_ASSERT;

    if( gsource && _geometryId && glProgramParameteri )
    {
        glAttachShader(_programId, _geometryId); GL_ASSERT;

        glProgramParameteri(_programId, GL_GEOMETRY_INPUT_TYPE_EXT,  m_geomShaderInType  ); GL_ASSERT;
        glProgramParameteri(_programId, GL_GEOMETRY_OUTPUT_TYPE_EXT, m_geomShaderOutType ); GL_ASSERT;

		int temp;
		glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT,&temp); GL_ASSERT;
		glProgramParameteri(_programId, GL_GEOMETRY_VERTICES_OUT_EXT,temp); GL_ASSERT;
    }

    // Link The Program Object
    glLinkProgram(_programId); GL_ASSERT

    bind();
    // These are required. If they fail, abort
    m_vertLoc = glGetAttribLocation(_programId, _vertAttribName.c_str()); GL_ASSERT;
    m_normLoc = glGetAttribLocation(_programId, _normalAttribName.c_str()); GL_ASSERT;

    if (!_texParamAttribName.empty()) {
        m_texParamLoc = glGetAttribLocation(_programId, _texParamAttribName.c_str()); GL_ASSERT;
    }

    if (!_colorAttribName.empty()) {
        m_colorLoc = glGetAttribLocation(_programId, _colorAttribName.c_str()); GL_ASSERT;
    }

    if (!m_defaultsLoaded)
        loadDefaultVariables();

    initUniform();
    CHECK_GLSL_STATE;

    unBind();

    HDTIMELOG("Succesfully completed COglShaderBase::load()");
    return true;
}

 void COglShaderBase::setGeomShaderIOtype( int intype, int outtype)
 {
    m_geomShaderInType = intype;
    m_geomShaderOutType = outtype;
 }

bool COglShaderBase::unLoad()
{
    CHECK_GLSL_STATE;
    return true;
}

bool COglShaderBase::bind()
{
    GL_IGNORE_ERROR;    //ignore any rogue errors from other areas of the system
    CHECK_GLSL_STATE;
    
    assert(!m_bound); // logic error all shaders must be bound and unbound 

    int progID = programID();
    if( progID && !m_error )
    {
        glUseProgramObjectARB( progID ); GL_ASSERT; // Use The Program Object Instead Of Fixed Function OpenGL
        hasProgramError( progID );       // this can show additional errors...
    }

    else if (m_error) {             // some error state, so stop trying to load or bind
        glUseProgramObjectARB(0); GL_ASSERT; // Fixed Function OpenGL

    } else // must be first time, shader needs loading
    {
        if( !load() )
            return false;
   
        else
        {
            glUseProgramObjectARB( progID ); GL_ASSERT; // Use The Program Object Instead Of Fixed Function OpenGL

            if( hasProgramError( progID ) )
            {
                assert(!"shader program error");
                return false;
            }

            GL_ASSERT; // seems like the above is no longer catching fail conditions, e.g. simple glsl compile errors
        }
    }     

#if TEXTURE_SUPPORT
    //bind all the textures now 
    if( m_TextureMap.size() )
    {
        assert( !m_textureUnitStates ); // texture states should have been destroyed at end of each render pass
        m_textureUnitStates = new ActiveTextureUnits;

		int texUnit = -1;

        // taking 2 passes so the textures with matrices get the first 8 slots
        // texture slots 8-16 do not have matrices
        for( int i=0; i<2; i++ ) 
        for( TextureMapType::iterator mp = m_TextureMap.begin(); mp != m_TextureMap.end(); mp++ )
        {
            string textureName  =  mp->first;
            COglTexture* texture =  mp->second;

            if( !i && (texture && texture->textureMatrix().isIdentity()) )
                continue;

            texUnit++;
			int type = texture ? texture->get_type() : GL_TEXTURE_2D;
			int hwid = texture ? texture->m_hwID : 0;

            m_textureUnitStates->activateUnit( texUnit, type );               GL_ASSERT;

			if( texture )
			{
				texture->m_textureShaderUnit = texUnit;
				texture->bind();                                                   GL_ASSERT;

				// texture matrix 
				if( texUnit < 8 ) // as of GTX280 anno end 2009 there are only 8 slots for texture matrices
				{
					glMatrixMode(GL_TEXTURE); // verify that this works correctly for independent texture slots
					glLoadIdentity();
					glMultMatrixf( texture->textureMatrix() );
					glMatrixMode(GL_MODELVIEW);													GL_ASSERT;
				}
			}

            int samplerLocation = glGetUniformLocationARB( progID, Uni2Ansi(textureName));  GL_ASSERT;
            
            HLOG(Format("Bound texture %s unit: %d loc: %d hwid: %d",(const string&)textureName, texUnit, samplerLocation, hwid ));

            if( samplerLocation >= 0 )
            {
                glUniform1iARB(samplerLocation, texUnit);    GL_ASSERT;
            }
        }
    }
#endif

    //send all the arguments
    if(m_argumentMap.size())
    {
        for( ArgMapType::iterator arg = m_argumentMap.begin(); arg != m_argumentMap.end(); arg++ )
        {
            string argName =  arg->first;
            COglArg& argVal  = *arg->second;

            int argLocation = glGetUniformLocationARB( programID(), argName.c_str()); GL_ASSERT;

            if( argLocation >= 0 )
            {
                /* This would be good to be able to switch on per shader
                string argval;
                switch(argVal.getType() )
                {
                case COglArg::eFloat: argval = Format(" %.1f",argVal.getFloat());break;
                case COglArg::eFloat3:
                    {
                        float* val=argVal.getFloatPtr();
                        argval = Format(" %.1f %.1f %.1f",val[0],val[1],val[2]);break;
                    }
                case COglArg::eFloat4:
                    {
                        float* val=argVal.getFloatPtr();
                        argval = Format(" %.1f %.1f %.1f %.1f",val[0],val[1],val[2],val[3]);break;
                    }
                }
                HLOG(Format("Binding argName:%s val:%s",(const string&)argName,(const string&)argval));
                */

                switch( argVal.getType() )
                {
                case COglArg::eInt: 
                    glUniform1i( argLocation, argVal.getInt() ); GL_ASSERT;
                    break;
                case COglArg::eFloat:
                    glUniform1f( argLocation, argVal.getFloat() );  GL_ASSERT;
                    break;
                case COglArg::eFloat2: 
                    glUniform2fv(argLocation, 1, argVal.getFloatPtr()); GL_ASSERT;
                    break;
                case COglArg::eFloat3:
                    glUniform3fv(argLocation, 1, argVal.getFloatPtr()); GL_ASSERT;
                    break;
                case COglArg::eFloat4:
                    glUniform4fv(argLocation, 1, argVal.getFloatPtr()); GL_ASSERT;
                    break;
                case COglArg::eFloat16:
                    glUniformMatrix4fv( argLocation, 1, false, argVal.getFloatPtr());    GL_ASSERT;
                    break;
                default: assert(0);
                }
            }
        }
    }

    m_bound = true;
    return m_error;
}

bool COglShaderBase::getVariable( const string& name, string& type, string& value ) const
{
    bool found = false;

    if(m_argumentMap.size())
    {
        auto arg = m_argumentMap.find( name );
        stringstream ss;
        if( arg != m_argumentMap.end() )
        {
            found = true;
            COglArg& argVal = *arg->second;
            switch( argVal.getType() )
            {
            case COglArg::eInt: 
                assert(!"implement type");
                break;
            case COglArg::eFloat:
                type = "float";  // TBD: should share this string with COglMaterial
                ss << argVal.getFloat();
                break;
            case COglArg::eFloat2: 
                assert(!"implement type"); 
                break;
            case COglArg::eFloat3:
                assert(!"implement type");
                break;
            case COglArg::eFloat4:
                type = "color";  // TBD: should share this string with COglMaterial
                ss << argVal.getFloatAt(0) << " " << argVal.getFloatAt(1) << " " << argVal.getFloatAt(2) << " " << argVal.getFloatAt(3);
                break;
            case COglArg::eFloat16:
                type = "m44f";  // TBD: should share this string with COglMaterial
                for(int i=0; i<16; i++) {
                    ss << argVal.getFloatAt(i);
                    if (i != 15)
                        ss << " ";
                }
                break;
            default: assert(0);
            }
            value = ss.str();
        }
    }

    return found;
}

bool COglShaderBase::getVariablei( const string& name, int& value ) const
{
    auto arg = m_argumentMap.find( name );
    if( arg != m_argumentMap.end() )
    {
        COglArg& argVal = *arg->second;
        assert( argVal.getType() == COglArg::eInt );
        
		value = argVal.getInt();
        return true;
    }

    assert_variable_not_found;
    return false;
}

bool COglShaderBase::getVariable( const string& name, float& value ) const
{
    auto arg = m_argumentMap.find( name );
    if( arg != m_argumentMap.end() )
    {
        COglArg& argVal = *arg->second;
        assert( argVal.getType() == COglArg::eFloat );
        
        value = argVal.getFloat();
        return true;
    }

    assert_variable_not_found;
    return false;
}

bool COglShaderBase::getVariable( const string& name, col4f& value ) const
{
    auto arg = m_argumentMap.find( name );
    if( arg != m_argumentMap.end() )
    {
        COglArg& argVal = *arg->second;
        assert( argVal.getType() == COglArg::eFloat4 );

        float* flt = argVal.getFloatPtr();
        value.set( flt[0], flt[1], flt[2], flt[3] );

        return true;
    }

    assert_variable_not_found;
    return false;
}

bool COglShaderBase::getVariable( const string& name, m44f&  value ) const
{
    auto arg = m_argumentMap.find( name );
    if( arg != m_argumentMap.end() )
    {
        COglArg& argVal = *arg->second;
        assert( argVal.getType() == COglArg::eFloat16 );

        value.set( argVal.getFloatPtr() );

        return true;
    }

    assert_variable_not_found;
    return false;
}

bool COglShaderBase::getVariable( const string& name, p4f&   value ) const
{
    auto arg = m_argumentMap.find( name );
    if( arg != m_argumentMap.end() )
    {
        COglArg& argVal = *arg->second;
        assert( argVal.getType() == COglArg::eFloat4 );

        float* flt = argVal.getFloatPtr();
        value.set( flt[0], flt[1], flt[2], flt[3] );

        return true;
    }

    assert_variable_not_found;
    return false;
}

bool COglShaderBase::getVariable( const string& name, p3f&   value ) const
{
    auto arg = m_argumentMap.find( name );
    if( arg != m_argumentMap.end() )
    {
        COglArg& argVal = *arg->second;
        assert( argVal.getType() == COglArg::eFloat3 );

        float* flt = argVal.getFloatPtr();
        value.set( flt[0], flt[1], flt[2]);

        return true;
    }

    assert_variable_not_found;
    return false;
}

bool COglShaderBase::getVariable( const string& name, p2f&   value ) const
{
    auto arg = m_argumentMap.find( name );
    if( arg != m_argumentMap.end() )
    {
        COglArg& argVal = *arg->second;
        assert( argVal.getType() == COglArg::eFloat2 );        

        float* flt = argVal.getFloatPtr();
        value.set( flt[0], flt[1]);

        return true;
    }

    assert_variable_not_found;
    return false;
}

bool COglShaderBase::unBind()
{    
    CHECK_GLSL_STATE;
    assert(m_bound); // logic error can't unbind an unbound shader

    glUseProgramObjectARB(0);   GL_ASSERT; // Back to Fixed Function OpenGL

#if TEXTURE_SUPPORT
	//reset all the states of all the texture units
	if( m_TextureMap.size() )
	{
		assert( m_textureUnitStates );
		delete m_textureUnitStates; //the destructor fires the state cleanup code
		m_textureUnitStates = 0;
	}
#endif

    m_bound = false;   
    return true;
}

int COglShaderBase::programID() const
{
    return _programId;
}

int COglShaderBase::vertexID() const
{
    return _vertexId;
}

int COglShaderBase::fragmentID() const
{
    return _fragmentId;
}

int COglShaderBase::geometryID() const
{
    return _geometryId;
}

void COglShader::setIncludeSrcFile(const string& filename)
{
    ifstream in(filename);
    string src;
    char buf[2048];
    while (in.good() && !in.eof()) {
        in.getline(buf, 2048);
        src += string(buf) + "\n";
    }
    setIncludeSrc(src);
}

void COglShader::setVertexSrcFile(const string& filename)
{
    ifstream in(filename);
    if (!in.good() || !in.is_open())
    	cout << "Cannot read vertex shader source.\n";
    string src;
    char buf[2048];
    while (in.good() && !in.eof()) {
        in.getline(buf, 2048);
        src += string(buf) + "\n";
    }
    setVertexSrc(src);
}

void COglShader::setFragmentSrcFile(const string& filename)
{
    ifstream in(filename);
    string src;
    char buf[2048];
    while (in.good() && !in.eof()) {
        in.getline(buf, 2048);
        src += string(buf) + "\n";
    }
    setFragmentSrc(src);
}

void COglShader::setGeometrySrcFile(const string& filename)
{
    ifstream in(filename);
    string src;
    char buf[2048];
    while (in.good() && !in.eof()) {
        in.getline(buf, 2048);
        src += string(buf) + "\n";
    }
    setGeometrySrc(src);
}

void COglShader::setIncludeSrc(const string& src)
{
    _shaderIncSrc = src;
}

void COglShader::setVertexSrc(const string& src)
{
    _vertSrc = src;
}

void COglShader::setFragmentSrc(const string& src)
{
    _fragSrc = src;
}

void COglShader::setGeometrySrc(const string& src)
{
    _geomSrc = src;
}

const char* COglShader::getShaderIncludeSource() const
{
    if (!_shaderIncSrc.empty())
        return _shaderIncSrc.data();
    return nullptr;
}

const char* COglShader::getVertexShaderSource() const
{
    if (!_vertSrc.empty())
        return _vertSrc.data();
    return nullptr;
}

const char* COglShader::getFragmentShaderSource() const
{
    if (!_fragSrc.empty())
        return _fragSrc.data();
    return nullptr;
}

const char* COglShader::getGeometryShaderSource() const
{
    if (!_geomSrc.empty())
        return _geomSrc.data();
    return nullptr;
}

void COglShader::initUniform()
{

}
