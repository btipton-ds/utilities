// OglShader.cpp: implementation of the COglMesh class.
//
//////////////////////////////////////////////////////////////////////

//#include <Kernel/OpenGLib/glu.h> // LIMBO not picking up from the precompiled header in 64 bit
#include <OglShader.h>
#include <vector>
#include <map>

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

typedef map<CString,COglTexture*> TextureMapType;
#define textureMap (*((TextureMapType*)(m_TextureMap)))

typedef map<CString,COglArg*> ArgMapType;
#define argumentMap (*((ArgMapType*)(m_ArgumentMap)))

PFNWGLGETEXTENSIONSSTRINGARBPROC COglShader::wglGetExtensionsStringARB =0;
PFNGLGETSHADERIVPROC			 COglShader::glGetShaderiv			   =0;
PFNGLGETINFOLOGARBPROC			 COglShader::glGetInfoLogARB		   =0;
PFNGLGETPROGRAMIVPROC			 COglShader::glGetProgramiv			   =0;
PFNGLGETPROGRAMINFOLOGPROC		 COglShader::glGetProgramInfoLog       =0;
PFNGLCREATEPROGRAMOBJECTARBPROC  COglShader::glCreateProgramObjectARB  =0;
PFNGLCREATESHADEROBJECTARBPROC   COglShader::glCreateShaderObjectARB   =0;
PFNGLSHADERSOURCEARBPROC         COglShader::glShaderSourceARB         =0;
PFNGLCOMPILESHADERARBPROC        COglShader::glCompileShaderARB        =0;
PFNGLATTACHOBJECTARBPROC         COglShader::glAttachObjectARB         =0;
PFNGLLINKPROGRAMARBPROC          COglShader::glLinkProgramARB          =0;
PFNGLUSEPROGRAMOBJECTARBPROC     COglShader::glUseProgramObjectARB     =0;
PFNGLGETUNIFORMLOCATIONARBPROC   COglShader::glGetUniformLocationARB   =0;
PFNGLUNIFORM1IARBPROC            COglShader::glUniform1iARB            =0;
PFNGLUNIFORM1FARBPROC            COglShader::glUniform1fARB            =0;
PFNGLUNIFORM2FARBPROC            COglShader::glUniform2fARB            =0;
PFNGLUNIFORM3FARBPROC            COglShader::glUniform3fARB            =0;
PFNGLUNIFORM4FARBPROC            COglShader::glUniform4fARB            =0;
PFNGLUNIFORMMATRIX4FVARBPROC     COglShader::glUniformMatrix4fv        =0; 
PFNGLPROGRAMPARAMETERIEXTPROC    COglShader::glProgramParameteriEXT    =0; 
PFNGLACTIVETEXTUREPROC			 COglShader::glActiveTexture		   =0;
PFNGLTEXIMAGE3DEXTPROC           COglShader::glTexImage3D              =0;

#if HAS_SHADER_SUBROUTINES
COglShader::PFNGLGETSUBROUTINEINDEXPROC		 COglShader::glGetSubroutineIndex	   =0;
COglShader::PFNGLUNIFORMSUBROUTINESUIVPROC	 COglShader::glUniformSubroutinesuiv   =0;
#endif

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

COglArg::COglArg(float* val, int numFloats)
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

