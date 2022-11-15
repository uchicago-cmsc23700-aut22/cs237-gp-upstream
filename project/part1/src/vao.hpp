/*! \file vao.hpp
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _VAO_HPP_
#define _VAO_HPP_

#include "cs237.hpp"
#include "map-cell.hpp"

//! A vertex-array object is a container for the information
//! required to render a chunk of the mesh.
struct VAO {
    cs237::VertexBuffer *_vBuf; //!< the vertex buffer
    cs237::IndexBuffer *_iBuf;  //!< the index buffer

    VAO (cs237::Application *_app, struct Chunk const &chunk);
    ~VAO ();

    uint32_t nIndices () const { return this->_iBuf->nIndices(); }

    //! emit commands to render the contents of the VAO.
    void render (VkCommandBuffer cmdBuf)
    {
        VkBuffer vertBuffers[] = {this->_vBuf->vkBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertBuffers, offsets);

        vkCmdBindIndexBuffer(
            cmdBuf, this->_iBuf->vkBuffer(), 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(cmdBuf, this->_iBuf->nIndices(), 1, 0, 0, 0);
    }

};

#endif //! _VAO_HPP_
