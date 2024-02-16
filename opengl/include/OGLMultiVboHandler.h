#pragma once

#include <vector>
#include <memory>

#ifdef WIN32
#include <OGLExtensions.h>
#endif

#include <OGLMultiVbo.h>

// Open this file for documentation. It's stored in the include directory with this header file.
// 
// OGLMultiVboHandler.html
//
// If someone can find a way to hyperlink to a relative file, please do that.
class COglShaderBase;

class COglMultiVboHandler : public COglExtensions
{
public:
    struct OGLIndices {
        OGLIndices() = default;
        OGLIndices(const OGLIndices&) = default;
        OGLIndices(size_t batchIndex, const std::vector<unsigned int>& elementIndices);

        void clear();

        std::vector<unsigned int> m_elementIndices;

        size_t m_batchIndex = _SIZE_T_ERROR;   // Pointer to the batch which stores this face's data
        unsigned int m_vertBaseIndex = -1;    // Index of the entity's first vertex index in the batch
        size_t m_numVertsInBatch = 0;          // Number of entity vertices in the batch

        // Memory managment members
        bool m_inUse = true; // Used for mark and sweep garbage collection
        int m_changeNumber = -1;
        size_t m_chunkIdx = _SIZE_T_ERROR; // Index of the first chunk
        size_t m_numChunks = _SIZE_T_ERROR;// Number of chunks
    };

    struct OGLIndex {
        OGLIndex(const OGLIndex&) = default;
        OGLIndex(size_t batchIndex = 0, size_t vertIndex = 0);
        bool operator < (const OGLIndex& rhs) const;

        size_t m_batchIndex = 0, m_vertIndex = 0;
    };

    struct VertexBatch {
        struct ElemIndexMapRec {
            inline ElemIndexMapRec(GLuint texId, const std::vector<unsigned int>& elementIndices)
                : m_texId(texId)
                , m_elementIndices(elementIndices)
            {
            }
            ElemIndexMapRec(const ElemIndexMapRec& src) = default;
            GLuint m_texId = 0;
            std::vector<unsigned int> m_elementIndices;
        };
        VertexBatch(int primitiveType);
        // TODO, in the future it may be beneficial to make this a real class and move some COglMultiVboHandler methods to here.
        // for now, this is pure structure with no code.

        // CPU side representation
        size_t m_nextFreeVertIndex = 0; // This used to make blocks of similar sizes for reuse

        bool m_needsUpdate = true;
        std::vector<float> m_points, m_normals, m_parameters;
        std::vector<rgbaColor> m_colors, m_backColors;
        std::map<int, std::vector<unsigned int>> m_indexMap;
        std::vector<std::shared_ptr<ElemIndexMapRec>> m_texturedFaces;

        std::vector<size_t> m_allocatedChunks; // The index is the chunk number and the value is the number of allocated chunks at that index

        // Video card representation
        COglMultiVBO m_VBO;
    };

    COglMultiVboHandler(int primitiveType, int maxKeyIndex);
    ~COglMultiVboHandler();

    void setShader(const COglShaderBase* pShader);

    // This defines the draw order for keys.
    // Drawing is done in layers, with lowest numbered layer first.
    // Each layer has a set of draw keys and those keys are drawn in that layer
    // This causes draw keys in higher layers to 'paint' over those drawn in lower layers
    // So, if the key for selection is in a higher layer, it will over paint the normal normal version
    void moveKeyToLayer(int key, int layer);

    bool empty() const;
    void clear();
    bool isTriangleVBO() const;

    void beginFaceTesselation();
    // vertiIndices is index pairs into points, normals and parameters to form triangles. It's the standard OGL element index structure
    const OGLIndices* setFaceTessellation(long entityKey, int changeNumber, const std::vector<float>& points, const std::vector<float>& normals, const std::vector<float>& parameters,
        const std::vector<unsigned int>& vertiIndices);
    const OGLIndices* setFaceTessellation(long entityKey, int changeNumber, const std::vector<float>& points, const std::vector<float>& normals, const std::vector<float>& parameters,
        const std::vector<rgbaColor>& colors, const std::vector<unsigned int>& vertiIndices);
    void endFaceTesselation(bool smoothNormals);

    void beginEdgeTesselation();
    const OGLIndices* setEdgeStripTessellation(long entityKey, const std::vector<float>& lineStripPoints);
    const OGLIndices* setEdgeSegTessellation(long entityKey, int changeNumber, const std::vector<float>& points, const std::vector<int>& indices);
    void endEdgeTesselation();

    bool getRawData(long entityKey, std::vector<unsigned int>& indices) const;

    const OGLIndices* getOglIndices(long entityKey) const;

    void beginSettingElementIndices(size_t layerBitMask);
    void includeElementIndices(int key, const OGLIndices& batchIndices, GLuint texId = 0);
    void endSettingElementIndices();
    void draw(int key, COglMultiVBO::DrawVertexColorMode drawColors = COglMultiVBO::DRAW_COLOR_NONE) const;

    // For best speed, we need to bind all the common buffers for a batch, then draw every key that batch uses.
    // This requires a loop inversion. We can't store by keys, because some keys are tiny (like highlighted) and others are huge.
    // Dividing the draw by key doesn't divide the storage effectively.
    // Lambda functions were used for generality in drawAllKeys, but they been known to be too slow. If the lambda functions are too slow, some other callback 
    // mechanism such as functionals or std::function or even a traditional C callback may be needed.

