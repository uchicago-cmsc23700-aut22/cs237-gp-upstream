/*! \file texture.cpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hpp"

namespace cs237 {

namespace __detail {

TextureBase::TextureBase (
    Application *app,
    uint32_t wid, uint32_t ht,
    cs237::__detail::ImageBase const *img)
  : _app(app)
{
    void *data = img->data();
    size_t nBytes = img->nBytes();
    VkFormat fmt = img->format();

    this->_img = app->_createImage (
        wid, ht, fmt,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    this->_mem = app->_allocImageMemory(
        this->_img,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    this->_view = app->_createImageView(
        this->_img, fmt,
        VK_IMAGE_ASPECT_COLOR_BIT);

    // create a staging buffer for copying the image
    VkBuffer stagingBuf = this->_createBuffer (
        nBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    VkDeviceMemory stagingBufMem = this->_allocBufferMemory(
        stagingBuf,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // copy the image data to the staging buffer
    void* stagingData;
    vkMapMemory(app->_device, stagingBufMem, 0, nBytes, 0, &stagingData);
    memcpy(stagingData, data, nBytes);
    vkUnmapMemory(app->_device, stagingBufMem);

    app->_transitionImageLayout(
        this->_img, fmt,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    app->_copyBufferToImage(this->_img, stagingBuf, nBytes, wid, ht);
    app->_transitionImageLayout(
        this->_img, fmt,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // free up the staging buffer
    vkFreeMemory(app->_device, stagingBufMem, nullptr);
    vkDestroyBuffer(app->_device, stagingBuf, nullptr);

}

TextureBase::~TextureBase ()
{
    vkDestroyImageView(this->_app->_device, this->_view, nullptr);
    vkDestroyImage(this->_app->_device, this->_img, nullptr);
    vkFreeMemory(this->_app->_device, this->_mem, nullptr);
}

} // namespce __detail

/******************** class Texture1D methods ********************/

Texture1D::Texture1D (Application *app, Image1D const *img)
  : __detail::TextureBase(app, img->width(), 1, img)
{
}

/******************** class Texture2D methods ********************/

Texture2D::Texture2D (Application *app, Image2D const *img, bool mipmap)
  : __detail::TextureBase(app, img->width(), img->height(), img)
{
    if (mipmap) {
        ERROR("mipmap generation not supported yet");
    }
}

} // namespace cs237
