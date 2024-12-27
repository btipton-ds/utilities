
#include <sstream>

#include <assert.h>

#include <OGLMultiVboHandler.h>
#include <OGLShader.h>

#ifdef WIN32
#include <Windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#else
#include "/usr/include/GL/gl.h"
#include "/usr/include/GL/glext.h"
#endif

#define VBO_VALID_UNKNOWN 0
#define VBO_VALID_TRUE 1
#define VBO_VALID_FALSE 2

/*
The batch size is empiracle and dependent on GPU cores, memory, triangle count, how often faces will be edited and more.
Larger VBOs draw faster, smaller can be modified faster IF the bookeeping allows only touching the batches which contain data for the face being modified.
Lets assume that the graphics card has 5000 GPU cores, there are 1,000 faces and each has 1,000 triangles.

If there is one VBO and one draw call for each face, then each call will only draw 1,000 triangles. That can only use 20% of the card's available cores,
so we're only using a fraction of the card's potential. Placing 5 faces in a batch makes each call draw 5,000 triangles which matches the number of cores.

To complicate things further, the GPU is pipelined and there is startup and shutdown overhead, cache stalls etc.

It's more complicated than that.

The game coders will tell you to put everything in one huge VBO and make one draw call. If you never have to modify the contents, all faces are the same color, transform and there is no
chance of GPU memory fragmentation due to deletion and creation of VBOs, they're right. Freeform's use cases are not that simple.

TODO - One obvious refinement is to make each batch VBO a constant size. This reduces (should elliminate) GPU memory fragmentation because a newly allocated VBO exactly fits in a
vacant spot left by the deleted one. Another approach is to allocate a permanent pool of VBOs, never delete them but mark them as 'free' in a reuse pool. As of now, these refinements
aren't needed and the coding time to implement them seems unjustified. If we start having obscure segment faults, adding these refinements is probably the first response.

Another refinement is to our own vertex mangement using partial VBO writing to replace sections(batches) of a VBO. The same labor vs performance tradeoff applies to this and it has not been done yet.

*/

using namespace std;
using namespace OGL;

#ifndef HLOG
#define HLOG(msg)
#endif

MultiVBO::MultiVBO(int m_primitiveType)
    : m_numVerts(0)
    , m_vertexVboID(0)
    , m_normalVboID(0)
    , m_textureVboID(0)
    , m_colorVboID(0)
    , m_backColorVboID(0)
    , m_regionalNormalVboID(0)
    , m_dataID(-1)
    , m_smoothNormals(false)
    , m_regionalNormals(false)
    , m_colors(false)
    , m_backColors(false)
    , m_primitiveType(m_primitiveType)
    , m_valid(VBO_VALID_UNKNOWN)
{

}

MultiVBO::~MultiVBO()
{
    HLOG(_T("~MultiVBO()"));

#ifdef WIN32
    if (!wglGetCurrentContext())
    {
        assert(!"Bad gl context");
        // Can crash in glIsBuffer if the current context is bad.
        HLOG(_T("SKIPPED ~MultiVBO() Ogl cleanup... there is no gl context"));
        return;
    }
#endif

    releaseVBOs();
}

MultiVBO::ElementVBORec::ElementVBORec()
    : m_mumElements(0)
    , m_elementIdxVboID(0)
{
    int isValid;
    createVBO(m_elementIdxVboID, isValid);
}

MultiVBO::ElementVBORec::ElementVBORec(const ElementVBORec& src)
    : m_mumElements(src.m_mumElements)
    , m_elementIdxVboID(src.m_elementIdxVboID)
    , m_isLiveInstance(src.m_isLiveInstance)
{
    src.m_isLiveInstance = false; // Mark it as copied out of so it won't be destroyed
}

MultiVBO::ElementVBORec::~ElementVBORec()
{
    if (m_isLiveInstance && m_elementIdxVboID != 0) {
        int isValid;
        releaseVBO(m_elementIdxVboID, isValid);
    }
}

MultiVBO::ElementVBORec& MultiVBO::ElementVBORec::operator = (const ElementVBORec& src)
{
    if (src.m_isLiveInstance) {
        m_isLiveInstance = true;
        src.m_isLiveInstance = false;

        m_mumElements = src.m_mumElements;
        m_elementIdxVboID = src.m_elementIdxVboID;
    } else {
        m_isLiveInstance = false;
    }
    return *this;
}

