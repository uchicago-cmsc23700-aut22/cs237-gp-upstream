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
    uint32_t wid, uint32_t ht, uint32_t mipLvls,
    cs237::__detail::ImageBase const *img)
  : _app(app), _wid(wid), _ht(ht), _fmt(img->format())
{
    this->_img = app->_createImage (
        wid, ht, this->_fmt,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        mipLvls);
    this->_mem = app->_allocImageMemory(
        this->_img,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    this->_view = app->_createImageView(
        this->_img, this->_fmt,
        VK_IMAGE_ASPECT_COLOR_BIT);

}

TextureBase::~TextureBase ()
{
    vkDestroyImageView(this->_app->_device, this->_view, nullptr);
    vkDestroyImage(this->_app->_device, this->_img, nullptr);
    vkFreeMemory(this->_app->_device, this->_mem, nullptr);
}

void TextureBase::_init (cs237::__detail::ImageBase const *img)
{
    void *data = img->data();
    size_t nBytes = img->nBytes();
    auto device = this->_app->_device;

    // create a staging buffer for copying the image
    VkBuffer stagingBuf = this->_createBuffer (
        nBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    VkDeviceMemory stagingBufMem = this->_allocBufferMemory(
        stagingBuf,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // copy the image data to the staging buffer
    void* stagingData;
    vkMapMemory(device, stagingBufMem, 0, nBytes, 0, &stagingData);
    memcpy(stagingData, data, nBytes);
    vkUnmapMemory(device, stagingBufMem);

    this->_app->_transitionImageLayout(
        this->_img, this->_fmt,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    this->_app->_copyBufferToImage(this->_img, stagingBuf, nBytes, this->_wid, this->_ht);
    this->_app->_transitionImageLayout(
        this->_img, this->_fmt,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // free up the staging buffer
    vkFreeMemory(device, stagingBufMem, nullptr);
    vkDestroyBuffer(device, stagingBuf, nullptr);
}

} // namespce __detail

/******************** class Texture1D methods ********************/

Texture1D::Texture1D (Application *app, Image1D const *img)
  : __detail::TextureBase(app, img->width(), 1, 1, img)
{
    this->_init(img);
}

/******************** class Texture2D methods ********************/

// return the integer log2 of n; if n is not a power of 2, then return -1.
static int32_t ilog2 (uint32_t n)
{
    uint32_t k = 0, two_k = 1;
    while (two_k < n) {
        k++;
        two_k *= 2;
        if (two_k == n)
            return k;
    }
    return -1;

}

// compute the number of mipmap levels for an image.  This value is log2 of
// the larger dimension plus one for the base level image.  We require that
// both dimensions be a power of 2.
static uint32_t mipLevels (Image2D const *img, bool mipmap)
{
    if (mipmap) {
        int32_t log2Wid = ilog2(img->width());
        int32_t log2Ht = ilog2(img->height());
        if ((log2Wid < 0) || (log2Ht < 0)) {
            ERROR("texture size not a power of 2");
        }
        return std::max(log2Wid, log2Ht) + 1;
    }
    else {
        return 1;
    }
}

Texture2D::Texture2D (Application *app, Image2D const *img, bool mipmap)
  : __detail::TextureBase(app, img->width(), img->height(), mipLevels(img, mipmap), img)
{
    this->_init (img);

    if (mipmap) {
        this->_generateMipMaps();
    }
}

// helper function for generating the mipmaps for a texture
void Texture2D::_generateMipMaps ()
{
    // Check if image format supports linear blitting
    VkFormatProperties props = this->_app->formatProps(this->_fmt);
    if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        ERROR("texture image format does not support linear blitting!");
    }

    VkCommandBuffer cmdBuf = this->_app->newCommandBuf();

    this->_app->beginCommands(cmdBuf);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = this->_img;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWid = this->_wid;
    int32_t mipHt = this->_ht;

    // compute the mipmap levels; note that level 0 is the base image
    for (uint32_t i = 1; i < this->_nMipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        int32_t nextWid = (mipWid > 1) ? (mipWid >> 1) : 1;
        int32_t nextHt = (mipHt > 1) ? (mipHt >> 1) : 1;

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWid, mipHt, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { nextWid, nextHt, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmdBuf,
            this->_img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            this->_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        mipWid = nextWid;
        mipHt = nextHt;
    }

    barrier.subresourceRange.baseMipLevel = this->_nMipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuf,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    this->_app->endCommands(cmdBuf);
    this->_app->submitCommands(cmdBuf);
    this->_app->freeCommandBuf(cmdBuf);

}

} // namespace cs237