void  COglArg::set(float* val)
{
    switch(type)
    {
    case eFloat :  fval = *val;                            break;
    case eFloat2:  memcpy( fvalptr, val, sizeof(float)*2) ; break;
    case eFloat3:  memcpy( fvalptr, val, sizeof(float)*3) ; break;
    case eFloat4:  memcpy( fvalptr, val, sizeof(float)*4) ; break;
    case eFloat16: memcpy( fvalptr, val, sizeof(float)*16); break;
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

    return fvalptr;
}

float COglArg::getFloatAt(int idx)
{
    switch( type )
    {
    case eFloat2:  if( idx >= 0 && idx < 2 ) return fvalptr[idx]; else { assert(!"range error"); }  break;
    case eFloat3:  if( idx >= 0 && idx < 3 ) return fvalptr[idx]; else { assert(!"range error"); }  break;
    case eFloat4:  if( idx >= 0 && idx < 4 ) return fvalptr[idx]; else { assert(!"range error"); }  break;
    case eFloat16: if( idx >= 0 && idx < 16) return fvalptr[idx]; else { assert(!"range error"); }  break;
    default: assert(0); 
    } 

    return 0.0f;
}

void COglArg::allocFloat(float* val, int numFloats)
{
    fvalptr = new float[numFloats];
    memcpy( fvalptr, val, sizeof(float)*numFloats);
}

COglArg::~COglArg()
{
    if( type >= eFloat2 || type <= eFloat16 )
        delete [] fvalptr;
}


bool COglShader::mEnabled = true; //user override of global shader usage

COglShader::COglShader()
    : m_Error(false)
	, m_TextureUnitStates(0)
    , m_DefaultsLoaded(false)
    , m_Bound(false)
    , m_GeomShaderInType(GL_TRIANGLES)
    , m_GeomShaderOutType(GL_TRIANGLES)
{
    m_TextureMap  = (void*) new TextureMapType;
    m_ArgumentMap = (void*) new ArgMapType;
}

COglShader::~COglShader()
{
    HLOG(_T("~COglShader()"));
    clearTextures();
    delete ((TextureMapType*) m_TextureMap);
    delete ((ArgMapType*) m_ArgumentMap);
}

bool COglShader::isEnabled()
{
    return mEnabled && hasShaderSupport();
}

void COglShader::enable(bool set)
{
    mEnabled = set;
}

bool COglShader::hasOGL3Support()
{
	static bool ok = false;

    if( !wglGetCurrentContext() )
    {
        assert( !"no wglGetCurrentContext" );
        return ok;
    }

	RETURN_IF_NOT_FIRST_TIME(ok);
    CString message;

    ok = hasShaderSupport();
    ok = ok ? Ogl::verifyGLVersionAtOrAfter(3, 0, message) : false; 
    ok = ok ? checkOglExtension("GL_EXT_geometry_shader4") : false;

	if(!ok)
    {
        LoadOglResourceLocal localRsc;
		CString frmtstr;
        frmtstr.LoadString( IDS_INADEQUATE_HW );
		AfxMessageBox( Format( (LPCTSTR)frmtstr, (LPCTSTR)message));
    }

    return ok;
}

bool COglShader::hasShaderSupport()
{
	static bool ok = false;

    if( !wglGetCurrentContext() )
    {
        assert( !"no wglGetCurrentContext" );
        return ok;
    }

#ifdef OPENGLIB_EXPORTS // e.g. FF not StlViewer
	static Symbol RPOM_sym( "RunPrimitiveOglMode" );
	if( getPrefEntry<bool>(RPOM_sym) )
		return false;
#endif

	RETURN_IF_NOT_FIRST_TIME(ok);

    CString message;
	ok = Ogl::verifyGLVersionAtOrAfter(3, 0, message); // required by metal shader (high quality)  
    // LIMBO, if needed we will need ATI version too:
    // ok = ok ? Ogl::verifyNVidiaDriverVersionAtLeast(_T("6.14.11.6262"), message) : false;
	ok = ok ? loadGlFuncPointers(): false;
	ok = ok ? checkOglExtension("GL_ARB_shader_objects") : false;

	if(!ok)
    {
        LoadOglResourceLocal localRsc;
		CString frmtstr;
        frmtstr.LoadString( IDS_INADEQUATE_HW );

		AfxMessageBox( Format( (LPCTSTR)frmtstr, (LPCTSTR)message));

#ifndef OPENGLIB_EXPORTS //meaning we're not building this as part of the FF OpenGLib.dll
        abort();
#endif
    }

	return ok;
}

void COglShader::setVariablei( LPCTSTR name, int  value )
{
    if( !m_DefaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = argumentMap.find( name );
    if( it == argumentMap.end() )
        argumentMap.insert( ArgMapType::value_type( name, new COglArg( value ) ) );
    else
        it->second->set(value);

    if(m_Bound)
    {
		int progId = programID();
		int dbg = glGetUniformLocationARB(progId, "DepthPeel");
        int argLocation = glGetUniformLocationARB( progId, Uni2Ansi(name)); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform1iARB( argLocation, value ); GL_ASSERT;
        }
    }
}

void COglShader::setVariable( LPCTSTR name, float  value )
{
    if( !m_DefaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = argumentMap.find( name );
    if( it == argumentMap.end() )
        argumentMap.insert( ArgMapType::value_type( name, new COglArg( value ) ) );
    else
        it->second->set(value);

    if(m_Bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), Uni2Ansi(name)); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform1fARB( argLocation, value ); GL_ASSERT;
        }
    }
}

void COglShader::setVariable( LPCTSTR name, col4f& value )
{
    if( !m_DefaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = argumentMap.find( name );
    if( it == argumentMap.end() )
        argumentMap.insert( ArgMapType::value_type( name, new COglArg( (float*)value, 4 ) ) );
    else
        it->second->set(value);

    if(m_Bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), Uni2Ansi(name)); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform4fARB( argLocation, value.r, value.g, value.b, value.o );  GL_ASSERT;
        }
    }
}