    template<typename PRE_FUNC, typename POST_FUNC, typename PRE_TEX_FUNC, typename POST_TEX_FUNC>
    void drawAllKeys(PRE_FUNC preDrawFunc, POST_FUNC postDrawFunc, PRE_TEX_FUNC preDrawTexFunc, POST_TEX_FUNC postDrawTexFunc) const;

    bool getVert(const COglMultiVboHandler::OGLIndex& glIndicesOut, float coords[3]) const;
    bool getNormal(const COglMultiVboHandler::OGLIndex& glIndicesOut, float coords[3]) const;

    bool getRawData(long entityKey, std::vector<float>& points, std::vector<float>& normals, std::vector<float>& parameters) const;

    bool setColorVBO(long entityKey, std::vector<rgbaColor>& colors);
    bool setBackColorVBO(long entityKey, std::vector<rgbaColor>& colors);

    int findLayerForKey(int key) const;

// Memory management
    struct ChangeRec 
    {
        inline ChangeRec(long entityKey, int changeNumber)
            : m_entityKey(entityKey)
            , m_changeNumber(changeNumber)
        {}

        ChangeRec(const ChangeRec&) = default;
        const long m_entityKey;
        const int m_changeNumber;
    };
    void doGarbageCollection(const std::vector<ChangeRec>& entityKeysInUse);
private:
    struct FreeChunkRecord {
        inline FreeChunkRecord(size_t batchIndex, size_t chunkIndex)
            : m_batchIndex(batchIndex)
            , m_chunkIndex(chunkIndex)
        {
        }

        // m_chunkSizeToBlocksMap is a map of size to address. If you need a block of N chunks, look for entries there which are
        // sorted by size to find the smallest that fits
        size_t m_batchIndex = 0;
        size_t m_chunkIndex = 0;
    };
    using ChunkSizeBlockMap = std::map<size_t, std::vector<FreeChunkRecord>>;

    size_t vertChunkSize() const;
    size_t calcVertChunkIndex(size_t baseIndex) const;
    size_t calcNumVertChunks(size_t numVerts) const;

    // Returns the vertex index in the block.
    // If no released storage is found, batchIndex doesn't change
    // If the data fits in released storage, it is consumed and the batchIndex may change
    void getStorageFor(size_t numVertsNeeded, size_t& batchIndex, size_t& chunkIndex, size_t& blockSizeInChunks);

    void setFaceTessellationInner(size_t batchIndex, size_t chunkIndex, const std::vector<float>& points, const std::vector<float>& normals, const std::vector<float>& parameters,
        const std::vector<rgbaColor>& colors, const std::vector<unsigned int>& triIndices, OGLIndices& glIndicesOut);

    void setEdgeStripTessellationInner(size_t batchIndex, size_t vertChunkIndex, const std::vector<float>& lineStripPts, OGLIndices& glIndicesOut);
    void setEdgeSegTessellationInner(size_t batchIndex, size_t vertChunkIndex, const std::vector<float>& pts, const std::vector<int>& indices, OGLIndices& glIndicesOut);

    // Supporting methods for drawKeys
    void bindCommonBuffers(std::shared_ptr<VertexBatch> batchPtr) const;
    void drawKeyForBatch(int key, std::shared_ptr<VertexBatch> batchPtr, COglMultiVBO::DrawVertexColorMode drawColors) const;
    void drawTexturedFaces(std::shared_ptr<VertexBatch> batchPtr) const;
    void unbindCommonBuffers(std::shared_ptr<VertexBatch> batchPtr) const;
    void initLayerToKeyMap(int maxKeyIndex);

    // This should only be called from doGarbageCollection
    void releaseTessellation(long entityKey);

    bool m_insideBeginFaceTessellation = false;
    bool m_insideBeginEdgeTessellation = false;

    const int m_primitiveType;
    ChunkSizeBlockMap m_chunkSizeToBlocksMap; // The map key is the size of the chunk. The map value is the batchIndex and starting blockNum of the block
    std::vector<bool> m_keysToDraw;
    bool m_clearAllLayers = true;
    size_t m_layerBitMask = 0;
    const COglShaderBase* m_pShader = nullptr;
    std::vector<int> m_keysLayer;
    std::vector<std::vector<int>> m_layersKeys;
    std::vector<std::shared_ptr<VertexBatch>> m_batches;
    std::map<long, std::shared_ptr<OGLIndices>> m_entityKeyToOGLIndicesMap;
};

inline void COglMultiVboHandler::setShader(const COglShaderBase* pShader)
{
    m_pShader = pShader;
}

inline bool COglMultiVboHandler::empty() const
{
    return m_batches.empty();
}

inline const COglMultiVboHandler::OGLIndices* COglMultiVboHandler::getOglIndices(long entityKey) const
{
    auto iter = m_entityKeyToOGLIndicesMap.find(entityKey);
    if (iter != m_entityKeyToOGLIndicesMap.end())
        return iter->second.get();
    return nullptr;
}

inline COglMultiVboHandler::OGLIndex::OGLIndex(size_t batchIndex, size_t vertIndex)
    : m_batchIndex(batchIndex)
    , m_vertIndex(vertIndex)
{
}

inline bool COglMultiVboHandler::OGLIndex::operator < (const OGLIndex& rhs) const
{
    if (m_batchIndex < rhs.m_batchIndex)
        return true;
    else if (m_batchIndex > rhs.m_batchIndex)
        return false;

    return m_vertIndex < rhs.m_vertIndex;
}

