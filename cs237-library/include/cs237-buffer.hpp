/*! \file cs237-buffer.hpp
 *
 * Buffer objects with backing device memory.
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_BUFFER_HPP_
#define _CS237_BUFFER_HPP_

#ifndef _CS237_HPP_
#error "cs237-buffer.hpp should not be included directly"
#endif

namespace cs237 {

//! A base class for buffer objects of all kinds backed by device memory
class Buffer {
public:

    //! return the Vulkan handle for the buffer
    VkBuffer vkBuffer () const { return this->_buf; }

    //! return the size of the buffer in bytes
    size_t size() const { return this->_sz; }

protected:
    Application *_app;          //!< the application
    VkBuffer _buf;              //!< the Vulkan buffer object
    VkDeviceMemory _mem;        //!< device memory for the buffer
    size_t _sz;                 //!< size of the buffer

    Buffer (Application *app, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, size_t sz);
    ~Buffer ();

    //! \brief copy data to the buffer using a staging buffer.
    //! \param src      address of data to copy
    //! \param offset   offset from the beginning of the destination buffer to copy
    //!                 the data to
    //! \param sz       size in bytes of the data to copy
    void _stageDataToBuffer (const void *src, size_t offset, size_t sz);

    //! directly copy data to a subrange of the device memory object
    //! \param src      address of data to copy
    //! \param offset   offset from the beginning of the memory object to copy
    //!                 the data to
    //! \param sz       size in bytes of the data to copy
    //!
    //! Note that this operation only works for buffers that are "host visible".
    void _copyDataToBuffer (const void *src, size_t offset, size_t sz);

    //! \brief copy data from the buffer using a staging buffer.
    //! \param dst    the destination for the data
    //! \param offset the source offset in the buffer
    //! \param sz     the size (in bytes) of data to copy
    void _stageDataFromBuffer (void *dst, size_t offset, size_t sz);

};

//! Buffer class for vertex data
class VertexBuffer : public Buffer {
public:

    //! VertexBuffer constuctor
    //! \param app   the application pointer
    //! \param sz    the size (in bytes) of the buffer
    //! \param data  optional pointer to data for initialization
    VertexBuffer (Application *app, size_t sz, const void *data = nullptr);

    //! \brief copy data to the buffer; the amount of data copied is the size of the buffer
    //! \param data the source of the data to copy to the buffer
    void copyTo (const void *data)
    {
        this->_stageDataToBuffer(data, 0, this->_sz);
    }

    //! \brief copy data to the buffer
    //! \param data   the source of the data to copy to the buffer
    //! \param offset the destination offset in the buffer
    //! \param sz     the size (in bytes) of data to copy
    void copyTo (const void *data, size_t offset, size_t sz)
    {
        this->_stageDataToBuffer(data, offset, sz);
    }

};

//! Buffer class for index data
class IndexBuffer : public Buffer {
public:

    //! IndexBuffer constuctor
    //! \param app  the application pointer
    //! \param nIndices  the number of indices in the buffer
    //! \param ty        the type of index (8, 16, or 32 bit)
    //! \param data      optional pointer to data for initialization
    IndexBuffer (Application *app, uint32_t nIndices, VkIndexType ty, const void *data = nullptr);

    //! return the number of indices
    uint32_t nIndices () const { return this->_nIndices; }

    //! return the type of indices
    VkIndexType indexTy () const { return this->_ty; }

    //! \brief copy data to the buffer; the amount of data copied is the size of the buffer
    //! \param data the source of the data to copy to the buffer
    void copyTo (const void *data)
    {
        this->_stageDataToBuffer(data, 0, this->_sz);
    }

    //! \brief copy data to the buffer
    //! \param data   the source of the data to copy to the buffer
    //! \param offset the destination offset in the buffer
    //! \param sz     the size (in bytes) of data to copy
    void copyTo (const void *data, size_t offset, size_t sz)
    {
        this->_stageDataToBuffer(data, offset, sz);
    }

private:
    uint32_t _nIndices;
    VkIndexType _ty;

};

//! Buffer class for uniform data
class UniformBuffer : public Buffer {
public:

    //! UniformBuffer constuctor
    //! \param app  the application pointer
    //! \param sz    the size (in bytes) of the buffer
    UniformBuffer (Application *app, size_t sz);

    //! \brief copy data to the buffer; the amount of data copied is the size of the buffer
    //! \param data the source of the data to copy to the buffer
    void copyTo (const void *data)
    {
        this->_copyDataToBuffer(data, 0, this->_sz);
    }

    //! \brief copy data to the buffer
    //! \param data   the source of the data to copy to the buffer
    //! \param offset the destination offset in the buffer
    //! \param sz     the size (in bytes) of data to copy
    void copyTo (const void *data, size_t offset, size_t sz)
    {
        this->_copyDataToBuffer(data, offset, sz);
    }

};

//! Buffer class for storage buffers, which are used to hold data that is both readable and
//! writable by the GPU.
class StorageBuffer : public Buffer {
public:

    //! StorageBuffer constuctor
    //! \param app  the application pointer
    //! \param sz   the size (in bytes) of the buffer
    //! \param data optional pointer to data for initialization
    StorageBuffer (Application *app, size_t sz, const void *data = nullptr);

    //! \brief copy data to the buffer; the amount of data copied is the size of the buffer
    //! \param data the source of the data to copy to the buffer
    void copyTo (const void *data)
    {
        this->_stageDataToBuffer(data, 0, this->_sz);
    }

    //! \brief copy data to the buffer
    //! \param data   the source of the data to copy to the buffer
    //! \param offset the destination offset in the buffer
    //! \param sz     the size (in bytes) of data to copy
    void copyTo (const void *data, size_t offset, size_t sz)
    {
        this->_stageDataToBuffer(data, offset, sz);
    }

    //! \brief copy data to the buffer
    //! \param data   the destination for the data
    //! \param offset the source offset in the buffer
    //! \param sz     the size (in bytes) of data to copy
    void copyFrom (void *data, size_t offset, size_t sz)
    {
        this->_stageDataFromBuffer(data, offset, sz);
    }

};

} // namespace cs237

#endif // !_CS237_BUFFER_HPP_