void COglShader::setVariable( LPCTSTR name, m44f&  value )
{
    if( !m_DefaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = argumentMap.find( name );
    if( it == argumentMap.end() )
        argumentMap.insert( ArgMapType::value_type( name, new COglArg( value.transposef(), 16 ) ) );
    else
        it->second->set( value.transposef() );

    if(m_Bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), Uni2Ansi(name)); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniformMatrix4fv( argLocation, 1, false, value.transposef() );  GL_ASSERT;
        }
    }

}

void COglShader::setVariable( LPCTSTR name, p4f&   value )
{
    if( !m_DefaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = argumentMap.find( name );
    if( it == argumentMap.end() )
        argumentMap.insert( ArgMapType::value_type( name, new COglArg( (float*)value, 4 ) ) );
    else
        it->second->set(value);

    if(m_Bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), Uni2Ansi(name)); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform4fARB( argLocation, value.x, value.y, value.z, value.w ); GL_ASSERT;
        }
    }
}

void COglShader::setVariable( LPCTSTR name, p3f&   value )
{
    if( !m_DefaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = argumentMap.find( name );
    if( it == argumentMap.end() )
        argumentMap.insert( ArgMapType::value_type( name, new COglArg( (float*)value, 3 ) ) );
    else
        it->second->set(value);

    if(m_Bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), Uni2Ansi(name)); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform3fARB( argLocation, value.x, value.y, value.z );  GL_ASSERT;
        }
    }
}

void COglShader::setVariable( LPCTSTR name, p2f&   value )
{
    if( !m_DefaultsLoaded ) 
        loadDefaultVariables();

    ArgMapType::iterator it;

    it = argumentMap.find( name );
    if( it == argumentMap.end() )
        argumentMap.insert( ArgMapType::value_type( name, new COglArg( (float*)value, 2 ) ) );
    else
        it->second->set(value);

    if(m_Bound)
    {
        int argLocation = glGetUniformLocationARB( programID(), Uni2Ansi(name)); GL_ASSERT;
        if( argLocation >= 0 )
        {
            glUniform2fARB( argLocation, value.x, value.y );  GL_ASSERT;
        }
    }
}

#if HAS_SHADER_SUBROUTINES
void COglShader::setFragSubRoutine(const char* name)
{
	GLuint subIdx = glGetSubroutineIndex(programID(), GL_FRAGMENT_SHADER, name);
	if (subIdx >= 0) {
		glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subIdx);
	}
}

void COglShader::setVertSubRoutine(const char* name)
{
	GLuint subIdx = glGetSubroutineIndex(programID(), GL_VERTEX_SHADER, name);
	if (subIdx >= 0) {
		glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &subIdx);
	}
}
#endif
void COglShader::clearTextures()
{
    for( TextureMapType::iterator mp = textureMap.begin(); mp != textureMap.end(); mp++ )
        if( mp->second && (mp->second->mOwner == this) )
        {
            HLOG(Format(_T("deleting texture %d"),mp->second->m_hwID));
            delete mp->second;
        }

    textureMap.clear();
}

bool COglShader::setTexture( LPCTSTR textureShaderName, LPCTSTR filename, bool flipImage )
{
    HLOG(Format(_T("COglShader::setTexture %s %s"),textureShaderName,filename));

    TextureMapType::iterator map = textureMap.find( textureShaderName );
    if( map != textureMap.end() )
    {
        if( map->second && (map->second->mOwner == this) )
        {
            HLOG(Format(_T("deleting texture %s, id %d in in shader"),textureShaderName,map->second->m_hwID));
            delete map->second;
			map->second = 0;
        }
    }

	if( !filename )
	{
		textureMap.insert( TextureMapType::value_type( textureShaderName, (COglTexture *)0 ) );
		return true;
	}

    bool dds=false;
    {
        //LIMBO use check extension utility
        CString name( filename );
        name = name.MakeLower();
        dds = name.Find(_T(".dds")) > 0;
    }
    
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

    textureMap.insert( TextureMapType::value_type( textureShaderName, img ) );
    return true;
}

bool COglShader::hasTexture ( LPCTSTR textureShaderName)
{
    TextureMapType::iterator target = textureMap.find( textureShaderName );
    return target != textureMap.end();
}

bool COglShader::setTexture ( LPCTSTR textureShaderName, COglTexture* texture, bool takeOwnerShip )
{
    // HLOG(Format(_T("COglShader::setTexture %s %s"),textureShaderName, takeOwnerShip ? _T("shader will delete") : _T("shader will NOT delete")));

    if( !m_DefaultsLoaded ) 
        loadDefaultVariables();

    if( !texture )
    {
        TextureMapType::iterator target = textureMap.find( textureShaderName );
        if( target != textureMap.end() )
        {
            if( target->second && (target->second->mOwner == this ))
            {
                HLOG(Format(_T("deleting texture %s, id %d in in shader"),textureShaderName,target->second->m_hwID));
                delete target->second;
				target->second = 0;
            }
        }

		textureMap.insert( TextureMapType::value_type( textureShaderName, (COglTexture *)0 ) );
    }
    else
    {
        TextureMapType::iterator target = textureMap.find( textureShaderName );
        if( target != textureMap.end() )
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

            textureMap.insert( TextureMapType::value_type( textureShaderName, texture ) );
        }
    }

    return true;
}

