/*! \file buffer-cache.hpp
 *
 * The BufferCache provides a cache for VAO objects.
 *
 * \author John Reppy
 */

/* CMSC23700 Final Project sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _BUFFER_CACHE_HPP_
#define _BUFFER_CACHE_HPP_

#include "cs237.hpp"

//! A wrapper for a vertex+index buffer.
struct VAO {
    cs237::VertexBuffer *_vBuf; //!< the vertex buffer
    cs237::IndexBuffer *_iBuf;  //!< the index buffer
    cs237::MemoryObj *_vMem;    //!< the vertex-buffer memory object
    cs237::MemoryObj *_iMem;    //!< the vertex-buffer memory object
    bool _inUse;                //!< true when this VAO is assigned to a chunk

    VAO (cs237::Application *_app);
    ~VAO ();

    uint32_t nIndices () const { return (this->_inUse ? this->_iBuf->nIndices() : 0); }

  //! load the contents of a VAO with the data for a chunk
  //! \param[in] chunk the source of data to load in the VAO
    void load (struct Chunk const &chunk);

  //! emit commands to render the contents of the VAO.
    void render (VkCommandBuffer cmdBuf)
    {
        assert (this->_inUse);

/* TODO: set drawing mode */
        VkBuffer vertBuffers[] = { this->_vBuf->vkBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertBuffers, offsets);

        vkCmdBindIndexBuffer(cmdBuf, this->_iBuf->vkBuffer(), 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(cmdBuf, this->nIndices(), 1, 0, 0, 0);

#ifdef XXX
        CS237_CHECK( glBindVertexArray (this->_id) );
        CS237_CHECK( glDrawElements (GL_TRIANGLE_STRIP, this->_nIndices, GL_UNSIGNED_SHORT, 0) );
        CS237_CHECK( glBindVertexArray (0) );
#endif
    }

};

//! A cache of VAO objects
class BufferCache {
  public:

  //! constructor
    BufferCache ();
  //! destructor
    ~BufferCache ();

  //! acquire a VAO object from the cache for use.
    VAO *acquire ();
  //! release an unused VAO back to the cache
    void release (VAO *vao);

  private:
    std::vector<VAO *>  _freeList;      //!< the list of free VAO objects
};

#endif // !_BUFFER_CACHE_HPP_
