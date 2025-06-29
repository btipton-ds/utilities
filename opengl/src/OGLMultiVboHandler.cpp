
#include <sstream>
#include <assert.h>
#include <set>

#include <OGLMultiVboHandler.h>

#define VBO_VALID_UNKNOWN 0
#define VBO_VALID_TRUE 1
#define VBO_VALID_FALSE 2

/* 
The batch size is empirical and dependent on GPU cores, memory, triangle count, how often faces will be edited and more.
Larger VBOs draw faster, smaller can be modified faster IF the bookeeping allows only touching the batches which contain data for the face being modified.
Lets assume that the graphics card has 5000 GPU cores, there are 1,000 faces and each has 1,000 triangles.

If there is one VBO and one draw call for each face, then each call will only draw 1,000 triangles. That can only use 20% of the card's available cores, 
so we're only using a fraction of the card's potential. Placing 5 faces in a batch makes each call draw 5,000 triangles which matches the number of cores.

To complicate things further, the GPU is pipelined and there is startup and shutdown overhead, cache stalls etc.

It's more complicated than that.

The game coders will tell you to put everything in one huge VBO and make one draw call. If you never have to modify the contents, all faces are the same color, transform and there is no 
chance of GPU memory fragmentation due to deletion and creation of VBOs, they're right. Freeform's use cases are not that simple.

TODO - One obvious refinement is to make each batch VBO a constant size. This reduces (should eliminate) GPU memory fragmentation because a newly allocated VBO exactly fits in a 
vacant spot left by the deleted one. Another approach is to allocate a permanent pool of VBOs, never delete them but mark them as 'free' in a reuse pool. As of now, these refinements 
aren't needed and the coding time to implement them seems unjustified. If we start having obscure segment faults, adding these refinements is probably the first response.

Another refinement is to our own vertex mangement using partial VBO writing to replace sections(batches) of a VBO. The same labor vs performance tradeoff applies to this and it has not been done yet.

*/

#define OGL_FACE_BATCH_VERT_COUNT_MAX_K 128
#define OGL_EDGE_BATCH_VERT_COUNT_MAX_K 16

using namespace std;
using namespace OGL;

MultiVboHandler::MultiVboHandler(int primitiveType, int maxKeyIndex)
    : m_primitiveType(primitiveType)
{
    initLayerToKeyMap(maxKeyIndex);
}

void MultiVboHandler::initLayerToKeyMap(int maxKeyIndex)
{
    m_layersKeys.resize(1);
    for (int i = 0; i < maxKeyIndex; i++) {
        m_keysLayer.push_back(0);
        m_layersKeys[0].push_back(i);
    }
}

inline int MultiVboHandler::findLayerForKey(int key) const
{
    return m_keysLayer[key];
}

void MultiVboHandler::doGarbageCollection(const std::vector<ChangeRec>& entityIdsInUse)
{
    for (const auto& pair : m_entityIdToIndicesMap) {
        auto& subMap = pair.second;
        for (const auto& pair1 : subMap) {
            pair1.second->m_inUse = false;
        }
    }

    for (const auto& changeRec : entityIdsInUse) {
        auto iter = m_entityIdToIndicesMap.find(changeRec.m_entityId);
        if (iter != m_entityIdToIndicesMap.end()) {
            // Only keep it if it's the same id AND it has not been changed
            auto& subMap = iter->second;
            for (const auto& pair1 : subMap) {
                pair1.second->m_inUse = true;
            }
        }
    }

    vector<size_t> entityIdsToRelease;
    for (const auto& pair : m_entityIdToIndicesMap) {
        auto& subMap = pair.second;
        bool inUse = false;
        for (const auto& pair1 : subMap) {
            if (pair1.second->m_inUse) {
                inUse = true;
            }
        }

        if (inUse) {
            entityIdsToRelease.push_back(pair.first);
        }
    }

    while (!entityIdsToRelease.empty()) {
        size_t entityKey = entityIdsToRelease.back();
        releaseTessellations(entityKey);

        entityIdsToRelease.pop_back();
        m_entityIdToIndicesMap.erase(entityKey);
    }
}

MultiVboHandler::~MultiVboHandler()
{
}

void MultiVboHandler::moveKeyToLayer(int key, int layer)
{
    assert(layer > 0); // All keys are in layer zero by default. Placing a key from layer zero to layer zero is not valid.
    int curLayer = m_keysLayer[key];
    m_keysLayer[key] = layer;

    auto iter = std::find(m_layersKeys[curLayer].begin(), m_layersKeys[curLayer].end(), key);
    if (iter != m_layersKeys[curLayer].end())
        m_layersKeys[curLayer].erase(iter);

    if (layer >= m_layersKeys.size())
        m_layersKeys.resize(layer + 1);
    m_layersKeys[layer].push_back(key);
}

void MultiVboHandler::clear()
{
    for (auto batchPtr : m_batches) {
        batchPtr->m_VBO.releaseVBOs();
    }
    m_batches.clear();

    m_keysToDraw.clear();
    m_chunkSizeToBlocksMap.clear();
    m_entityIdToIndicesMap.clear();

    // Don't clear
    // m_keysLayer;
    // m_layersKeys;

}

void MultiVboHandler::beginFaceTesselation()
{
    assert(m_insideBeginEdgeTessellation == false);
    assert(m_insideBeginFaceTessellation == false);
    m_insideBeginFaceTessellation = true;
    // Nothing else required at this time
}

