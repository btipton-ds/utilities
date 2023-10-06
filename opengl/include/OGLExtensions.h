#pragma once

#ifdef WIN32
#include <Windows.h>
#include <GL/gl.h>
#include <glext.h>
#endif

#define HAS_SHADER_SUBROUTINES 1

class COglExtensions
{
public:
    static bool hasVBOSupport();    //< pretty much a given, but just in case

    static PFNGLBINDBUFFERPROC      glBindBuffer;
    static PFNGLGENBUFFERSPROC      glGenBuffers;
    static PFNGLBUFFERDATAPROC      glBufferData;
    static PFNGLBUFFERSUBDATAPROC	glBufferSubData;
    static PFNGLISBUFFERPROC        glIsBuffer;
    static PFNGLDELETEBUFFERSPROC   glDeleteBuffers;
    static PFNGLDRAWRANGEELEMENTSEXTPROC glDrawRangeElements;
    static PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv;
    static PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData;

    //    static PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
    static PFNGLGETSTRINGIPROC			    glGetStringi;
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
    typedef GLuint(APIENTRYP PFNGLGETSUBROUTINEINDEXPROC) (GLuint program, GLenum shadertype, const GLchar* name);
    typedef void (APIENTRYP PFNGLUNIFORMSUBROUTINESUIVPROC) (GLenum shadertype, GLsizei count, const GLuint* indices);
    static PFNGLGETSUBROUTINEINDEXPROC			glGetSubroutineIndex;
    static PFNGLUNIFORMSUBROUTINESUIVPROC			glUniformSubroutinesuiv;
#endif
};