void MultiVBO::ElementVBORec::bind(const vector<unsigned int>& indices)
{
    m_mumElements = indices.size();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementIdxVboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);  

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void MultiVBO::releaseKeysElementVBO(int key)
{
    auto iter = m_elementVBOIDMap.find(key);
    if (iter != m_elementVBOIDMap.end()) {
        m_elementVBOIDMap.erase(iter);
    }
}

void MultiVBO::releaseVBOs()
{
    HLOG(_T("MultiVBO::releaseVBOs()"));
    releaseVBO(m_vertexVboID, m_valid);
    releaseVBO(m_normalVboID, m_valid);
    releaseVBO(m_textureVboID, m_valid);
    releaseVBO(m_colorVboID, m_valid);
    releaseVBO(m_backColorVboID, m_valid);
    releaseVBO(m_regionalNormalVboID, m_valid);
    m_elementVBOIDMap.clear();

    m_numVerts = 0;
    m_valid = VBO_VALID_UNKNOWN;
}

bool MultiVBO::canFitInVboSpace(int numTriangles)
{
    //TBD
    return true;
}

bool MultiVBO::isInitialized() const
{
#ifdef WIN32
    if (!hasVBOSupport() || !wglGetCurrentContext())
        return false;
#endif

    return glIsBuffer(m_vertexVboID) && glIsBuffer(m_normalVboID);
}

bool MultiVBO::createVBO(GLuint& vboID, int& valid)
{
#ifdef WIN32
    if (!hasVBOSupport() || !wglGetCurrentContext())
        return false;
#endif

    assert(!vboID);

    if (vboID && glIsBuffer(vboID))
        return true;

    valid = VBO_VALID_UNKNOWN;
    glGenBuffers(1, &vboID);
    HLOG(Format(_T("glGenBuffers VBO %d"), vboID));
    assert(vboID != 0);

    // A name returned by glGenBuffers, but not yet associated with a buffer object 
    // by calling glBindBuffer, is not the name of a buffer object.
    // NVidia returns true to glIsBuffer here but actually it shouldn't... ATI returns false 
    return vboID != 0;// glIsBuffer(vboID);
}

void MultiVBO::releaseVBO(GLuint& vboID, int& valid)
{
    valid = false;

#ifdef WIN32
    HLOG(_T("MultiVBO::releaseVBO"));
    if (!hasVBOSupport() || !wglGetCurrentContext())
    {
        HLOG(_T("Skipping MultiVBO::releaseVBO()"));
        return;
    }
#endif

    if (vboID)
    {
        if (isValid_unbindsVBO(vboID)) {
            glDeleteBuffers(1, &vboID); 

            HLOG(Format(_T("MultiVBO::releaseVBO: deleted buffer %d"), vboID));
        }
        else {
            HLOG(Format(_T("MultiVBO::releaseVBO: %d is not a valid buffer"), vboID));
            assert(!"vboID != 0 BUT glIsBuffer(vboID) returned false. This should not be possible and is a serious fault.");
            // However, it's not serious enough to justify terminating the app and losing work.
        }
    }
    vboID = 0;
}

bool MultiVBO::isValid_unbindsVBO(GLuint& vboID)
{
    if (vboID) {
        glBindBuffer(GL_ARRAY_BUFFER, vboID);
        if (glIsBuffer(vboID)) {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            return true;
        }
    }
    return false;
}

bool MultiVBO::copyToVBO(const vector<float>& verts, const vector<float>& colors, int dataID)
{
    m_valid = VBO_VALID_UNKNOWN;
    vector<float> norms, tex;
    return copyToVBO(verts, norms, false, tex, colors, dataID);
}

bool MultiVBO::copyToVBO(const vector<float>& verts, int dataID)
{
    m_valid = VBO_VALID_UNKNOWN;
    vector<float> norms, tex;
    vector<float> colors;
    return copyToVBO(verts, norms, true, tex, colors, dataID);
}

bool MultiVBO::copyToVBO(const vector<float>& verts, const vector<float>& normals, bool smoothNrmls, const vector<float>& textureCoords, int dataID)
{
    m_valid = VBO_VALID_UNKNOWN;
    vector<float> colors;
    return copyToVBO(verts, normals, smoothNrmls, textureCoords, colors, dataID);
}

