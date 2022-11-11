/*! \file cs237-memory-obj.hpp
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_MEMORY_OBJ_HPP_
#define _CS237_MEMORY_OBJ_HPP_

#ifndef _CS237_HPP_
#error "cs237-memory.hpp should not be included directly"
#endif

namespace cs237 {

//! wrapper around Vulkan memory objects
class MemoryObj {
    friend class Buffer;

public:
    MemoryObj (Application *app, VkMemoryRequirements const &reqs);
    ~MemoryObj ();

    //! copy data to a subrange of the device memory object
    //! \param src      address of data to copy
    //! \param offset   offset from the beginning of the memory object to copy
    //!                 the data to
    //! \param sz       size in bytes of the data to copy
    void copyTo (const void *src, size_t offset, size_t sz)
    {
        assert (offset + sz <= this->_sz);

        // first we need to map the object into our address space
        void *dst;
        auto sts = vkMapMemory(
                this->_app->_device,
                this->_mem,
                offset, this->_sz, 0,
                &dst);
        if (sts != VK_SUCCESS) {
            ERROR ("unable to map memory object");
        }
        // copy the data
        memcpy(dst, src, sz);
        // unmap the object
        vkUnmapMemory (this->_app->_device, this->_mem);
    }

    //! copy data to the device memory object
    //! \param src      address of data to copy
    void copyTo (const void *src) { this->copyTo(src, 0, this->_sz); }

    size_t size () const { return this->_sz; }

protected:
    Application *_app;          //!< the application
    VkDeviceMemory _mem;        //!< the device memory object
    size_t _sz;                 //!< the size of the memory object

};

} // namespace cs237

#endif // !_CS237_MEMORY_OBJ_HPP_
