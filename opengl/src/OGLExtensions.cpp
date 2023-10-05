#include <OGLExtensions.h>
#include <assert.h>

#ifdef WIN32
#include <tchar.h>
#include <Windows.h>
#include <GL/gl.h>

#define GET_EXT_POINTER(name, type) name = (type)wglGetProcAddress(#name)
#define GET_EXT_POINTER_MESSAGE_ONCE(name, type)         {                      static bool first=true; if(first){ first=false; name = (type)wglGetProcAddress(#name); if(!name){ }}}
#define GET_EXT_POINTER_MESSAGE_RETURN_VOID(name, type)  { static bool ok=true; static bool first=true; if(first){ first=false; name = (type)wglGetProcAddress(#name); if(!name){ ok=false; return; }}      if(!ok)return;}
#define GET_EXT_POINTER_MESSAGE_RETURN_FALSE(name, type) { static bool ok=true; static bool first=true; if(first){ first=false; name = (type)wglGetProcAddress(#name); if(!name){ ok=false; return false; }} if(!ok) return false;}
#else
#endif // WIND32


#define	RETURN_IF_NOT_FIRST_TIME(a) static bool first=true; if(!first) return a; first = false;



PFNGLBINDBUFFERPROC      COglVboFunctions::glBindBuffer = 0;
PFNGLGENBUFFERSPROC      COglVboFunctions::glGenBuffers = 0;
PFNGLBUFFERDATAPROC      COglVboFunctions::glBufferData = 0;
PFNGLBUFFERSUBDATAPROC   COglVboFunctions::glBufferSubData = 0;
PFNGLISBUFFERPROC        COglVboFunctions::glIsBuffer = 0;
PFNGLDELETEBUFFERSPROC   COglVboFunctions::glDeleteBuffers = 0;
PFNGLDRAWRANGEELEMENTSEXTPROC COglVboFunctions::glDrawRangeElements = 0;
PFNGLGETBUFFERPARAMETERIVPROC COglVboFunctions::glGetBufferParameteriv = 0;
PFNGLGETBUFFERSUBDATAPROC COglVboFunctions::glGetBufferSubData = 0;


bool COglVboFunctions::hasVBOSupport()
{
    static bool support = false;
    RETURN_IF_NOT_FIRST_TIME(support);

    assert(wglGetCurrentContext());
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glBindBuffer, PFNGLBINDBUFFERPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGenBuffers, PFNGLGENBUFFERSPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glBufferData, PFNGLBUFFERDATAPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glBufferSubData, PFNGLBUFFERSUBDATAPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glIsBuffer, PFNGLISBUFFERPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glDrawRangeElements, PFNGLDRAWRANGEELEMENTSEXTPROC);

    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC);
    GET_EXT_POINTER_MESSAGE_RETURN_FALSE(glGetBufferSubData, PFNGLGETBUFFERSUBDATAPROC);

    support = glBindBuffer && glGenBuffers && glBufferData && glBufferSubData &&
        glIsBuffer && glDeleteBuffers && glDrawRangeElements && glGetBufferParameteriv &&
        glGetBufferSubData;

    return support;
}