bool MultiVBO::copyToVBO(const vector<float>& verts, const vector<float>& normals, bool smoothNrmls, const vector<float>& textureCoords, const vector<float>& colors, int id)
{
    m_valid = VBO_VALID_UNKNOWN;
#ifdef WIN32
    if (!hasVBOSupport() || !wglGetCurrentContext())
        return false;
#endif

    assert(!verts.empty());

    size_t size = verts.size();
    assert(!(size % 3)); //size should be multiple of 3 

    bool vboValid1 = assureVBOValid(verts, m_vertexVboID, m_valid);
    bool vboValid2 = normals.empty() || assureVBOValid(normals, m_normalVboID, m_valid);
    bool vboValid3 = textureCoords.empty() || assureVBOValid(textureCoords, m_textureVboID, m_valid);
    bool vboValid4 = colors.empty() || assureVBOValid(colors, m_colorVboID, m_valid);

    if (!vboValid1 || !vboValid2 || !vboValid3 || !vboValid4)
    {
        releaseVBOs();
        return false;
    }

    m_numVerts = size / 3;

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexVboID);
    glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), verts.data(), GL_STATIC_DRAW);  
    HLOG(Format(_T("Copied %.1f MB to vbo %d"), size * sizeof(float) / 1048576.0f, m_vertexVboID));

    if (m_primitiveType == GL_TRIANGLES) {
        if (m_normalVboID)
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_normalVboID);
            glBufferData(GL_ARRAY_BUFFER, size * sizeof(float), normals.data(), GL_STATIC_DRAW);  
            HLOG(Format(_T("Copied %.1f MB to vbo %d"), size * sizeof(float) / 1048576.0f, m_normalVboID));
        }
    }
    m_smoothNormals = smoothNrmls;

    if (!colors.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_colorVboID);
        glBufferData(GL_ARRAY_BUFFER, (m_numVerts * 3 * sizeof(float)), colors.data(), GL_STATIC_DRAW); 
        HLOG(Format(_T("Copied %.1f MB to vbo %d"), (m_numVerts * 3 * sizeof(float)) / 1048576.0f, m_colorVboID));
    }

    if (!textureCoords.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_textureVboID);
        glBufferData(GL_ARRAY_BUFFER, (m_numVerts * 2 * sizeof(float)), textureCoords.data(), GL_STATIC_DRAW); 
        HLOG(Format(_T("Copied %.1f MB to vbo %d"), (m_numVerts * 2 * sizeof(float)) / 1048576.0f, m_textureVboID));
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);   

    m_dataID = id;

    return true;
}

bool MultiVBO::copyVertecesToExistingVBO(const vector<float>& verts)
{
    m_valid = VBO_VALID_UNKNOWN;
    size_t numFloats = verts.size();
    if (m_numVerts * 3 != numFloats)
    {
        assert(!"vbo array size mismatch");
        return false;
    }

    if (!m_vertexVboID)
    {
        assert(!"no normal vbo to copy to");
        return false;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexVboID);
    glBufferData(GL_ARRAY_BUFFER, numFloats * sizeof(float), verts.data(), GL_STATIC_DRAW);  

    glBindBuffer(GL_ARRAY_BUFFER, 0);   

    return true;
}

bool MultiVBO::copyNormalsToExistingVBO(const vector<float>& normals, bool smooth)
{
    m_valid = VBO_VALID_UNKNOWN;
    size_t numFloats = normals.size();
    if (m_numVerts * 3 != numFloats)
    {
        assert(!"vbo array size mismatch");
        return false;
    }

    if (!m_normalVboID)
    {
        assert(!"no normal vbo to copy to");
        return false;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_normalVboID);
    glBufferData(GL_ARRAY_BUFFER, numFloats * sizeof(float), normals.data(), GL_STATIC_DRAW);  
    m_smoothNormals = smooth;

    glBindBuffer(GL_ARRAY_BUFFER, 0);   

    return true;
}