COglTexture* COglShader::getTexture( LPCTSTR textureShaderName )
{
    if( !m_DefaultsLoaded ) 
        loadDefaultVariables();

    TextureMapType::iterator target = textureMap.find( textureShaderName );
    assert(target != textureMap.end() && "shader not initialized or texture name incorrect");

    if( target != textureMap.end() )
        return target->second;     

    return 0;
}

bool COglShader::loadGlFuncPointers()
{
    static bool gl_ok = false;
    RETURN_IF_NOT_FIRST_TIME( gl_ok );

    assert( wglGetCurrentContext() ); //always call within valid gl rendering context

    if( checkOglExtension("GL_ARB_shader_objects") )
    {
		GET_EXT_POINTER( glGetShaderiv           , PFNGLGETSHADERIVPROC );
        GET_EXT_POINTER( glGetInfoLogARB         , PFNGLGETINFOLOGARBPROC );  
		GET_EXT_POINTER( glGetProgramiv          , PFNGLGETPROGRAMIVPROC ); 
		GET_EXT_POINTER( glGetProgramInfoLog     , PFNGLGETPROGRAMINFOLOGPROC ); 
        GET_EXT_POINTER( glCreateProgramObjectARB, PFNGLCREATEPROGRAMOBJECTARBPROC );
        GET_EXT_POINTER( glCreateShaderObjectARB , PFNGLCREATESHADEROBJECTARBPROC ); 
        GET_EXT_POINTER( glShaderSourceARB       , PFNGLSHADERSOURCEARBPROC );       
        GET_EXT_POINTER( glCompileShaderARB      , PFNGLCOMPILESHADERARBPROC );      
        GET_EXT_POINTER( glAttachObjectARB       , PFNGLATTACHOBJECTARBPROC );       
        GET_EXT_POINTER( glLinkProgramARB        , PFNGLLINKPROGRAMARBPROC );        
        GET_EXT_POINTER( glUseProgramObjectARB   , PFNGLUSEPROGRAMOBJECTARBPROC );   
        GET_EXT_POINTER( glGetUniformLocationARB , PFNGLGETUNIFORMLOCATIONARBPROC );
        GET_EXT_POINTER( glUniform1iARB          , PFNGLUNIFORM1IARBPROC );
        GET_EXT_POINTER( glUniform1fARB          , PFNGLUNIFORM1FARBPROC );
        GET_EXT_POINTER( glUniform2fARB          , PFNGLUNIFORM2FARBPROC );
        GET_EXT_POINTER( glUniform3fARB          , PFNGLUNIFORM3FARBPROC );
        GET_EXT_POINTER( glUniform4fARB          , PFNGLUNIFORM4FARBPROC );
        GET_EXT_POINTER( glUniformMatrix4fv      , PFNGLUNIFORMMATRIX4FVARBPROC );
        GET_EXT_POINTER( glProgramParameteriEXT  , PFNGLPROGRAMPARAMETERIEXTPROC );
        GET_EXT_POINTER( glActiveTexture  , PFNGLACTIVETEXTUREPROC );
		GET_EXT_POINTER( glTexImage3D     , PFNGLTEXIMAGE3DEXTPROC );

		gl_ok = glGetInfoLogARB && glCreateProgramObjectARB && glCreateShaderObjectARB && glShaderSourceARB &&
                glCompileShaderARB && glAttachObjectARB && glLinkProgramARB && glUseProgramObjectARB &&
                glGetUniformLocationARB && glUniform1iARB && 
                glUniform1fARB && glUniform2fARB && glUniform3fARB && glUniform4fARB &&
                glUniformMatrix4fv && glActiveTexture && glTexImage3D;

#if HAS_SHADER_SUBROUTINES
        GET_EXT_POINTER( glGetSubroutineIndex  , PFNGLGETSUBROUTINEINDEXPROC );
        GET_EXT_POINTER( glUniformSubroutinesuiv  , PFNGLUNIFORMSUBROUTINESUIVPROC );
		gl_ok = gl_ok && glGetSubroutineIndex && glUniformSubroutinesuiv;
#endif
        if(!gl_ok)
        {
            LoadOglResourceLocal glres;
            AfxMessageBox(IDS_FAILURE_TO_LOAD);
        }
    }
    else
    {
        LoadOglResourceLocal glres;
        AfxMessageBox(IDS_GL_SHADER_OBJ_NOT_SUPPORTED);
    }

    return gl_ok;
}

