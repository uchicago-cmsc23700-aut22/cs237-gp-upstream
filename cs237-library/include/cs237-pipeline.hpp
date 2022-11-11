/*! \file cs237-pipeline.hpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * This file contains a number of helper functions for
 * defining a graphics pipeline.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_PIPELINE_HPP_
#define _CS237_PIPELINE_HPP_

#ifndef _CS237_HPP_
#error "cs237-pipeline.hpp should not be included directly"
#endif

namespace cs237 {

//! \brief initialize info for the vertex input stage of the pipeline
//! \param descs   vector of binding descriptors
//! \param attrs   vector of input attributes
//!
//! The resources allocated by this function should be released by calling
//! the `destroyVertexInputInfo` function.
inline
VkPipelineVertexInputStateCreateInfo vertexInputInfo (
    std::vector<VkVertexInputBindingDescription> const &descs,
    std::vector<VkVertexInputAttributeDescription> const &attrs)
{
    VkPipelineVertexInputStateCreateInfo vertexInfo{};
    vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInfo.vertexBindingDescriptionCount = descs.size();
    if (descs.size() > 0) {
        // we copy the data into the heap to ensure that it is still around
        // when we create the pipeline
        auto pDescs = new VkVertexInputBindingDescription[descs.size()];
        for (int i = 0;  i < descs.size();  ++i) {
            pDescs[i] = descs[i];
        }
        vertexInfo.pVertexBindingDescriptions = pDescs;
    } else {
        vertexInfo.pVertexBindingDescriptions = nullptr;
    }

    vertexInfo.vertexAttributeDescriptionCount = attrs.size();
    if (attrs.size() > 0) {
        auto pAttrs = new VkVertexInputAttributeDescription[attrs.size()];
        for (int i = 0;  i < attrs.size();  ++i) {
            pAttrs[i] = attrs[i];
        }
        vertexInfo.pVertexAttributeDescriptions = pAttrs;
    } else {
        vertexInfo.pVertexAttributeDescriptions = nullptr;
    }

    return vertexInfo;
}

//! release resources allocated by vertexInputInfo
inline
void destroyVertexInputInfo (VkPipelineVertexInputStateCreateInfo &info)
{
    if (info.pVertexBindingDescriptions != nullptr) {
        delete info.pVertexBindingDescriptions;
        info.pVertexBindingDescriptions = nullptr;
    }
    if (info.pVertexAttributeDescriptions != nullptr) {
        delete info.pVertexAttributeDescriptions;
        info.pVertexAttributeDescriptions = nullptr;
    }
}

} // namespace cs237

#endif // !_CS237_PIPELINE_HPP_