bool MultiVBO::reverseNormals()
{
    m_valid = VBO_VALID_UNKNOWN;
    glBindBuffer(GL_ARRAY_BUFFER, m_normalVboID);
    GLint size;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    vector<unsigned char> buf;
    buf.resize(size);
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, &buf[0]);
    float* pv = (float*)&buf[0];
    int num = size / sizeof(float);
    for (int i = 0; i < num; i++)
        pv[i] = -pv[i];

    glBufferSubData(GL_ARRAY_BUFFER, 0, size, &buf[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

bool MultiVBO::copyColorsToExistingVBO(const vector<float>& colors)
{
    m_valid = VBO_VALID_UNKNOWN;
    if (m_numVerts != colors.size())
    {
        assert(!"vbo array size mismatch");
        return false;
    }

    if (!m_colorVboID)
    {
        createVBO(m_colorVboID, m_valid);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_colorVboID);   
    glBufferData(GL_ARRAY_BUFFER, (m_numVerts * sizeof(float)), colors.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);   

    return true;
}

bool MultiVBO::copyBackColorsToExistingVBO(const vector<float>& backColors)
{
    m_valid = VBO_VALID_UNKNOWN;
    if (m_numVerts != backColors.size())
    {
        assert(!"vbo array size mismatch");
        return false;
    }

    if (!m_backColorVboID)
    {
        createVBO(m_backColorVboID, m_valid);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_backColorVboID);   
    glBufferData(GL_ARRAY_BUFFER, (m_numVerts * sizeof(unsigned int)), backColors.data(), GL_STATIC_DRAW);  

    glBindBuffer(GL_ARRAY_BUFFER, 0);   

    return true;
}

void MultiVBO::setUseSmoothNormal(bool set)
{
    m_valid = VBO_VALID_UNKNOWN;
    if (!m_normalVboID || !glIsBuffer(m_normalVboID))
    {
        // actually we could just create it here instead
        assert(!"can't set smooth normals without normal data");
        return;
    }

    m_smoothNormals = set;
}

void MultiVBO::setUseRegionalNormal(bool set)
{
    m_valid = VBO_VALID_UNKNOWN;
    if (!m_normalVboID || !glIsBuffer(m_normalVboID))
    {
        assert(!"can't set regional normals without regular normals");
        return;
    }

    m_regionalNormals = m_numVerts ? set : false;

    if (set && (!m_regionalNormalVboID || !glIsBuffer(m_regionalNormalVboID)))
    {
        createVBO(m_regionalNormalVboID, m_valid);

        glBindBuffer(GL_ARRAY_BUFFER, m_regionalNormalVboID);
        glBufferData(GL_ARRAY_BUFFER, m_numVerts * 3 * sizeof(float), 0, GL_STATIC_DRAW);  
    }
}

bool MultiVBO::setIndexVBO(int key, const vector<unsigned int>& indices)
{
#ifdef WIN32
    if (!hasVBOSupport() || !wglGetCurrentContext())
#else
    if (!hasVBOSupport())
#endif
        return false;
    auto iter = m_elementVBOIDMap.find(key);
    if (iter == m_elementVBOIDMap.end()) {
        ElementVBORec er;
        iter = m_elementVBOIDMap.insert(make_pair(key, er)).first;
    }

    // An empty list is a normal condition
    if (indices.empty())
        return true;

    iter->second.bind(indices);

    return true;
}

bool MultiVBO::clearIndexVBO()
{
    m_elementVBOIDMap.clear();
    return true;
}

bool MultiVBO::drawVBO(const ShaderBase* pShader, int key, DrawVertexColorMode drawColors) const
{
    auto iter = m_elementVBOIDMap.find(key);
    if (iter != m_elementVBOIDMap.end()) {
        // Calling draw on an empty list is an expected condition.
        GLsizei numElements = (GLsizei)iter->second.getNumElements();
        if (numElements <= 0) {
            return true;
        }

        GLuint elementIdxVboID = iter->second.getVboId();

        return drawVBO(pShader, numElements, elementIdxVboID, drawColors);

    }

    return false;
}

bool MultiVBO::areVBOsValid(size_t numElements, GLuint elementIdxVboID, DrawVertexColorMode drawColors) const
{
#ifdef WIN32
    if (!hasVBOSupport() || !wglGetCurrentContext())
        return false;
#endif

    if (m_valid != VBO_VALID_TRUE) {
        m_valid = VBO_VALID_FALSE;
        if (!m_numVerts)
            return false;

#ifdef WIN32
        if (!hasVBOSupport() || !wglGetCurrentContext())
#else
        if (!hasVBOSupport())
#endif
            return false;

        if (!glIsBuffer(m_vertexVboID))
            return false;

        if (m_normalVboID && !glIsBuffer(m_normalVboID))
            return false;

        if (m_textureVboID && !glIsBuffer(m_textureVboID))
            return false;

        if (numElements && !glIsBuffer(elementIdxVboID))
            return false;

        if (drawColors == DRAW_COLOR && m_colorVboID && !glIsBuffer(m_colorVboID))
            return false;

        if (drawColors == DRAW_COLOR_BACK && m_backColorVboID && !glIsBuffer(m_backColorVboID))
            return false;

        if (!m_numVerts)
            return false;
        m_valid = VBO_VALID_TRUE;
    }

    return true;
}

bool MultiVBO::bindCommon(const ShaderBase* pShader, size_t numElements) const
{
    //bind the verteces
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexVboID);       GL_ASSERT;
    glEnableVertexAttribArray(pShader->getVertexLoc()); GL_ASSERT;
    glVertexAttribPointer(pShader->getVertexLoc(), 3, GL_FLOAT, 0, 0, 0); GL_ASSERT;

    //bind the normals
    if (m_normalVboID)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_normalVboID);      GL_ASSERT;
        glEnableVertexAttribArray(pShader->getNormalLoc()); GL_ASSERT;
        glVertexAttribPointer(pShader->getNormalLoc(), 3, GL_FLOAT, 0, 0, 0); GL_ASSERT;
    }

    if (m_textureVboID && pShader->getTexParamLoc() != -1)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_textureVboID);   GL_ASSERT;
        glEnableVertexAttribArray(pShader->getTexParamLoc()); GL_ASSERT;
        glVertexAttribPointer(pShader->getTexParamLoc(), 2, GL_FLOAT, 0, 0, 0); GL_ASSERT;
    }

    return true;
}