const IndicesPtr MultiVboHandler::setFaceTessellation(size_t entityId, size_t changeNumber, const vector<float>& points, const vector<float>& normals, const vector<float>& parameters,
    const vector<unsigned int>& vertIndices)
{
    return setFaceTessellation(entityId, 0, changeNumber, points, normals, parameters, vertIndices);
}

const IndicesPtr MultiVboHandler::setFaceTessellation(size_t entityId, size_t subPartId, size_t changeNumber, const vector<float>& points, const vector<float>& normals, const vector<float>& parameters,
    const vector<unsigned int>& vertIndices)
{
    vector<float> colors;
    return setFaceTessellation(entityId, subPartId, changeNumber, points, normals, parameters, colors, vertIndices);
}

const IndicesPtr MultiVboHandler::setFaceTessellation(size_t entityId, size_t changeNumber, const vector<float>& points, const vector<float>& normals,
    const vector<float>& parameters, const std::vector<float>& colors, const vector<unsigned int>& vertIndices)
{
    return setFaceTessellation(entityId, 0, changeNumber, points, normals, parameters, colors, vertIndices);
}

const IndicesPtr MultiVboHandler::setFaceTessellation(size_t entityId, size_t subPartId, size_t changeNumber, const vector<float>& points, const vector<float>& normals,
    const vector<float>& parameters, const std::vector<float>& colors, const vector<unsigned int>& vertIndices)
{
    assert(m_insideBeginFaceTessellation);
    assert(!points.empty());
    assert(points.size() == normals.size());

    size_t numVerts = points.size() / 3;
    size_t batchIndex, vertChunkIndex, blockSizeInChunks;
    getStorageFor(numVerts, !colors.empty(), batchIndex, vertChunkIndex, blockSizeInChunks);

    shared_ptr<Indices> pOglIndices = make_shared<Indices>();
    setFaceTessellationInner(batchIndex, vertChunkIndex, points, normals, parameters, colors, vertIndices, *pOglIndices);

    // Memory management
    pOglIndices->m_chunkIdx = vertChunkIndex;
    pOglIndices->m_numChunks = blockSizeInChunks;
    pOglIndices->m_changeNumber = changeNumber;
    m_entityIdToIndicesMap[entityId][subPartId] = pOglIndices;

    return pOglIndices;
}

void MultiVboHandler::setFaceTessellationInner(size_t batchIndex, size_t vertChunkIndex, const vector<float>& points, const vector<float>& normals, const vector<float>& parameters,
    const std::vector<float>& colors, const vector<unsigned int>& triIndices, Indices& glIndicesOut)
{
    assert(!m_batches.empty() && batchIndex < m_batches.size());

    size_t numVerts = points.size() / 3;
    unsigned int vertBaseIndex = (unsigned int) (vertChunkIndex * vertChunkSize());

    glIndicesOut.clear();
    glIndicesOut.m_batchIndex = batchIndex;
    glIndicesOut.m_vertBaseIndex = vertBaseIndex;
    glIndicesOut.m_numVertsInBatch = numVerts;

    auto& batchPtr = m_batches[batchIndex];
    batchPtr->m_needsUpdate = true;

    size_t sizeRequred2 = 2 * (vertBaseIndex + numVerts);
    size_t sizeRequred3 = 3 * (vertBaseIndex + numVerts);

    // In the primary code path these arrays are always sized correctly, but during undo/redo there can be a mismatch. Resize everything if they're too small.
    if (batchPtr->m_points.size() < sizeRequred3)
        batchPtr->m_points.resize(sizeRequred3);
    if (batchPtr->m_normals.size() < sizeRequred3)
        batchPtr->m_normals.resize(sizeRequred3);

    if (batchPtr->m_parameters.size() < sizeRequred2)
        batchPtr->m_parameters.resize(sizeRequred2);

    if (!colors.empty() && batchPtr->m_colors.size() < sizeRequred3)
        batchPtr->m_colors.resize(sizeRequred3);

    for (size_t vertIdx = 0; vertIdx < numVerts; vertIdx++) {
        for (int j = 0; j < 3; j++) {
            size_t dstIdx = 3 * (vertBaseIndex + vertIdx) + j;
            size_t srcIdx = 3 * vertIdx + j;
            batchPtr->m_points[dstIdx] = points[srcIdx];
            batchPtr->m_normals[dstIdx] = normals[srcIdx];
            if (!colors.empty())
                batchPtr->m_colors[dstIdx] = colors[srcIdx];
        }

        for (int j = 0; j < 2; j++) {
            size_t dstIdx = 2 * (vertBaseIndex + vertIdx) + j;
            size_t srcIdx = 2 * vertIdx + j;
            batchPtr->m_parameters[dstIdx] = parameters[srcIdx];
        }
    }

    for (size_t i = 0; i < triIndices.size(); i++) {
        unsigned int vertIndex = vertBaseIndex + triIndices[i];
        glIndicesOut.m_elementIndices.push_back(vertIndex);
    }
}

