/*! \file buffer.cpp
 *
 * Buffer objects with backing device memory.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hpp"

namespace cs237 {

Buffer::Buffer (Application *app, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, size_t sz)
  : _app(app), _buf(app->_createBuffer (sz, usage)), _mem(VK_NULL_HANDLE), _sz(sz)
{
    // get memory requirements
    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(this->_app->_device, this->_buf, &reqs);

    // allocate the memory object for the buffer
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    // the allocated memory size must be a multiple of the alignment
    allocInfo.allocationSize =
        (reqs.size + reqs.alignment - 1) & ~(reqs.alignment - 1);
    allocInfo.memoryTypeIndex = app->_findMemory(
        reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    auto sts = vkAllocateMemory(app->_device, &allocInfo, nullptr, &this->_mem);
    if (sts != VK_SUCCESS) {
        ERROR("failed to allocate vertex buffer memory!");
    }

    // bind the buffer to the memory
    sts = vkBindBufferMemory(app->_device, this->_buf, this->_mem, 0);
    if (sts != VK_SUCCESS) {
        ERROR ("unable to bind buffer to memory object.");
    }

}

Buffer::~Buffer ()
{
    vkFreeMemory (this->_app->_device, this->_mem, nullptr);
    vkDestroyBuffer (this->_app->_device, this->_buf, nullptr);
}

void Buffer::_copyDataToBuffer (const void *src, size_t offset, size_t sz)
{
    assert (offset + sz <= this->_sz);
    assert (sz > 0);

    // first we need to map the buffer's memory object into our address space
    void *dst;
    auto sts = vkMapMemory(this->_app->_device, this->_mem, offset, sz, 0, &dst);
    if (sts != VK_SUCCESS) {
        ERROR ("unable to map memory object");
    }
    // copy the data
    memcpy(dst, src, sz);
    // unmap the memory object
    vkUnmapMemory (this->_app->_device, this->_mem);
}

void Buffer::_stageDataToBuffer (const void *src, size_t offset, size_t sz)
{
    assert (offset + sz <= this->_sz);
    assert (sz > 0);

    // allocate a staging buffer that is host visible
    Buffer stagingBuf(
        this->_app,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sz);

    // copy the data to the staging buffer
    stagingBuf._copyDataToBuffer (src, 0, sz);

    // use the GPU to copy the data from the staging buffer to this buffer
    this->_app->_copyBuffer (stagingBuf._buf, this->_buf, offset, sz);

}

/***** class VertexBuffer methods *****/

VertexBuffer::VertexBuffer (Application *app, size_t sz, const void *data)
  : Buffer (
        app,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        sz)
{
    if (data != nullptr) {
        this->copyTo (data, 0, sz);
    }
}

/***** class IndexBuffer methods *****/

// helper function for computing the size of an index buffer in bytes
inline size_t indexBufSz (uint32_t nIndices, VkIndexType ty)
{
    switch (ty) {
    case VK_INDEX_TYPE_UINT16: return static_cast<size_t>(2 * nIndices);
    case VK_INDEX_TYPE_UINT32: return static_cast<size_t>(4 * nIndices);
    case VK_INDEX_TYPE_UINT8_EXT: return static_cast<size_t>(nIndices);
    default: ERROR("invalid index type for index buffer");
    }
}

IndexBuffer::IndexBuffer (Application *app, uint32_t nIndices, VkIndexType ty, const void *data)
  : Buffer (
        app,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBufSz(nIndices, ty)),
    _nIndices(nIndices),
    _ty(ty)
{
    if (data != nullptr) {
        this->copyTo (data, 0, this->_sz);
    }
}

/***** class UniformBuffer methods *****/

UniformBuffer::UniformBuffer (Application *app, size_t sz)
  : Buffer (
        app,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sz)
{ }

} // namespace cs237