bool COglShader::hasShaderError(int obj)
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

		CString wErr = CString(err);

//		cout << err << endl;
		HLOG( Format( _T("compiling shader \"%s\":\n%s\n\n"),(LPCTSTR)m_Name, wErr));
		delete [] err;
	}
#endif
	return false;
}

bool COglShader::hasProgramError(int obj)
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

		CString wErr = CString(err);
		HLOG( Format( _T("linking shader \"%s\":\n%s"),(LPCTSTR)m_Name, wErr) );

		delete [] err;
	}
#endif
	return false;
}

CString COglShader::getDataDir()
{
#ifdef OPENGLIB_EXPORTS
    return getGUIWrapper()->getExeDirectory().getBuffer();
#else
    return _T(""); //LIMBO do something here for standalone mode (Dental etc.)?
#endif
}


//assumes an array of strings delimited with multiple newlines
static char* getNextLine(char* str, char* max)
{
    assert(str && str < max);
    if(!str || (str >= max))
        return 0;

    int offset = strlen(str);

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

    static char name[24];
    name[0] = '\0';
    sscanf_s(str, "%s", name, COUNTOF(name));
    
    int sz = strlen(name);
    if(name[sz-1]==';')
        name[sz-1] = '\0';

    return name;
}

static bool doesFileExist( LPCTSTR pathFile )
{
	CString file(pathFile);
	if (file.IsEmpty())
		return false;

	CFileFind finder;
	bool ok = finder.FindFile( pathFile );

	if(!ok) //double check if there are unquoted spaces in the filename
		ok = finder.FindFile( Format(_T("\"%s\""),pathFile) );

	finder.Close();

	return ok;
}

static CString getCheckedImagePath(const char* str)
{
    CString fname = COglShader::getDataDir();

    if( fname.GetLength() )
    {
        fname += CString( str );

        if( !doesFileExist(fname) )
        {
            assert(!"default shader image file not found");
            fname = _T("");
        }
    }
    return fname;
}

void COglShader::loadDefaultVariables()
{
    m_DefaultsLoaded = true; 

    // LIMBO need to add vertex shader parsing? 
    // LIMBO Do we need more info in the header?
    // LIMBO Should set up a static texture cache for default textures
    // so we only have one default instance of each texture on the card
    for(int i=0; i<3; i++)
    {
        char* orgSource = 0;
        
        switch(i)
        {
        case 0: orgSource = getVertexShaderSource();   break;
        case 1: orgSource = getGeometryShaderSource(); break;
        case 2: orgSource = getFragmentShaderSource(); break;
        };
        
        if(!orgSource || !strlen(orgSource)) 
            continue;

        int end = strlen(orgSource);
        char* sourceCpy = new char[ end+1 ];
        strcpy(sourceCpy,orgSource);
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
            if( !strncmp( source, "uniform", 7 ) )
            {
                char* tname  = 0;
				char* stiArg = 0;

                CString vname;

				if( stiDefaultVariable )
				{
					stiArg = strstr( source, "//*" ) + 3;

					while( stiArg[0] == ' ' || stiArg[0] == '\t' )    
						stiArg++;
				}

                if( stiDefaultVariable && (tname = strstr( source, "float" )) )
                {
                    vname = getVariableName( tname + 5 );
                    setVariable( vname, atof( stiArg ));
                }
                else if( stiDefaultVariable && (tname = strstr( source, "vec2" )) )
                {
                    vname = getVariableName( tname + 4 );
                    p2f val;
                    int num = sscanf( stiArg, "%f %f", &val.x, &val.y ); 
                    assert(num == 2 && "missing shader defaults");

                    setVariable( vname, val);
                }
                else if( stiDefaultVariable && (tname = strstr( source, "vec3" )) )
                {
                    vname = getVariableName( tname + 4 );
                    p3f val;
                    int num = sscanf( stiArg, "%f %f %f", &val.x, &val.y, &val.z ); 
                    assert(num == 3 && "missing shader defaults");

                    setVariable( vname, val);
                }
                else if( stiDefaultVariable && (tname = strstr( source, "vec4" )) )
                {
                    vname = getVariableName( tname + 4 );
                    col4f col;
                    int num = sscanf( stiArg, "%f %f %f %f", &col.r, &col.g, &col.b, &col.o ); 
                    assert(num == 4 && "missing shader defaults");

                    setVariable( vname, col);
                }
                else if( tname = strstr( source, "sampler2DRect" ) )
                {
                    vname = getVariableName( tname + 14 );
					if( !stiArg )
					{
						setTexture( CString(vname), 0 );
					}
                    else if( strstr( stiArg, "GraphicsRGBABuffer") )
                    {
                        setTexture( vname, new COglGraphicsColorBuffer, true );
                    }
                    else if( strstr( stiArg, "GraphicsDepthBuffer") )
                    {
                        setTexture( vname, new COglGraphicsDepthBuffer(), true );
                    }
                    else if( strstr( stiArg, "SystemRGBABuffer") )
                    {
                        setTexture( vname, new COglSystemImageBuffer(GL_BGRA_EXT), true );
                    }
                    else if( strstr( stiArg, "SystemAlphaBuffer") )
                    {
                        setTexture( vname, new COglSystemImageBuffer(GL_ALPHA), true );
                    }
                    else
                    {
                        CString path = getCheckedImagePath( stiArg );
                        if( path.GetLength() )
                            setTexture( CString(vname), path );
						else
							setTexture( CString(vname), 0 ); // need to make sure the texture is present and gets a slot for ATI
                    }
                }
                else if( tname = strstr( source, "sampler2D" ) )
                {
                    // we may be using pow2 image buffers in the future
                    // also we should verify the po2ness of the file and trigger a resize if needed
                    vname = getVariableName( tname + 9 );

					if( !stiArg )
					{
						setTexture( CString(vname), 0 );
					}
					else
					{
						CString path = getCheckedImagePath( stiArg );
						if( path.GetLength() )
							setTexture( CString(vname), path );
						else
							setTexture( CString(vname), 0 ); // need to make sure the texture is present and gets a slot for ATI
					}
                }
                else if( tname = strstr( source, "samplerCube" ) )
                {
                    vname = getVariableName( tname + 11 );

					if( !stiArg )
					{
						setTexture( CString(vname), 0 );
					}
					else
					{
						CString path;
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
						if( path.GetLength() )
							setTexture( CString(vname), path );
						else
							setTexture( CString(vname), 0 ); // need to make sure the texture is present and gets a slot for ATI
					}
				}
                else if( stiDefaultVariable )
                {
                    assert(!"unknown type");
                    continue;
                } 
            }
        }while( source = getNextLine(source, endChar) );

        delete [] sourceCpy;
    }
}