void MultiVBO::unbindCommon() const
{
    // revert state
    glDisableClientState(GL_VERTEX_ARRAY);            GL_ASSERT;

    if (m_normalVboID) {
        glDisableClientState(GL_NORMAL_ARRAY); GL_ASSERT;
    }
    if (m_textureVboID) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY); GL_ASSERT;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);  GL_ASSERT;
}

template<class T>
inline bool MultiVBO::assureVBOValid(const vector<T>& vec, GLuint& vboID, int& valid)
{
    if (vboID) {
        if (vec.empty()) {
            glDeleteBuffers(1, &vboID); 
            vboID = 0;
            return true;
        }
        else
            return glIsBuffer(vboID);
    }
    else {
        if (vec.empty())
            return true;
        else {
            return createVBO(vboID, valid);
        }
    }
}

bool MultiVBO::drawVBO(const ShaderBase* pShader, GLsizei numElements, GLuint elementIdxVboID, DrawVertexColorMode drawColors) const
{
    if (drawColors == DRAW_COLOR_SKIP)
        return true;

    bool drawingColors = false;
    int priorCulling = -1;

    glPushAttrib(GL_ALL_ATTRIB_BITS);                GL_ASSERT;
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT); GL_ASSERT;

    // Enable vertex and normal arrays
    if (!areVBOsValid(numElements, elementIdxVboID, drawColors) || !bindCommon(pShader, numElements)) {
        assert(!"MultiVBO not valid");
        areVBOsValid(numElements, elementIdxVboID, drawColors); GL_ASSERT;
        bindCommon(pShader, numElements); GL_ASSERT;
        return false;
    }

    if (m_colorVboID && drawColors == DRAW_COLOR) {
        glEnable(GL_COLOR_MATERIAL); GL_ASSERT;
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); GL_ASSERT;
        glEnableClientState(GL_COLOR_ARRAY); GL_ASSERT;
        glEnableVertexAttribArray(pShader->getColorLoc()); GL_ASSERT;
        glVertexAttribPointer(pShader->getColorLoc(), 3, GL_FLOAT, 0, 0, 0); GL_ASSERT;
    }
#if 0	// Good chance something similar to this is necessary,
		// but I haven't yet found the right formulation to get
		// the front and back passes to blend correctly.
    else if (m_backColorVboID && drawColors == DRAW_COLOR_BACK) {
        // Not sure if settings should be the same ?
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnableClientState(GL_COLOR_ARRAY);
    }
#endif

    if (drawColors == DRAW_COLOR && m_colorVboID && pShader->getColorLoc() != -1)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_colorVboID);     GL_ASSERT;
        glVertexAttribPointer(pShader->getColorLoc(), 3, GL_FLOAT, 0, 0, 0); GL_ASSERT;

        drawingColors = true;
    }
    else if (drawColors == DRAW_COLOR_BACK && m_backColorVboID && pShader->getColorLoc() != -1)
    {
        glGetIntegerv(GL_CULL_FACE_MODE, &priorCulling); GL_ASSERT;
        glCullFace(GL_BACK); GL_ASSERT;
        glBindBuffer(GL_ARRAY_BUFFER, m_backColorVboID);     GL_ASSERT;
        glVertexAttribPointer(pShader->getColorLoc(), 3, GL_FLOAT, 0, 0, 0); GL_ASSERT;

        drawingColors = true;
    }

    // Render the triangles
    if (numElements && (m_primitiveType == GL_TRIANGLES || m_primitiveType == GL_QUADS || m_primitiveType == GL_LINES || m_primitiveType == GL_LINE_STRIP))
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementIdxVboID);       GL_ASSERT;
        glDrawElements(m_primitiveType, numElements, GL_UNSIGNED_INT, 0); GL_ASSERT;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); GL_ASSERT;
    }
    else
    {
        glDrawArrays(m_primitiveType, 0, (GLsizei)m_numVerts);      GL_ASSERT;
    }

    unbindCommon();

    if (drawingColors)
        glDisableClientState(GL_COLOR_ARRAY);

    if (priorCulling != -1)
        glCullFace(priorCulling);

    glPopAttrib();
    glPopClientAttrib();                                GL_ASSERT;

    return true;
}

