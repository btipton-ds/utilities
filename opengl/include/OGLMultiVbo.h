#pragma once

#include <GL/gl.h>

#include <vector>
#include <map>
#include <memory>
#include <limits.h>

#include <OGLExtensions.h>
#define _SIZE_T_ERROR SIZE_MAX

// Open this file for documentation. It's stored in the include directory with this header file.
// 
// OGLMultiVboHandler.html
//
// If someone can find a way to hyperlink to a relative file, please do that.

class COglMultiVBO : public COglVboFunctions
{
    friend class COglMultiVboHandler;
public:
    enum DrawVertexColorMode {
        DRAW_COLOR_NONE,
        DRAW_COLOR,
        DRAW_COLOR_BACK,
    };
    static bool createVBO(GLuint& vboID, int& valid);
    static void releaseVBO(GLuint& vboID, int& valid);

    // There is an inconsistency between the OGL standard, Nvidia and ATI cards. For consistency, the buffer must be bound in order to confirm it's valid
    // This function binds the buffer (to an arbitrary source type), tests validity and unbinds it. It always works, but it has the side effect of unbinding
    // the buffer.
    static bool isValid_unbindsVBO(GLuint& vboID);

    COglMultiVBO(int m_primitiveType);
    virtual ~COglMultiVBO();

    void dumpToStream(std::ostream& out) const;

    virtual bool copyToVBO(const std::vector<float>& verts, const std::vector<float>& normals, bool smoothNrmls, const std::vector<float>& textureCoords, int dataID = 0);    ///replaces whatever was there before, normal size == vertex size && tex size is 2/3 of vertex size or has no size
    virtual bool copyToVBO(const std::vector<float>& verts, const std::vector<float>& normals, bool smoothNrmls, const std::vector<float>& textureCoords, std::vector<unsigned int>& colors, int dataID = 0);    ///replaces whatever was there before, normal size == vertex size && tex size is 2/3 of vertex size or has no size
    virtual bool copyToVBO(const std::vector<float>& verts, int dataID = 0);    ///replaces whatever was there before, normal size == vertex size && tex size is 2/3 of vertex size or has no size

    // For best performance, the object keeps large, complete VBOs for all the geometry (verts, normals etc) and the caller draws from that VBO using 
    // element id lists. The lists can be in the form of a raw list or element VBO ids. This object maintains a list of elementVBO ids and it is also possible for
    // the user to keep their own lists separately.
    // sets the indices for the given listNum. It will create the entry if it's out of bounds.
    // It replaces the current contents and rewrites the VBO
    virtual bool setIndexVBO(int key, const std::vector<unsigned int>& indices);
    virtual bool clearIndexVBO();

    // This call extracts the element count and VBO id for an index, then calls drawVBO(GLuint numElements, GLuint indexVBOId, bool drawColors) using the result.
    // If drawColors = DRAW_COLOR_NONE, color array buffer is not used
    // If drawColors = DRAW_COLOR, color array buffer is used
    // If drawColors = DRAW_COLOR_BACK, cull face is set to back face and back color array buffer is used
    virtual bool drawVBO(int key, DrawVertexColorMode drawColors = DRAW_COLOR_NONE) const;
    virtual bool drawVBO(GLsizei numElements, GLuint indexVBOId, DrawVertexColorMode drawColors = DRAW_COLOR_NONE) const;
    virtual bool drawVBO(const std::vector<unsigned int>& indices = std::vector<unsigned int>(), DrawVertexColorMode drawColors = DRAW_COLOR_NONE) const;

    bool usingSmoothNormals() { return m_smoothNormals; }
    bool usingRegionalNormals() { return m_regionalNormals; }
    bool usingColors() { return m_colors; }
    bool usingBackColors() { return m_backColors; }

    void setUseSmoothNormal(bool set);
    virtual void setUseRegionalNormal(bool set);

    bool copyVertecesToExistingVBO(const std::vector<float>& verts);

    bool copyNormalsToExistingVBO(const std::vector<float>& normals, bool smooth);

    bool reverseNormals();

    bool copyColorsToExistingVBO(const std::vector<unsigned int>& colors);
    bool copyBackColorsToExistingVBO(const std::vector<unsigned int>& backColors);

    bool isInitialized() const;
    int  dataID() const { return m_dataID; } //<  the id passed in when the vbo was created
    void invalidate() { m_dataID = -1; } //<  don't blow away the memory or cardID, just be sure data is refreshed

    static bool canFitInVboSpace(int numTriangles); //< a guess as to whether this size mesh can be fit on the card without having it thrash

    size_t numVerts() const { return m_numVerts; }
    GLuint vertexVboID() const { return m_vertexVboID; }
    GLuint normalVboID() const { return m_normalVboID; }
    GLuint textureVboID() const { return m_textureVboID; }
    GLuint regionalVboID() { setUseRegionalNormal(true); return m_regionalNormalVboID; }
    GLuint colorVboID() { return m_colorVboID; }
    GLuint backColorVboID() { return m_backColorVboID; }

    virtual void releaseVBOs();
    virtual void releaseKeysElementVBO(int key);

    bool getVBOArray(GLuint vboId, std::vector<float>& values) const;
    bool getVBOArray(GLuint vboId, std::vector<unsigned int>& values) const;
protected:
    bool areVBOsValid(size_t numElements, GLuint elementIdxVboID, DrawVertexColorMode drawColors) const;
    bool bindCommon(size_t numElements) const;
    void unbindCommon() const;
    template<class T>
    static bool assureVBOValid(const std::vector<T>& vec, GLuint& vboID, int& valid);

    class ElementVBORec {
    public:
        ElementVBORec();
        ElementVBORec(const ElementVBORec& src);
        ~ElementVBORec();

        ElementVBORec& operator = (const ElementVBORec& src);

        void bind(const std::vector<unsigned int>& indices);
        size_t getNumElements() const;
        GLuint getVboId() const;

    private:
        mutable bool m_isLiveInstance = true; // This class should be a singlelton, but it gets copied, destroyed etc. This token moves with the VBOID
        size_t  m_mumElements;   //< Only relevant if indeces were passed in
        GLuint  m_elementIdxVboID;
    };

    const int m_primitiveType;

    size_t m_numVerts;
    int    m_dataID;
    bool   m_smoothNormals; //< Need to keep track of which normals we're using
    bool   m_regionalNormals;
    bool   m_colors;
    bool   m_backColors;
    mutable int   m_valid;

    GLuint m_vertexVboID;
    GLuint m_normalVboID;
    GLuint m_regionalNormalVboID;
    GLuint m_textureVboID;
    GLuint m_colorVboID;
    GLuint m_backColorVboID;
    std::map<int, ElementVBORec> m_elementVBOIDMap;
};

inline size_t COglMultiVBO::ElementVBORec::getNumElements() const
{
    return m_mumElements;
}

inline GLuint COglMultiVBO::ElementVBORec::getVboId() const
{
    return m_elementIdxVboID;
}