#define LOG_AND_RETURN(a,b) if(a) { m_Error = true; m_Log = b; return false;}

bool COglShader::load()
{
    HDTIMELOG(_T("Entering COglShader::load()"));
    CHECK_GLSL_STATE;

    // Get Vertex And Fragment Shader Sources
    char * isource = getShaderIncludeSource();
    char * fsource = getFragmentShaderSource();
    char * vsource = getVertexShaderSource();
    char * gsource = getGeometryShaderSource();

    // insert the 'shader include source' in the fragment source
    // we may want to extend this in the future
    int len = isource ? strlen(isource)+strlen(fsource)+1 : strlen(fsource);
    char* source = new char[len+1];

    if(isource)
    {
        strcpy(source,isource);
        strcpy(&source[strlen(isource)],fsource);
    }
    else
        strcpy(source,fsource);

    source[len] = '\0';

    LOG_AND_RETURN( !source || !vsource, _T("Failed to access shader code") )

    if( !m_DefaultsLoaded ) 
        loadDefaultVariables();

    // Create Shader And Program Objects
    int& progID = programID();
    int& vertID = vertexID (); 
    int& fragID = fragmentID();
    int& geomID = geometryID();

    progID  = glCreateProgramObjectARB();
    vertID  = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
    fragID  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
    geomID  = gsource ? glCreateShaderObjectARB(GL_GEOMETRY_SHADER_EXT) : 0;

    LOG_AND_RETURN( !progID || !vertID || !fragID, _T("Shader compilation failed") )

    if(gsource && !geomID)
        LOG_AND_RETURN( gsource && !geomID , _T("Your card and or driver does not support geometry shaders"))

    // Load Shader Sources
    glShaderSourceARB( vertID, 1, (const GLcharARB**)&vsource, NULL); GL_ASSERT;
    glShaderSourceARB( fragID, 1, (const GLcharARB**) &source, NULL); GL_ASSERT;

    if( gsource && geomID )
        glShaderSourceARB( geomID, 1, (const GLcharARB**)&gsource, NULL); GL_ASSERT;

    // Compile The Shaders
    glCompileShaderARB( vertID); 
	hasShaderError( vertID );

    glCompileShaderARB( fragID );   
	hasShaderError( fragID );

    if( gsource && geomID )
	{
        glCompileShaderARB( geomID );
		hasShaderError( geomID );
	}

    // Attach The Shader Objects To The Program Object
    glAttachObjectARB( progID, vertID );   GL_ASSERT;
    glAttachObjectARB( progID, fragID );   GL_ASSERT;

    if( gsource && geomID && glProgramParameteriEXT )
    {
        glAttachObjectARB( progID, geomID ); GL_ASSERT;

		glProgramParameteriEXT(progID, GL_GEOMETRY_INPUT_TYPE_EXT,  m_GeomShaderInType  );
		glProgramParameteriEXT(progID, GL_GEOMETRY_OUTPUT_TYPE_EXT, m_GeomShaderOutType );

		int temp;
		glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT,&temp);
		glProgramParameteriEXT(progID, GL_GEOMETRY_VERTICES_OUT_EXT,temp);
    }

    // Link The Program Object
    glLinkProgramARB( progID );  
    delete [] source; // NB: living with the leak if we have an error condition

	initUniform();
    HDTIMELOG(_T("Succesfully completed COglShader::load()"));
    return true;
}

 void COglShader::setGeomShaderIOtype( int intype, int outtype)
 {
    m_GeomShaderInType = intype;
    m_GeomShaderOutType = outtype;
 }

