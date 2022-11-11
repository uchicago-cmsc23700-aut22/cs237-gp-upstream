/*! \file memory-obj.cpp
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hpp"

namespace cs237 {

MemoryObj::MemoryObj (Application *app, VkMemoryRequirements const &reqs)
  : _app(app), _sz(reqs.size)
{
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
}

MemoryObj::~MemoryObj ()
{
    vkFreeMemory (this->_app->_device, this->_mem, nullptr);
}

} // namespace cs237
