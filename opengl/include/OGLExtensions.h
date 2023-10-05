#pragma once

#ifdef WIN32
#include <Windows.h>
#include <GL/gl.h>
#include <glext.h>
#endif

class COglVboFunctions
{
public:
    static bool hasVBOSupport();    //< pretty much a given, but just in case

protected:

    static PFNGLBINDBUFFERPROC      glBindBuffer;
    static PFNGLGENBUFFERSPROC      glGenBuffers;
    static PFNGLBUFFERDATAPROC      glBufferData;
    static PFNGLBUFFERSUBDATAPROC	glBufferSubData;
    static PFNGLISBUFFERPROC        glIsBuffer;
    static PFNGLDELETEBUFFERSPROC   glDeleteBuffers;
    static PFNGLDRAWRANGEELEMENTSEXTPROC glDrawRangeElements;
    static PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv;
    static PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData;
};