bool COglShader::unLoad()
{
    CHECK_GLSL_STATE;
    return true;
}

bool COglShader::bind()
{
    GL_IGNORE_ERROR;    //ignore any rogue errors from other areas of the system
    CHECK_GLSL_STATE;
    
    assert(!m_Bound); // logic error all shaders must be bound and unbound 

    int& progID = programID();
    if( progID && !m_Error )
    {
        glUseProgramObjectARB( progID ); // Use The Program Object Instead Of Fixed Function OpenGL
        hasProgramError( progID );       // this can show additional errors...
    }

    else if(m_Error)              // some error state, so stop trying to load or bind
        glUseProgramObjectARB(0); // Fixed Function OpenGL
   
    else // must be first time, shader needs loading
    {
        if( !load() )
            return false;
   
        else
        {
            glUseProgramObjectARB( progID ); // Use The Program Object Instead Of Fixed Function OpenGL

            if( hasProgramError( progID ) )
            {
                AfxMessageBox( m_Log );
                return false;
            }

            GL_ASSERT; // seems like the above is no longer catching fail conditions, e.g. simple glsl compile errors
        }
    }     

    //bind all the textures now 
    if( textureMap.size() )
    {
        assert( !m_TextureUnitStates ); // texture states should have been destroyed at end of each render pass
        m_TextureUnitStates = new ActiveTextureUnits;

		int texUnit = -1;

        // taking 2 passes so the textures with matrices get the first 8 slots
        // texture slots 8-16 do not have matrices
        for( int i=0; i<2; i++ ) 
        for( TextureMapType::iterator mp = textureMap.begin(); mp != textureMap.end(); mp++ )
        {
            CString textureName  =  mp->first;
            COglTexture* texture =  mp->second;

            if( !i && (texture && texture->textureMatrix().isIdentity()) )
                continue;

            texUnit++;
			int type = texture ? texture->get_type() : GL_TEXTURE_2D;
			int hwid = texture ? texture->m_hwID : 0;

            m_TextureUnitStates->activateUnit( texUnit, type );               GL_ASSERT;

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
            
            HLOG(Format(_T("Bound texture %s unit: %d loc: %d hwid: %d"),(LPCTSTR)textureName, texUnit, samplerLocation, hwid ));

            if( samplerLocation >= 0 )
            {
                glUniform1iARB(samplerLocation, texUnit);    GL_ASSERT;
            }
        }
    }

    //send all the arguments
    if(argumentMap.size())
    {
        for( ArgMapType::iterator arg = argumentMap.begin(); arg != argumentMap.end(); arg++ )
        {
            CString argName =  arg->first;
            COglArg& argVal  = *arg->second;

            int argLocation = glGetUniformLocationARB( programID(), Uni2Ansi(argName)); GL_ASSERT;

            if( argLocation >= 0 )
            {
                /* This would be good to be able to switch on per shader
                CString argval;
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
                HLOG(Format("Binding argName:%s val:%s",(LPCTSTR)argName,(LPCTSTR)argval));
                */

                switch( argVal.getType() )
                {
                case COglArg::eInt: 
                    glUniform1iARB( argLocation, argVal.getInt() );
                    break;
                case COglArg::eFloat:
                    glUniform1fARB( argLocation, argVal.getFloat() ); 
                    break;
                case COglArg::eFloat2: 
                    glUniform2fARB( argLocation, argVal.getFloatAt(0),  argVal.getFloatAt(1)); 
                    break;
                case COglArg::eFloat3:
                    glUniform3fARB( argLocation, argVal.getFloatAt(0),  argVal.getFloatAt(1), argVal.getFloatAt(2)); 
                    break;
                case COglArg::eFloat4:
                    glUniform4fARB( argLocation, argVal.getFloatAt(0),  argVal.getFloatAt(1), argVal.getFloatAt(2), argVal.getFloatAt(3)); 
                    break;
                case COglArg::eFloat16:
                    glUniformMatrix4fv( argLocation, 1, false, argVal.getFloatPtr());   
                    break;
                default: assert(0);
                }
            }
        }
    }

    m_Bound = true;
    return m_Error;
}