const IndicesPtr MultiVboHandler::setFaceTessellation(size_t entityId, const IndicesPtr& pSrc, const vector<unsigned int>& elementIndices)
{
    shared_ptr<Indices> pOglIndices = make_shared<Indices>();

    pOglIndices->m_elementIndices = elementIndices;

    pOglIndices->m_batchIndex =  pSrc->m_batchIndex;
    pOglIndices->m_vertBaseIndex = pSrc->m_vertBaseIndex;
    pOglIndices->m_numVertsInBatch = pSrc->m_numVertsInBatch;
    pOglIndices->m_inUse = true;
    pOglIndices->m_changeNumber = pSrc->m_changeNumber;
    pOglIndices->m_chunkIdx = pSrc->m_chunkIdx;
    pOglIndices->m_numChunks = pSrc->m_numChunks;

    m_entityIdToIndicesMap[entityId][0] = pOglIndices;

    return pOglIndices;
}

void MultiVboHandler::endFaceTesselation(bool smoothNormals)
{
    assert(m_insideBeginFaceTessellation);
    m_insideBeginFaceTessellation = false;
    for (size_t idx = 0; idx < m_batches.size(); idx++) {
        auto pBatch = m_batches[idx];
        if (pBatch && pBatch->m_needsUpdate) {
            pBatch->m_VBO.copyToVBO(pBatch->m_points, pBatch->m_normals, smoothNormals, pBatch->m_parameters, pBatch->m_colors);
            pBatch->m_needsUpdate = false;

            pBatch->m_points = {};
            pBatch->m_normals = {};
            pBatch->m_parameters = {};
            pBatch->m_colors = {};
        }
    }
}

bool MultiVboHandler::getVert(const Index& glIndicesOut, float coords[3]) const
{
    if (m_batches.empty() || glIndicesOut.m_batchIndex >= m_batches.size())
        return false;
    shared_ptr<VertexBatch> pBatch = m_batches[glIndicesOut.m_batchIndex];
    if (pBatch->m_points.empty())
        return false;
    size_t vertIdx = 3 * glIndicesOut.m_vertIndex;
    if (vertIdx + 2 < pBatch->m_points.size()) {
        coords[0] = pBatch->m_points[vertIdx + 0];
        coords[1] = pBatch->m_points[vertIdx + 1];
        coords[2] = pBatch->m_points[vertIdx + 2];
        return true;
    }

    return false;
}

bool MultiVboHandler::getNormal(const Index& glIndicesOut, float coords[3]) const
{
    if (m_batches.empty() || glIndicesOut.m_batchIndex >= m_batches.size())
        return false;
    shared_ptr<VertexBatch> pBatch = m_batches[glIndicesOut.m_batchIndex];
    size_t vertIdx = 3 * glIndicesOut.m_vertIndex;
    if (vertIdx + 2 < pBatch->m_normals.size()) {
        coords[0] = pBatch->m_normals[vertIdx + 0];
        coords[1] = pBatch->m_normals[vertIdx + 1];
        coords[2] = pBatch->m_normals[vertIdx + 2];
        return true;
    }

    return false;
}

bool MultiVBO::getVBOArray(GLuint vboId, vector<float>& values) const
{
    bool result = false;
    if (vboId != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, vboId);
        GLint size;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
        if (size > 0) {
            values.resize(size / sizeof(float));
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, (void*)values.data());
            result = true;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    return result;
}

