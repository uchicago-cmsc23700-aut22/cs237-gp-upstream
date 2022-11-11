/*! \file cs237-buffer.hpp
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

//! A base class for buffer objects of all kinds
class Buffer {
public:
    VkBuffer vkBuffer () const { return this->_buf; }

    void bindMemory (MemoryObj *memObj)
    {
        auto sts = vkBindBufferMemory(
            this->_app->_device,
            this->_buf,
            memObj->_mem,
            0);
        if (sts != VK_SUCCESS) {
            ERROR ("unable to bind buffer to memory object.");
        }
    }

    //! get the memory requirements of this buffer
    VkMemoryRequirements requirements ()
    {
        VkMemoryRequirements reqs;
        vkGetBufferMemoryRequirements(this->_app->_device, this->_buf, &reqs);
        return reqs;
    }

protected:
    Application *_app;          //!< the application
    VkBuffer    _buf;           //!< the Vulkan buffer object

    Buffer (Application *app, VkBufferUsageFlags usage, size_t sz)
      : _app(app)
    {
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.pNext = nullptr;
        info.usage = usage;
        info.size = sz;
        info.queueFamilyIndexCount = 0;
        info.pQueueFamilyIndices = nullptr;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.flags = 0;

        auto sts = vkCreateBuffer (app->_device, &info, nullptr, &this->_buf);
        if (sts != VK_SUCCESS) {
            ERROR ("unable to create buffer object.");
        }
    }

    ~Buffer ()
    {
        vkDestroyBuffer (this->_app->_device, this->_buf, nullptr);
    }

};

//! Buffer class for vertex data
class VertexBuffer : public Buffer {
public:

    VertexBuffer (Application *app, size_t sz)
      : Buffer (app, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sz)
    { }
};

//! Buffer class for index data
class IndexBuffer : public Buffer {
public:

    IndexBuffer (Application *app, uint32_t nIndices, size_t sz)
      : Buffer (app, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sz), _nIndices(nIndices)
    { }

    uint32_t nIndices () const { return this->_nIndices; }

private:
    uint32_t _nIndices;

};

//! Buffer class for uniform data
class UniformBuffer : public Buffer {
public:

    UniformBuffer (Application *app, size_t sz)
      : Buffer (app, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sz)
    { }
};

} // namespace cs237

#endif // !_CS237_BUFFER_HPP_
