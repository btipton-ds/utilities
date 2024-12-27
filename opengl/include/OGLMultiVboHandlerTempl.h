#pragma once

// Open this file for documentation. It's stored in the include directory with this header file.
// 
// OGLMultiVboHandler.html
//
// If someone can find a way to hyperlink to a relative file, please do that.

#include <vector>
#include <memory>
#include <OGLMultiVboHandler.h>

template<typename PRE_FUNC, typename POST_FUNC, typename PRE_TEX_FUNC, typename POST_TEX_FUNC>
inline void OGL::COglMultiVboHandler::drawAllKeys(PRE_FUNC preDrawFunc, POST_FUNC postDrawFunc, PRE_TEX_FUNC preDrawTexFunc, POST_TEX_FUNC postDrawTexFunc) const
{
    for (auto batchPtr : m_batches) {
        bindCommonBuffers(batchPtr);

        for (size_t layerNum = 0; layerNum < m_layersKeys.size(); layerNum++) { // Each layer has a list of keys to be drawn in the layer. The keys are stored in a set
            const std::vector<int>& layerKeys = m_layersKeys[layerNum];
            for (int key : layerKeys) {
                if (m_keysToDraw[key] && (batchPtr->m_indexMap.count(key) != 0)) {
                    COglMultiVBO::DrawVertexColorMode colorMode = preDrawFunc(key); // Sets up the required gl calls for drawing this key
                    drawKeyForBatch(key, batchPtr, colorMode);
                    postDrawFunc(); // Clean up gl properties
                }
            }

            if (layerNum == 0) {
                if (!batchPtr->m_texturedFaces.empty()) {
                    for (const auto& rec : batchPtr->m_texturedFaces) {

                        preDrawTexFunc(rec->m_texId);
                        batchPtr->m_VBO.drawVBO(m_pShader, rec->m_elementIndices);
                        postDrawTexFunc();
                    }
                    drawTexturedFaces(batchPtr);
                }
            }
        }

        unbindCommonBuffers(batchPtr);
    }
}


