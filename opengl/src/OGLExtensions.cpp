#include <OGLExtensions.h>
#include <assert.h>

#ifdef WIN32
#include <tchar.h>

#define GET_EXT_POINTER(name, type) name = (type)wglGetProcAddress(#name)
#define GET_EXT_POINTER_MESSAGE_ONCE(name, type)         {                      static bool first=true; if(first){ first=false; name = (type)wglGetProcAddress(#name); if(!name){ }}}
#define GET_EXT_POINTER_MESSAGE_RETURN_VOID(name, type)  { static bool ok=true; static bool first=true; if(first){ first=false; name = (type)wglGetProcAddress(#name); if(!name){ ok=false; return; }}      if(!ok)return;}
#define GET_EXT_POINTER_MESSAGE_RETURN_FALSE(name, type) { static bool ok=true; static bool first=true; if(first){ first=false; name = (type)wglGetProcAddress(#name); if(!name){ ok=false; return false; }} if(!ok) return false;}
#else
#endif // WIND32

#define	RETURN_IF_NOT_FIRST_TIME(a) static bool first=true; if(!first) return a; first = false;

PFNGLBINDBUFFERPROC              COglExtensions::glBindBuffer = 0;
PFNGLGENBUFFERSPROC              COglExtensions::glGenBuffers = 0;
PFNGLBUFFERDATAPROC              COglExtensions::glBufferData = 0;
PFNGLBUFFERSUBDATAPROC           COglExtensions::glBufferSubData = 0;
PFNGLISBUFFERPROC                COglExtensions::glIsBuffer = 0;
PFNGLDELETEBUFFERSPROC           COglExtensions::glDeleteBuffers = 0;
PFNGLDRAWRANGEELEMENTSEXTPROC    COglExtensions::glDrawRangeElements = 0;
PFNGLGETBUFFERPARAMETERIVPROC    COglExtensions::glGetBufferParameteriv = 0;
PFNGLGETBUFFERSUBDATAPROC        COglExtensions::glGetBufferSubData = 0;

PFNWGLGETEXTENSIONSSTRINGARBPROC COglExtensions::wglGetExtensionsStringARB =0;
PFNGLGETSTRINGIPROC			     COglExtensions::glGetStringi = 0;
PFNGLGETSHADERIVPROC			 COglExtensions::glGetShaderiv = 0;
PFNGLGETINFOLOGARBPROC			 COglExtensions::glGetInfoLogARB = 0;
PFNGLGETPROGRAMIVPROC			 COglExtensions::glGetProgramiv = 0;
PFNGLGETPROGRAMINFOLOGPROC		 COglExtensions::glGetProgramInfoLog = 0;
PFNGLCREATEPROGRAMOBJECTARBPROC  COglExtensions::glCreateProgramObjectARB = 0;
PFNGLCREATESHADEROBJECTARBPROC   COglExtensions::glCreateShaderObjectARB = 0;
PFNGLSHADERSOURCEARBPROC         COglExtensions::glShaderSourceARB = 0;
PFNGLCOMPILESHADERARBPROC        COglExtensions::glCompileShaderARB = 0;
PFNGLATTACHOBJECTARBPROC         COglExtensions::glAttachObjectARB = 0;
PFNGLLINKPROGRAMARBPROC          COglExtensions::glLinkProgramARB = 0;
PFNGLUSEPROGRAMOBJECTARBPROC     COglExtensions::glUseProgramObjectARB = 0;
PFNGLGETUNIFORMLOCATIONARBPROC   COglExtensions::glGetUniformLocationARB = 0;
PFNGLUNIFORM1IARBPROC            COglExtensions::glUniform1iARB = 0;
PFNGLUNIFORM1FARBPROC            COglExtensions::glUniform1fARB = 0;
PFNGLUNIFORM2FARBPROC            COglExtensions::glUniform2fARB = 0;
PFNGLUNIFORM3FARBPROC            COglExtensions::glUniform3fARB = 0;
PFNGLUNIFORM4FARBPROC            COglExtensions::glUniform4fARB = 0;
PFNGLUNIFORMMATRIX4FVARBPROC     COglExtensions::glUniformMatrix4fv = 0;
PFNGLPROGRAMPARAMETERIEXTPROC    COglExtensions::glProgramParameteriEXT = 0;
PFNGLACTIVETEXTUREPROC			 COglExtensions::glActiveTexture = 0;
PFNGLTEXIMAGE3DEXTPROC           COglExtensions::glTexImage3D = 0;

#if HAS_SHADER_SUBROUTINES
COglExtensions::PFNGLGETSUBROUTINEINDEXPROC		 COglExtensions::glGetSubroutineIndex = 0;
COglExtensions::PFNGLUNIFORMSUBROUTINESUIVPROC	 COglExtensions::glUniformSubroutinesuiv = 0;
#endif

bool COglExtensions::hasVBOSupport()
{
    static bool support = false;
    RETURN_IF_NOT_FIRST_TIME(support);

    assert(wglGetCurrentContext());
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetStringi, PFNGLGETSTRINGIPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glBindBuffer, PFNGLBINDBUFFERPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGenBuffers, PFNGLGENBUFFERSPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glBufferData, PFNGLBUFFERDATAPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glBufferSubData, PFNGLBUFFERSUBDATAPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glIsBuffer, PFNGLISBUFFERPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glDrawRangeElements, PFNGLDRAWRANGEELEMENTSEXTPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetBufferSubData, PFNGLGETBUFFERSUBDATAPROC);

    
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(wglGetExtensionsStringARB, PFNWGLGETEXTENSIONSSTRINGARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetStringi, PFNGLGETSTRINGIPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetShaderiv, PFNGLGETSHADERIVPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetInfoLogARB, PFNGLGETINFOLOGARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glCreateProgramObjectARB, PFNGLCREATEPROGRAMOBJECTARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glCreateShaderObjectARB, PFNGLCREATESHADEROBJECTARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glShaderSourceARB, PFNGLSHADERSOURCEARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glCompileShaderARB, PFNGLCOMPILESHADERARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glAttachObjectARB, PFNGLATTACHOBJECTARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glLinkProgramARB, PFNGLLINKPROGRAMARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glUseProgramObjectARB, PFNGLUSEPROGRAMOBJECTARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetUniformLocationARB, PFNGLGETUNIFORMLOCATIONARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glUniform1iARB, PFNGLUNIFORM1IARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glUniform1fARB, PFNGLUNIFORM1FARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glUniform2fARB, PFNGLUNIFORM2FARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glUniform3fARB, PFNGLUNIFORM3FARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glUniform4fARB, PFNGLUNIFORM4FARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVARBPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glProgramParameteriEXT, PFNGLPROGRAMPARAMETERIEXTPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glActiveTexture, PFNGLACTIVETEXTUREPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glTexImage3D, PFNGLTEXIMAGE3DEXTPROC);


#if HAS_SHADER_SUBROUTINES
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetSubroutineIndex, PFNGLGETSUBROUTINEINDEXPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glUniformSubroutinesuiv, PFNGLUNIFORMSUBROUTINESUIVPROC);
#endif

    support = glBindBuffer && glGenBuffers && glBufferData && glBufferSubData &&
        glIsBuffer && glDeleteBuffers && glDrawRangeElements && glGetBufferParameteriv &&
        glGetBufferSubData;

    return support;
}