bool COglShader::getVariable( LPCTSTR name, CString& type, CString& value )
{
    bool found = false;

    if(argumentMap.size())
    {
        ArgMapType::iterator arg = argumentMap.find( name );
        if( arg != argumentMap.end() )
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
                value.Format(_T("%f"), argVal.getFloat() );
                break;
            case COglArg::eFloat2: 
                assert(!"implement type"); 
                break;
            case COglArg::eFloat3:
                assert(!"implement type");
                break;
            case COglArg::eFloat4:
                type = "color";  // TBD: should share this string with COglMaterial
                value.Format(_T("%f %f %f %f"), argVal.getFloatAt(0),  argVal.getFloatAt(1), argVal.getFloatAt(2), argVal.getFloatAt(3) );
                break;
            case COglArg::eFloat16:
                type = "m44f";  // TBD: should share this string with COglMaterial
                for(int i=0; i<16; i++)
                    value += Format(_T("%f "), argVal.getFloatAt(i));
                break;
            default: assert(0);
            }
        }
    }

    return found;
}

bool COglShader::getVariablei( LPCTSTR name, int& value )
{
    ArgMapType::iterator arg = argumentMap.find( name );
    if( arg != argumentMap.end() )
    {
        COglArg& argVal = *arg->second;
        assert( argVal.getType() == COglArg::eInt );
        
		value = argVal.getInt();
        return true;
    }

    assert_variable_not_found;
    return false;
}

bool COglShader::getVariable( LPCTSTR name, float& value )
{
    ArgMapType::iterator arg = argumentMap.find( name );
    if( arg != argumentMap.end() )
    {
        COglArg& argVal = *arg->second;
        assert( argVal.getType() == COglArg::eFloat );
        
        value = argVal.getFloat();
        return true;
    }

    assert_variable_not_found;
    return false;
}

bool COglShader::getVariable( LPCTSTR name, col4f& value )
{
    ArgMapType::iterator arg = argumentMap.find( name );
    if( arg != argumentMap.end() )
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

bool COglShader::getVariable( LPCTSTR name, m44f&  value )
{
    ArgMapType::iterator arg = argumentMap.find( name );
    if( arg != argumentMap.end() )
    {
        COglArg& argVal = *arg->second;
        assert( argVal.getType() == COglArg::eFloat16 );

        value.set( argVal.getFloatPtr() );

        return true;
    }

    assert_variable_not_found;
    return false;
}

bool COglShader::getVariable( LPCTSTR name, p4f&   value )
{
    ArgMapType::iterator arg = argumentMap.find( name );
    if( arg != argumentMap.end() )
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

bool COglShader::getVariable( LPCTSTR name, p3f&   value )
{
    ArgMapType::iterator arg = argumentMap.find( name );
    if( arg != argumentMap.end() )
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

bool COglShader::getVariable( LPCTSTR name, p2f&   value )
{
    ArgMapType::iterator arg = argumentMap.find( name );
    if( arg != argumentMap.end() )
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

bool COglShader::unBind()
{    
    CHECK_GLSL_STATE;
    assert(m_Bound); // logic error can't unbind an unbound shader

    glUseProgramObjectARB(0);   GL_ASSERT; // Back to Fixed Function OpenGL

	//reset all the states of all the texture units
	if( textureMap.size() )
	{
		assert( m_TextureUnitStates );
		delete m_TextureUnitStates; //the destructor fires the state cleanup code
		m_TextureUnitStates = 0;
	}

    m_Bound = false;   
    return true;
}

#define RET_LOG_SYS_ERROR(a,b) if(!a) { m_Log = GetLastErrorString( b ); m_Error = true;  return 0; }

char* COglShader::loadShaderFromResource( int shaderID )
{
    LPCTSTR resourceName = _T("Shader");
    LPCTSTR functionName = _T("loadShaderFromResource");

    int err=0;
    HINSTANCE inst = LoadLibrary( getResourceLibrary() );

    HRSRC vertResInfo = FindResource( inst, Format(_T("#%d"), shaderID), resourceName );
    RET_LOG_SYS_ERROR( vertResInfo, functionName );

    HGLOBAL vertRes = LoadResource( inst, vertResInfo );
    RET_LOG_SYS_ERROR( vertRes, functionName );

    char* res = (char*) LockResource( vertRes );
    RET_LOG_SYS_ERROR( res, functionName );

    return res;
}