bool MultiVBO::getVBOArray(GLuint vboId, vector<unsigned int>& values) const
{
    bool result = false;
    if (vboId != 0) {

        glBindBuffer(GL_ARRAY_BUFFER, vboId);
        GLint size;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
        if (size > 0) {
            values.resize(size / sizeof(unsigned int));
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, (void*)values.data());
            result = true;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    return result;
}

bool MultiVboHandler::getRawData(size_t entityId, vector<float>& vertices, vector<float>& normals, vector<float>& parameters) const
{
    vertices.clear();
    normals.clear();
    parameters.clear();

    auto p = m_entityIdToIndicesMap.find(entityId);
    if (p == m_entityIdToIndicesMap.end())
        return false;

    const auto& iter = p->second.begin();
    const Indices& triIndices = *iter->second;

    if ((triIndices.m_batchIndex == _SIZE_T_ERROR) || (triIndices.m_batchIndex >= m_batches.size()))
        return false;

    const shared_ptr<VertexBatch> pBatch = m_batches[triIndices.m_batchIndex];

    bool result = true;
    // Merge all the index ids in all maps to create the list for the entire
    result = result && pBatch->m_VBO.getVBOArray(pBatch->m_VBO.vertexVboID(), vertices);
    result = result && pBatch->m_VBO.getVBOArray(pBatch->m_VBO.normalVboID(), normals);
    result = result && pBatch->m_VBO.getVBOArray(pBatch->m_VBO.textureVboID(), parameters);

    return result;
}

bool MultiVboHandler::setColorVBO(size_t entityId, vector<float>& srcColors)
{
    auto p = m_entityIdToIndicesMap.find(entityId);
    if (p == m_entityIdToIndicesMap.end())
        return false;

    const auto& iter = p->second.begin();
    const Indices& triIndices = *iter->second;

    if (m_batches.empty() || triIndices.m_batchIndex >= m_batches.size() || srcColors.empty())
        return false;
    shared_ptr<VertexBatch> batchPtr = m_batches[triIndices.m_batchIndex];

    auto& currentColors = batchPtr->m_colors;

    // Overwrite those that belong to this face with the new colors
    size_t lastIdx = triIndices.m_vertBaseIndex + triIndices.m_numVertsInBatch;
    if (lastIdx >= currentColors.size()) {
        currentColors.resize(batchPtr->m_points.size(), 0);
    }

    // Write the new colors into the cache array
    for (size_t srcVertIdx = 0; srcVertIdx < triIndices.m_numVertsInBatch; srcVertIdx++) {
        size_t dstVertIdx = triIndices.m_vertBaseIndex + srcVertIdx;

        currentColors[dstVertIdx] = srcColors[srcVertIdx];
    }

    // Copy the color cache to the VBO
    return batchPtr->m_VBO.copyColorsToExistingVBO(currentColors);
}

bool MultiVboHandler::setBackColorVBO(size_t entityId, vector<float>& srcColors)
{
    auto p = m_entityIdToIndicesMap.find(entityId);
    if (p == m_entityIdToIndicesMap.end())
        return false;

    const auto& iter = p->second.begin();
    const Indices& triIndices = *iter->second;

    if (m_batches.empty() || triIndices.m_batchIndex >= m_batches.size())
        return false;
    shared_ptr<VertexBatch> batchPtr = m_batches[triIndices.m_batchIndex];

    auto& currentColors = batchPtr->m_backColors;

    // Overwrite those that belong to this face with the new colors
    size_t lastIdx = triIndices.m_vertBaseIndex + triIndices.m_numVertsInBatch;
    if (lastIdx >= currentColors.size()) {
        currentColors.resize(batchPtr->m_points.size() / 3, 0);
    }

    // Write the new colors into the cache array
    for (size_t srcVertIdx = 0; srcVertIdx < triIndices.m_numVertsInBatch; srcVertIdx++) {
        size_t dstVertIdx = triIndices.m_vertBaseIndex + srcVertIdx;

        currentColors[dstVertIdx] = srcColors[srcVertIdx];
    }

    // Copy the color cache to the VBO
    return batchPtr->m_VBO.copyBackColorsToExistingVBO(currentColors);
}

void MultiVboHandler::getStorageFor(size_t numVertsNeeded, bool needColorStorage, size_t& batchIndex, size_t& vertChunkIndex, size_t& blockSizeInChunks)
{
    bool allocateSpaceForTriangles = isTriangleVBO();
    batchIndex = _SIZE_T_ERROR;
    blockSizeInChunks = calcNumVertChunks(numVertsNeeded);

    ChunkSizeBlockMap::iterator bestFitBlockIter = m_chunkSizeToBlocksMap.end(), iter;
    size_t minSize = _SIZE_T_ERROR;
    for (iter = m_chunkSizeToBlocksMap.begin(); iter != m_chunkSizeToBlocksMap.end(); iter++) {
        if (iter->first >= blockSizeInChunks && iter->first < minSize) {
            bestFitBlockIter = iter;
            minSize = iter->first;
        }
    }

    if (bestFitBlockIter != m_chunkSizeToBlocksMap.end()) {
        auto& availBlocks = bestFitBlockIter->second;
        assert(!availBlocks.empty());
        size_t curBlockSizeInChunks = bestFitBlockIter->first;
        vertChunkIndex = availBlocks.back().m_chunkIndex;
        if (blockSizeInChunks < curBlockSizeInChunks) {
            size_t newChunkIndex = vertChunkIndex + blockSizeInChunks;
            size_t newBlockSizeInChunks = curBlockSizeInChunks - blockSizeInChunks;
            batchIndex = availBlocks.back().m_batchIndex;

            availBlocks.pop_back();
            if (availBlocks.empty())
                m_chunkSizeToBlocksMap.erase(bestFitBlockIter);
            if (newBlockSizeInChunks > 0) {
                auto targetIter = m_chunkSizeToBlocksMap.find(newBlockSizeInChunks);
                if (targetIter == m_chunkSizeToBlocksMap.end())
                    targetIter = m_chunkSizeToBlocksMap.insert(make_pair(newBlockSizeInChunks, vector<FreeChunkRecord>())).first;
                targetIter->second.push_back(FreeChunkRecord(batchIndex, newChunkIndex));
            }
            assert(batchIndex != _SIZE_T_ERROR && batchIndex < m_batches.size());
        } else {
            assert(blockSizeInChunks == curBlockSizeInChunks);

            batchIndex = availBlocks.back().m_batchIndex;
            vertChunkIndex = availBlocks.back().m_chunkIndex;

            availBlocks.pop_back();
            if (availBlocks.empty())
                m_chunkSizeToBlocksMap.erase(bestFitBlockIter);
            assert(batchIndex != _SIZE_T_ERROR && batchIndex < m_batches.size());
        }
        assert(batchIndex != _SIZE_T_ERROR && batchIndex < m_batches.size());
    } else {
        shared_ptr<VertexBatch> batchPtr;
        size_t vertBaseIndex = 0;
        if (allocateSpaceForTriangles) {
            static int MAX_FACE_VERTS_PER_BATCH = 0;
            if (MAX_FACE_VERTS_PER_BATCH == 0) {
                int val = OGL_FACE_BATCH_VERT_COUNT_MAX_K;
                if (val < 2)
                    val = 2;
                MAX_FACE_VERTS_PER_BATCH = val * 1024;
            }

            for (size_t i = 0; i < m_batches.size(); i++) {
                batchPtr = m_batches[i];
                size_t numVertsInBatch = batchPtr->m_points.size() / 3;
                if (numVertsInBatch + numVertsNeeded < MAX_FACE_VERTS_PER_BATCH) {
                    batchIndex = i;
                    vertBaseIndex = numVertsInBatch;
                    break;
                }
            }
        } else {
            static int MAX_EDGE_VERTS_PER_BATCH = 0;

            if (MAX_EDGE_VERTS_PER_BATCH == 0) {
                int val = OGL_EDGE_BATCH_VERT_COUNT_MAX_K;
                if (val < 2)
                    val = 2;
                MAX_EDGE_VERTS_PER_BATCH = val * 1024;
            }

            for (size_t i = 0; i < m_batches.size(); i++) {
                batchPtr = m_batches[i];
                size_t numVertsInBatch = batchPtr->m_points.size() / 3;
                if (numVertsInBatch + numVertsNeeded < MAX_EDGE_VERTS_PER_BATCH) {
                    batchIndex = i;
                    vertBaseIndex = numVertsInBatch;
                    break;
                }
            }
        }

        if (!batchPtr || batchIndex == _SIZE_T_ERROR) {
            batchIndex = m_batches.size();
            batchPtr = make_shared<VertexBatch>(m_primitiveType);
            m_batches.push_back(batchPtr);
        }

        vertChunkIndex = calcVertChunkIndex(vertBaseIndex);

        size_t sizeNeeded3 = 3 * (vertBaseIndex + blockSizeInChunks * vertChunkSize());
        size_t sizeNeeded2 = 2 * (vertBaseIndex + blockSizeInChunks * vertChunkSize());

        if (sizeNeeded3 >= batchPtr->m_points.size())
            batchPtr->m_points.resize(sizeNeeded3);

        if (allocateSpaceForTriangles) {
            batchPtr->m_normals.resize(sizeNeeded3);
            batchPtr->m_parameters.resize(sizeNeeded2);
            if (needColorStorage) {
                batchPtr->m_colors.resize(sizeNeeded3);
                batchPtr->m_backColors.resize(sizeNeeded3);
            }
        }

        assert(batchIndex != _SIZE_T_ERROR && batchIndex < m_batches.size());
    }
    assert(batchIndex != _SIZE_T_ERROR && batchIndex < m_batches.size());
    shared_ptr<VertexBatch> batchPtr = m_batches[batchIndex];
    if (vertChunkIndex >= batchPtr->m_allocatedChunks.size())
        batchPtr->m_allocatedChunks.resize(vertChunkIndex + 1);
    batchPtr->m_allocatedChunks[vertChunkIndex] = blockSizeInChunks;
}

void MultiVboHandler::releaseTessellations(size_t entityId)
{
    const auto iter = m_entityIdToIndicesMap.find(entityId);
    if (iter == m_entityIdToIndicesMap.end())
        return;

    const auto& iter1 = iter->second.begin();
    const auto& pGlIndices = iter1->second;

    if ((pGlIndices->m_numVertsInBatch == 0) || (pGlIndices->m_vertBaseIndex == _SIZE_T_ERROR) || (pGlIndices->m_batchIndex >= m_batches.size()))
        return;

    auto batchPtr = m_batches[pGlIndices->m_batchIndex];
    size_t vertChunkIndex = pGlIndices->m_chunkIdx;
    if (vertChunkIndex >= batchPtr->m_allocatedChunks.size() || batchPtr->m_allocatedChunks[vertChunkIndex] <= 0)
        return;

    // Mark the chunk as not allocated
    batchPtr->m_allocatedChunks[vertChunkIndex] = 0;

    m_clearAllLayers = true;

    size_t numChunks = pGlIndices->m_numChunks;
    assert(vertChunkIndex * vertChunkSize() == pGlIndices->m_vertBaseIndex); // This checks there is no integer round off error.

    auto nextIter = m_chunkSizeToBlocksMap.find(numChunks);
    if (nextIter == m_chunkSizeToBlocksMap.end())
        nextIter = m_chunkSizeToBlocksMap.insert(make_pair(numChunks, vector<FreeChunkRecord>())).first;

    auto& availBlocks = nextIter->second;
    availBlocks.push_back(FreeChunkRecord(pGlIndices->m_batchIndex, vertChunkIndex));
}

void MultiVboHandler::beginEdgeTesselation()
{
    assert(m_insideBeginEdgeTessellation == false);
    assert(m_insideBeginFaceTessellation == false);
    m_insideBeginEdgeTessellation = true;
    // Nothing else required at this time
}

const IndicesPtr MultiVboHandler::setEdgeStripTessellation(size_t entityId, const std::vector<float>& lineStripPoints)
{
    return (setEdgeStripTessellation(entityId, 0, lineStripPoints));
}

const IndicesPtr MultiVboHandler::setEdgeStripTessellation(size_t entityId, size_t subPartId, const vector<float>& lineStripPoints)
{
    assert(m_insideBeginEdgeTessellation);
    size_t numVerts = lineStripPoints.size() / 3;

    size_t batchIndex, vertChunkIndex, blockSizeInChunks;
    getStorageFor(numVerts, false, batchIndex, vertChunkIndex, blockSizeInChunks);

    shared_ptr<Indices> pOglIndices = make_shared<Indices>();
    setEdgeStripTessellationInner(batchIndex, vertChunkIndex, lineStripPoints, *pOglIndices);
    pOglIndices->m_chunkIdx = vertChunkIndex;
    pOglIndices->m_numChunks = blockSizeInChunks;
    m_entityIdToIndicesMap[entityId][subPartId] = pOglIndices;

    return pOglIndices;
}

const IndicesPtr MultiVboHandler::setEdgeSegTessellation(size_t entityId, size_t changeNumber, const std::vector<float>& points, const std::vector<unsigned int>& indices)
{
    vector<float> colors;
    return setEdgeSegTessellation(entityId, 0, changeNumber, points, colors, indices);
}

const IndicesPtr MultiVboHandler::setEdgeSegTessellation(size_t entityId, size_t subPartId, size_t changeNumber, const std::vector<float>& points, const std::vector<unsigned int>& indices)
{
    vector<float> colors;
    return setEdgeSegTessellation(entityId, subPartId, changeNumber, points, colors, indices);
}

const IndicesPtr MultiVboHandler::setEdgeSegTessellation(size_t entityId, size_t changeNumber, const std::vector<float>& points,
    const std::vector<float>& colors, const std::vector<unsigned int>& indices)
{
    return setEdgeSegTessellation(entityId, 0, changeNumber, points, colors, indices);
}

const IndicesPtr MultiVboHandler::setEdgeSegTessellation(size_t entityId, size_t subPartId, size_t changeNumber, const std::vector<float>& points,
    const std::vector<float>& colors, const std::vector<unsigned int>& indices)
{
    assert(m_insideBeginEdgeTessellation);
    size_t numVerts = points.size() / 3;

    size_t batchIndex, vertChunkIndex, blockSizeInChunks;
    getStorageFor(numVerts, false, batchIndex, vertChunkIndex, blockSizeInChunks);
    shared_ptr<Indices> pOglIndices = make_shared<Indices>();
    setEdgeSegTessellationInner(batchIndex, vertChunkIndex, points, colors, indices, *pOglIndices);
    pOglIndices->m_chunkIdx = vertChunkIndex;
    pOglIndices->m_numChunks = blockSizeInChunks;
    pOglIndices->m_changeNumber = changeNumber;
    m_entityIdToIndicesMap[entityId][subPartId] = pOglIndices;

    return pOglIndices;
}

void MultiVboHandler::setEdgeStripTessellationInner(size_t batchIndex, size_t vertChunkIndex, const vector<float>& lineStripPts, Indices& glIndicesOut)
{
    const size_t numVerts = lineStripPts.size() / 3;
    unsigned int vertBaseIndex = (unsigned int) (vertChunkIndex * vertChunkSize());

    glIndicesOut.m_batchIndex = batchIndex;
    glIndicesOut.m_vertBaseIndex = vertBaseIndex;
    glIndicesOut.m_numVertsInBatch = numVerts;

    glIndicesOut.m_elementIndices.clear();
    auto batchPtr = m_batches[batchIndex];

    vector<unsigned int> indices;

    for (size_t vertIdx = 0; vertIdx < numVerts; vertIdx++) {
        for (int j = 0; j < 3; j++) {
            batchPtr->m_points[3 * (vertBaseIndex + vertIdx) + j] = lineStripPts[3 * vertIdx + j];
        }

        // Now that we've pushed the verts, next index is the current index
        indices.push_back((unsigned int)(vertBaseIndex + vertIdx));
    }

    for (size_t i = 0; i < indices.size() - 1; i++) {
        glIndicesOut.m_elementIndices.push_back(indices[i]);
        glIndicesOut.m_elementIndices.push_back(indices[i + 1]);
    }
}

void MultiVboHandler::setEdgeSegTessellationInner(size_t batchIndex, size_t vertChunkIndex, const std::vector<float>& pts, const std::vector<float>& colors, const std::vector<unsigned int>& indicesIn, Indices& glIndicesOut)
{
    const size_t numVerts = pts.size() / 3;
    unsigned int vertBaseIndex = (unsigned int) (vertChunkIndex * vertChunkSize());

    glIndicesOut.m_batchIndex = batchIndex;
    glIndicesOut.m_vertBaseIndex = vertBaseIndex;
    glIndicesOut.m_numVertsInBatch = numVerts;

    glIndicesOut.m_elementIndices.clear();
    auto batchPtr = m_batches[batchIndex];

    // In the primary code path this array is always sized correctly, but during undo/redo there can be a mismatch. Resize it if it's too small.
    size_t spaceRequired = 3 * (vertBaseIndex + numVerts);
    if (batchPtr->m_points.size() < spaceRequired)
        batchPtr->m_points.resize(spaceRequired);

    for (size_t vertIdx = 0; vertIdx < numVerts; vertIdx++) {
        for (int j = 0; j < 3; j++) {
            batchPtr->m_points[3 * (vertBaseIndex + vertIdx) + j] = pts[3 * vertIdx + j];
        }
    }

    bool hasColors = !batchPtr->m_colors.empty();
    bool needsColors = !colors.empty();
    if (needsColors || hasColors) {
        if (batchPtr->m_colors.size() < spaceRequired)
            batchPtr->m_colors.resize(spaceRequired);

        if (!colors.empty()) {
            for (size_t vertIdx = 0; vertIdx < numVerts; vertIdx++) {
                for (int j = 0; j < 3; j++) {
                    batchPtr->m_colors[3 * (vertBaseIndex + vertIdx) + j] = colors[3 * vertIdx + j];
                }
            }
        }
    }

    for (size_t i = 0; i < indicesIn.size(); i++) {
        glIndicesOut.m_elementIndices.push_back(vertBaseIndex + indicesIn[i]);
    }
}

const IndicesPtr MultiVboHandler::setEdgeSegTessellation(size_t entityId, const IndicesPtr& pSrc, const std::vector<unsigned int>& elementIndices)
{
    shared_ptr<Indices> pOglIndices = make_shared<Indices>();

    pOglIndices->m_elementIndices = elementIndices;

    pOglIndices->m_batchIndex = pSrc->m_batchIndex;
    pOglIndices->m_vertBaseIndex = pSrc->m_vertBaseIndex;
    pOglIndices->m_numVertsInBatch = pSrc->m_numVertsInBatch;
    pOglIndices->m_inUse = true;
    pOglIndices->m_changeNumber = pSrc->m_changeNumber;
    pOglIndices->m_chunkIdx = pSrc->m_chunkIdx;
    pOglIndices->m_numChunks = pSrc->m_numChunks;

    m_entityIdToIndicesMap[entityId][0] = pOglIndices;

    return pOglIndices;
}

void MultiVboHandler::endEdgeTesselation()
{
    assert(m_insideBeginEdgeTessellation);
    m_insideBeginEdgeTessellation = false;
    for (size_t idx = 0; idx < m_batches.size(); idx++) {
        auto pBatch = m_batches[idx];
        if (pBatch && pBatch->m_needsUpdate) {
            pBatch->m_needsUpdate = false;
            if (!pBatch->m_points.empty()) {
                pBatch->m_VBO.copyToVBO(pBatch->m_points, pBatch->m_colors);

                pBatch->m_points = {};
                pBatch->m_colors = {};
            }
        }
    }
}

bool MultiVboHandler::getRawData(size_t entityId, std::vector<unsigned int>& indices) const
{
    indices.clear();

    const auto p = m_entityIdToIndicesMap.find(entityId);
    if (p == m_entityIdToIndicesMap.end())
        return false;

    const auto& iter = p->second.begin();
    const auto& glIndices = *iter->second;
    if (glIndices.m_batchIndex >= m_batches.size())
        return false;

    indices.reserve(glIndices.m_elementIndices.size());
    for (size_t i = 0; i < glIndices.m_elementIndices.size(); ++i) {
        indices.push_back(glIndices.m_elementIndices[i] - glIndices.m_vertBaseIndex);
    }
    return true;
}

void MultiVboHandler::beginSettingElementIndices(size_t layerBitMask)
{
    if (m_clearAllLayers) {
        m_layerBitMask = _SIZE_T_ERROR;
        m_clearAllLayers = false;
    } else
        m_layerBitMask = layerBitMask;

    for (auto& pBatch : m_batches) {
        int layer = 0;
        size_t bitMask = m_layerBitMask;
        while (bitMask > 0 && layer < m_layersKeys.size()) {
            if ((bitMask & 1) == 1) {
                vector<int>& keys = m_layersKeys[layer];
                for (int key : keys) {
                    auto iter = pBatch->m_indexMap.find(key);
                    if (iter != pBatch->m_indexMap.end()) {
                        iter->second.clear();
                    }

                    pBatch->m_VBO.releaseKeysElementVBO(key);
                }
            }
            layer += 1;
            bitMask = bitMask >> 1;
        }
    }
}

void MultiVboHandler::includeElementIndices(int key, const IndicesPtr& batchIndices, GLuint texId)
{
    if (m_batches.empty() || !batchIndices || batchIndices->m_batchIndex >= m_batches.size())
        return;

    m_keysToDraw.insert(key);

    size_t layer = findLayerForKey(key);
    size_t layerBit = 1LL << layer;
    if ((layerBit & m_layerBitMask) == 0) {
        return;
    }

    auto& pBatch = m_batches[batchIndices->m_batchIndex];
    if (texId) {
        shared_ptr<VertexBatch::ElemIndexMapRec> pRec = make_shared<VertexBatch::ElemIndexMapRec>(texId, batchIndices->m_elementIndices);
        pBatch->m_texturedFaces.push_back(pRec);
    } else {
        auto iter = pBatch->m_indexMap.find(key);
        if (iter == pBatch->m_indexMap.end()) {
            iter = pBatch->m_indexMap.insert(make_pair(key, vector<unsigned int>())).first;
        }
        vector<unsigned int>& indices = iter->second;
        const auto& triIndices = batchIndices->m_elementIndices;
        indices.insert(indices.end(), triIndices.begin(), triIndices.end());
    }
}

void MultiVboHandler::endSettingElementIndices()
{
    for (auto& pBatch : m_batches) {
        for (const auto& pair : pBatch->m_indexMap) {
            pBatch->m_VBO.setIndexVBO(pair.first, pair.second);
        }
    }
}

void MultiVboHandler::draw(int key, MultiVBO::DrawVertexColorMode drawColors) const
{
    for (size_t idx = 0; idx < m_batches.size(); idx++) {
        const auto& pBatch = m_batches[idx];
        pBatch->m_VBO.drawVBO(m_pShader, key, drawColors); // Draw using list 0, normal edge vert ids
    }
}

void MultiVboHandler::bindCommonBuffers(const shared_ptr<VertexBatch>& batchPtr) const
{
    auto& vbo = batchPtr->m_VBO;
    batchPtr->m_VBO.bindCommon(m_pShader, vbo.m_numVerts);
}

void MultiVboHandler::drawKeyForBatch(int key, const shared_ptr<VertexBatch>& batchPtr, MultiVBO::DrawVertexColorMode drawColors) const
{
    batchPtr->m_VBO.drawVBO(m_pShader, key, drawColors);
}

void MultiVboHandler::drawTexturedFaces(const std::shared_ptr<VertexBatch>& batchPtr) const
{
}

void MultiVboHandler::unbindCommonBuffers(const shared_ptr<VertexBatch>& batchPtr) const
{
    auto& vbo = batchPtr->m_VBO;
    vbo.unbindCommon();
}

inline size_t MultiVboHandler::vertChunkSize() const
{
    if (isTriangleVBO())
        return 20; // This is the number of vertices in a chunk, not bytes in a chunk. Bytes of floats in points = vertChunkSize * 3 * 4 = 252 ~= 240 bytes.
    else
        return 10;
}

inline size_t MultiVboHandler::calcVertChunkIndex(size_t vertBaseIndex) const
{
    return vertBaseIndex / vertChunkSize();
}

inline size_t MultiVboHandler::calcNumVertChunks(size_t numVerts) const
{
    size_t numChunks = size_t((numVerts + (vertChunkSize() - 1)) / vertChunkSize());
    assert(numVerts <= (numChunks * vertChunkSize()));
    return numChunks;
}

inline bool MultiVboHandler::isTriangleVBO() const
{
    return m_primitiveType == GL_TRIANGLES;
}

// TODO remove the dump methods once things are working, they won't be needed.
namespace
{
    template<class T>
    void dumpVect(const std::string& pad, ostream& out, const vector<T>& vals, size_t size)
    {
        size_t n = vals.size() / size;
        out << pad << "  num: " << n << "\n";
#if 0
        out << pad << "  [";
        for (size_t i = 0; i < n; i++) {
            out << "(";
            for (size_t j = 0; j < 3; j++)
                out << vals[j] << ", ";
            out << "),";
        }
        out << "]\n";
#endif
    }
}

Indices::Indices(size_t batchIndex, const vector<unsigned int>& elementIndices)
    : m_batchIndex(batchIndex)
    , m_elementIndices(elementIndices)
{
}

void Indices::clear()
{
    m_elementIndices.clear();

    m_batchIndex = _SIZE_T_ERROR;   // Pointer to the batch which stores this face's data
    m_vertBaseIndex = -1;    // Index of the entity's first index in the batch
    m_numVertsInBatch = 0;          // Number of entity vertices in the batch
}

size_t Indices::numBytes() const
{
    size_t result = sizeof(Indices);

    result += m_elementIndices.capacity() * sizeof(unsigned int);

    return result;
}

size_t MultiVboHandler::numBytes() const
{
    size_t result = sizeof(MultiVboHandler);

    result += sizeof(m_chunkSizeToBlocksMap);
    result += m_chunkSizeToBlocksMap.size() * sizeof(pair<size_t, std::vector<FreeChunkRecord>>);
    for (const auto& pair : m_chunkSizeToBlocksMap) {
        const auto& vec = pair.second;
        result += vec.capacity() * sizeof(FreeChunkRecord);
    }
    result += m_keysToDraw.size() * sizeof(int);
    result += m_keysLayer.size() * sizeof(int);

    result += m_layersKeys.size() * sizeof(vector<int>);
    for (const auto& vec : m_layersKeys)
        result += vec.capacity() * sizeof(int);

    result += m_batches.size() * sizeof(shared_ptr<VertexBatch>);
    for (const auto& pBatch : m_batches) {
        result += pBatch->numBytes();
    }

    for (const auto& pair0 : m_entityIdToIndicesMap) {
        const auto& map0 = pair0.second;
        for (const auto& pair1 : map0) {
            const auto& pIndices = pair1.second;
            result += pIndices->numBytes();
        }
    }

    return result;
}

MultiVboHandler::VertexBatch::VertexBatch(int primitiveType)
    : m_VBO(primitiveType)
{
}

size_t MultiVboHandler::VertexBatch::numBytes() const
{
    size_t result = sizeof(VertexBatch);

    result += m_points.capacity() * sizeof(float);
    result += m_normals.capacity() * sizeof(float);
    result += m_parameters.capacity() * sizeof(float);
    result += m_colors.capacity() * sizeof(float);
    result += m_backColors.capacity() * sizeof(float);
    result += m_allocatedChunks.capacity() * sizeof(size_t);

    for (const auto& pair : m_indexMap) {
        result += sizeof(pair);
        const auto& vec = pair.second;
        result += vec.capacity() * sizeof(unsigned int);
    }

    for (const auto& pVec : m_texturedFaces) {
        const auto& vec = *pVec;
        result += sizeof(vec);
        result += vec.m_elementIndices.capacity() * sizeof(unsigned int);
    }

    result += m_VBO.numBytes();

    return result;
}