bool MultiVBO::drawVBO(const ShaderBase* pShader, const vector<unsigned int>& indices, DrawVertexColorMode drawColors) const
{
    assert(m_valid != VBO_VALID_FALSE);
    if (m_valid == VBO_VALID_FALSE)
        return false;

    int priorCulling = -1;

    if (m_valid == VBO_VALID_UNKNOWN) {
        m_valid = VBO_VALID_FALSE;
        if (!m_numVerts)
            return false;

#ifdef WIN32
        if (!hasVBOSupport() || !wglGetCurrentContext())
#else
        if (!hasVBOSupport())
#endif
            return false;

        if (!glIsBuffer(m_vertexVboID))
            return false;

        if (m_normalVboID && !glIsBuffer(m_normalVboID))
            return false;

        if (m_textureVboID && !glIsBuffer(m_textureVboID))
            return false;

        if (drawColors == DRAW_COLOR && m_colorVboID && !glIsBuffer(m_colorVboID))
            return false;

        if (drawColors == DRAW_COLOR_BACK && m_backColorVboID && !glIsBuffer(m_backColorVboID))
            return false;

        if (!m_numVerts)
            return false;
        m_valid = VBO_VALID_TRUE;
    }
    glPushAttrib(GL_ALL_ATTRIB_BITS);                GL_ASSERT;
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

    // Enable vertex and normal arrays
    glEnableClientState(GL_VERTEX_ARRAY);            GL_ASSERT;

    if (m_normalVboID)
        glEnableClientState(GL_NORMAL_ARRAY);
    if (m_textureVboID)
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    if ((m_colorVboID && (drawColors == DRAW_COLOR)) || (m_backColorVboID && (drawColors == DRAW_COLOR_BACK)))
        glEnableClientState(GL_COLOR_ARRAY);

    //bind the normals
    if (m_normalVboID)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_normalVboID);      GL_ASSERT;
        glNormalPointer(GL_FLOAT, 0, 0);
    }

    //bind the verteces
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexVboID);       GL_ASSERT;
    glVertexPointer(3, GL_FLOAT, 0, 0);

    if (m_colorVboID && (drawColors == DRAW_COLOR))
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_colorVboID);     GL_ASSERT;
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);
    }

    else if (m_backColorVboID && (drawColors == DRAW_COLOR_BACK))
    {
        glGetIntegerv(GL_CULL_FACE_MODE, &priorCulling);
        glCullFace(GL_BACK);
        glBindBuffer(GL_ARRAY_BUFFER, m_backColorVboID);     GL_ASSERT;
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);
    }

#if 0
    if (m_textureVboID)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_textureVboID);   GL_ASSERT;
        glTexCoordPointer(2, GL_FLOAT, 0, 0);
    }

#endif

    // Render the triangles
    if (indices.size() > 0 && m_primitiveType == GL_TRIANGLES)
    {
        glDrawElements(m_primitiveType, (GLsizei)indices.size(), GL_UNSIGNED_INT, indices.data()); GL_ASSERT;
    }
    else
    {
        glDrawArrays(m_primitiveType, 0, (GLsizei)m_numVerts);      GL_ASSERT;
    }

    // revert state
    glDisableClientState(GL_VERTEX_ARRAY);            GL_ASSERT;

    if (m_normalVboID)
        glDisableClientState(GL_NORMAL_ARRAY);
    if (m_textureVboID)
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    if (m_colorVboID)
        glDisableClientState(GL_COLOR_ARRAY);

    glBindBuffer(GL_ARRAY_BUFFER, 0);                  GL_ASSERT;

    if (priorCulling != -1)
        glCullFace(priorCulling);

    glPopAttrib();
    glPopClientAttrib();                                GL_ASSERT;

    return true;
}

