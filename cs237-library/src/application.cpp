/*! \file application.cpp
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
#include <cstring>
#include <cstdlib>
#include <set>
#include <vector>

namespace cs237 {

static std::vector<const char *> requiredExtensions (bool debug);
static int graphicsQueueIndex (VkPhysicalDevice dev);

const std::vector<const char*> kDeviceExts = {
        "VK_KHR_portability_subset",
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

const std::vector<const char *> kValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };


/******************** class Application methods ********************/

Application::Application (std::vector<const char *> &args, std::string const &name)
  : _name(name),
    _messages(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT),
    _debug(0),
    _gpu(VK_NULL_HANDLE),
    _propsCache(nullptr)
{
    // process the command-line arguments
    for (auto it = args.cbegin();  it != args.cend();  ++it) {
        if (*it[0] == '-') {
            if (strcmp(*it, "-debug") == 0) {
                if (this->_messages > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                    this->_messages = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
                }
                this->_debug = true;
            }
            else if (strcmp(*it, "-verbose")) {
                this->_messages = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
            }
        }
    }

    // initialize GLFW
    glfwInit();

    // create a Vulkan instance
    this->_createInstance();

    // initialize the command pool
    this->_initCommandPool();
}

Application::~Application ()
{
    if (this->_propsCache != nullptr) {
        delete this->_propsCache;
    }

    // delete the command pool
    vkDestroyCommandPool(this->_device, this->_cmdPool, nullptr);

    // destroy the logical device
    vkDestroyDevice(this->_device, nullptr);

    // delete the instance
    vkDestroyInstance(this->_instance, nullptr);

    // shut down GLFW
    glfwTerminate();

}

// create a Vulkan instance
void Application::_createInstance ()
{
    // set up the application information
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = this->_name.data();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = nullptr;
    appInfo.engineVersion = 0;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // figure out what extensions we are going to need
    auto extensions = requiredExtensions(this->_debug);

    // intialize the creation info struct
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.pApplicationInfo = &appInfo;
    if (this->_debug) {
        // in debug mode, we add validation layer(s)
        createInfo.enabledLayerCount = kValidationLayers.size();
        createInfo.ppEnabledLayerNames = kValidationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
    }
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &(this->_instance)) != VK_SUCCESS) {
        ERROR("unable to create a vulkan instance");
    }

    // pick the physical device; we require fillModeNonSolid to support
    // wireframes and samplerAnisotropy for texture mapping
    VkPhysicalDeviceFeatures reqs{};
    reqs.fillModeNonSolid = VK_TRUE;
    reqs.samplerAnisotropy = VK_TRUE;
    this->_selectDevice (&reqs);

    // create the logical device and get the queues
    this->_createLogicalDevice ();
}

// check that a device meets the requested features
//
static bool hasFeatures (VkPhysicalDevice gpu, VkPhysicalDeviceFeatures *reqFeatures)
{
    if (reqFeatures == nullptr) {
        return true;
    }
    VkPhysicalDeviceFeatures availFeatures;
    vkGetPhysicalDeviceFeatures (gpu, &availFeatures);

    if (reqFeatures->fillModeNonSolid == availFeatures.fillModeNonSolid) {
        return true;
    }
    else if (reqFeatures->samplerAnisotropy == availFeatures.samplerAnisotropy) {
        return true;
    }
    else {
        return false;
    }
}

// A helper function to pick the physical device when there is more than one.
// Currently, we ignore the features and favor discrete GPUs over other kinds
//
void Application::_selectDevice (VkPhysicalDeviceFeatures *reqFeatures)
{
    // figure out how many devices are available
    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices(this->_instance, &devCount, nullptr);

    if (devCount == 0) {
        ERROR("no available GPUs");
    }

    // get the available devices
    std::vector<VkPhysicalDevice> devices(devCount);
    vkEnumeratePhysicalDevices(this->_instance, &devCount, devices.data());

    // Select a device that supports graphics and presentation
    // This code is brute force, but we only expect one or two devices.
    // Future versions will support checking for properties.

// FIXME: we should also check that the device supports swap chains!!!!

    // we first look for a discrete GPU
    for (auto & dev : devices) {
        if (hasFeatures(dev, reqFeatures) && this->_getQIndices(dev)) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                this->_gpu = dev;
                return;
            }
        }
    }
    // no discrete GPU, so look for an integrated GPU
    for (auto & dev : devices) {
        if (hasFeatures(dev, reqFeatures) && this->_getQIndices(dev)) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                this->_gpu = dev;
                return;
            }
        }
    }
    // check for any device that supports graphics and presentation
    for (auto & dev : devices) {
        if (hasFeatures(dev, reqFeatures) && this->_getQIndices(dev)) {
            this->_gpu = dev;
            return;
        }
    }

    ERROR("no available GPUs that support graphics");

}

// helper to set the properties cache variable
void Application::_getPhysicalDeviceProperties () const
{
    assert (this->_propsCache == nullptr);

    this->_propsCache = new VkPhysicalDeviceProperties;
    vkGetPhysicalDeviceProperties(this->_gpu, this->_propsCache);
}

int32_t Application::_findMemory (
    uint32_t reqTypeBits,
    VkMemoryPropertyFlags reqProps) const
{
    // get the memory properties for the device
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(this->_gpu, &memProps);

    for (int32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((reqTypeBits & (1 << i))
        && (memProps.memoryTypes[i].propertyFlags & reqProps) == reqProps)
        {
            return i;
        }
    }

    return -1;

}

VkFormat Application::_findBestFormat (
    std::vector<VkFormat> candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features)
{
    for (VkFormat fmt : candidates) {
        VkFormatProperties props = this->formatProps(fmt);
        if (tiling == VK_IMAGE_TILING_LINEAR) {
            if ((props.linearTilingFeatures & features) == features) {
                return fmt;
            }
        } else { // VK_IMAGE_TILING_OPTIMAL
            if ((props.optimalTilingFeatures & features) == features) {
                return fmt;
            }
        }
    }
    return VK_FORMAT_UNDEFINED;
}

VkFormat Application::_depthStencilBufferFormat (bool depth, bool stencil)
{
    if (!depth && !stencil) {
        return VK_FORMAT_UNDEFINED;
    }

    // construct a list of valid candidate formats in best-to-worst order
    std::vector<VkFormat> candidates;
    if (!depth) {
        candidates.push_back(VK_FORMAT_S8_UINT);                // 8-bit stencil; no depth
    }
    if (!stencil) {
        candidates.push_back(VK_FORMAT_D32_SFLOAT);             // 32-bit depth; no stencil
    }
    candidates.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);         // 32-bit depth + 8-bit stencil
    if (!stencil) {
        candidates.push_back(VK_FORMAT_X8_D24_UNORM_PACK32);    // 24-bit depth; no stencil
        candidates.push_back(VK_FORMAT_D16_UNORM);              // 16-bit depth; no stencil
    }
    candidates.push_back(VK_FORMAT_D16_UNORM_S8_UINT);          // 16-bit depth + 8-bit stencil

    return this->_findBestFormat(
        candidates,
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

}

void Application::_createLogicalDevice ()
{
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = nullptr;

    // set up the device queues info struct; the graphics and presentation queues may
    // be different or the same, so we have to initialize either one or two create-info
    // structures
    std::vector<VkDeviceQueueCreateInfo> qCreateInfos;
    std::set<uint32_t> uniqueQIndices = { this->_qIdxs.graphics, this->_qIdxs.present };

    float qPriority = 1.0f;
    for (auto qix : uniqueQIndices) {
        VkDeviceQueueCreateInfo qCreateInfo{};
        qCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qCreateInfo.queueFamilyIndex = qix;
        qCreateInfo.queueCount = 1;
        qCreateInfo.pQueuePriorities = &qPriority;
        qCreateInfos.push_back(qCreateInfo);
    }

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(qCreateInfos.size());
    createInfo.pQueueCreateInfos = qCreateInfos.data();

    // include validation layer if in debug mode
    if (this->_debug) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
        createInfo.ppEnabledLayerNames = kValidationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // set up the enabled extensions to include swap chains
    createInfo.enabledExtensionCount = static_cast<uint32_t>(kDeviceExts.size());
    createInfo.ppEnabledExtensionNames = kDeviceExts.data();

    // for now, we are only enabling a couple of extra features
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    createInfo.pEnabledFeatures = &deviceFeatures;

    // create the logical device
    if (vkCreateDevice(this->_gpu, &createInfo, nullptr, &this->_device) != VK_SUCCESS) {
        ERROR("unable to create logical device!");
    }

    // get the queues
    vkGetDeviceQueue(this->_device, this->_qIdxs.graphics, 0, &this->_queues.graphics);
    vkGetDeviceQueue(this->_device, this->_qIdxs.present, 0, &this->_queues.present);

}

// create a Vulkan image; used for textures, depth buffers, etc.
VkImage Application::_createImage (
    uint32_t wid,
    uint32_t ht, VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    uint32_t mipLvls)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = wid;
    imageInfo.extent.height = ht;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLvls;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImage image;
    auto sts = vkCreateImage(this->_device, &imageInfo, nullptr, &image);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create image!");
    }

    return image;
}

VkDeviceMemory Application::_allocImageMemory (VkImage img, VkMemoryPropertyFlags props)
{
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(this->_device, img, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = this->_findMemory(memRequirements.memoryTypeBits, props);

    VkDeviceMemory mem;
    if (vkAllocateMemory(this->_device, &allocInfo, nullptr, &mem) != VK_SUCCESS) {
        ERROR("unable to allocate image memory!");
    }

    vkBindImageMemory(this->_device, img, mem, 0);

    return mem;
}

VkImageView Application::_createImageView (
    VkImage img,
    VkFormat fmt,
    VkImageAspectFlags aspectFlags)
{
    assert (img != VK_NULL_HANDLE);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = img;
    viewInfo.format = fmt;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    auto sts = vkCreateImageView(this->_device, &viewInfo, nullptr, &imageView);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create texture image view!");
    }

    return imageView;

}

VkBuffer Application::_createBuffer (size_t size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buf;
    if (vkCreateBuffer(this->_device, &bufferInfo, nullptr, &buf) != VK_SUCCESS) {
        ERROR("unable to create buffer!");
    }

    return buf;
}

VkDeviceMemory Application::_allocBufferMemory (VkBuffer buf, VkMemoryPropertyFlags props)
{
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(this->_device, buf, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = this->_findMemory(memRequirements.memoryTypeBits, props);

    VkDeviceMemory mem;
    if (vkAllocateMemory(this->_device, &allocInfo, nullptr, &mem) != VK_SUCCESS) {
        ERROR("unable to allocate buffer memory!");
    }

    vkBindBufferMemory(this->_device, buf, mem, 0);

    return mem;

}

void Application::_transitionImageLayout (
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout)
{
    VkCommandBuffer cmdBuf = this->newCommandBuf();

    this->beginCommands(cmdBuf, true);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
    && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    && (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        ERROR("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        cmdBuf, srcStage, dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    this->endCommands(cmdBuf);
    this->submitCommands(cmdBuf);
    this->freeCommandBuf(cmdBuf);

}

void Application::_copyBuffer (VkBuffer srcBuf, VkBuffer dstBuf, size_t offset, size_t size)
{
    VkCommandBuffer cmdBuf = this->newCommandBuf();

    this->beginCommands(cmdBuf, true);

    VkBufferCopy copyRegion{};
    copyRegion.dstOffset = offset;
    copyRegion.size = size;
    vkCmdCopyBuffer(cmdBuf, srcBuf, dstBuf, 1, &copyRegion);

    this->endCommands(cmdBuf);
    this->submitCommands(cmdBuf);
    this->freeCommandBuf(cmdBuf);

}

void Application::_copyBufferToImage (
        VkImage dstImg, VkBuffer srcBuf, size_t size,
        uint32_t wid, uint32_t ht, uint32_t depth)
{
    VkCommandBuffer cmdBuf = this->newCommandBuf();

    this->beginCommands(cmdBuf);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = { wid, ht, depth };

    vkCmdCopyBufferToImage(
        cmdBuf, srcBuf, dstImg,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    this->endCommands(cmdBuf);
    this->submitCommands(cmdBuf);
    this->freeCommandBuf(cmdBuf);

}

void Application::_initCommandPool ()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = this->_qIdxs.graphics;

    auto sts = vkCreateCommandPool(this->_device, &poolInfo, nullptr, &this->_cmdPool);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create command pool!");
    }
}

VkCommandBuffer Application::newCommandBuf ()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->_cmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuf;
    auto sts = vkAllocateCommandBuffers(this->_device, &allocInfo, &cmdBuf);
    if (sts != VK_SUCCESS) {
        ERROR("unable to allocate command buffer!");
    }

    return cmdBuf;
}

void Application::beginCommands (VkCommandBuffer cmdBuf, bool oneTime)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (oneTime) {
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }

    if (vkBeginCommandBuffer(cmdBuf, &beginInfo) != VK_SUCCESS) {
        ERROR("unable to begin recording command buffer!");
    }
}

void Application::endCommands (VkCommandBuffer cmdBuf)
{
    if (vkEndCommandBuffer(cmdBuf) != VK_SUCCESS) {
        ERROR("unable to record command buffer!");
    }
}

void Application::submitCommands (VkCommandBuffer cmdBuf)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    auto grQ = this->_queues.graphics;
    vkQueueSubmit(grQ, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(grQ);

}

VkSampler Application::createSampler (Application::SamplerInfo const &info)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = info.magFilter;
    samplerInfo.minFilter = info.minFilter;
    samplerInfo.mipmapMode = info.mipmapMode;
    samplerInfo.addressModeU = info.addressModeU;
    samplerInfo.addressModeV = info.addressModeV;
    samplerInfo.addressModeW = info.addressModeW;
    samplerInfo.borderColor = info.borderColor;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = this->limits()->maxSamplerAnisotropy;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    VkSampler sampler;
    auto sts = vkCreateSampler(this->_device, &samplerInfo, nullptr, &sampler);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create texture sampler!");
    }

    return sampler;
}

// Get the list of supported extensions
//
std::vector<VkExtensionProperties> Application::supportedExtensions ()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extProps(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extProps.data());
    return extProps;
}

// Get the list of supported layers
//
std::vector<VkLayerProperties> Application::supportedLayers ()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    return layers;
}

VkPipeline Application::createPipeline (
    cs237::Shaders *shaders,
    VkPipelineVertexInputStateCreateInfo const &vertexInfo,
    VkPrimitiveTopology prim,
    bool primRestart,
    VkPolygonMode polyMode,
    VkCullModeFlags cullMode,
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    uint32_t subPass,
    std::vector<VkDynamicState> const &dynamic)
{
    VkPipelineInputAssemblyStateCreateInfo asmInfo{};
    asmInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    asmInfo.topology = prim;
    asmInfo.primitiveRestartEnable = primRestart ? VK_TRUE : VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_TRUE;
    rasterizer.polygonMode = polyMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineDepthStencilStateCreateInfo *pDepthStencil;
    // set up the depth/stencil-buffer state
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    pDepthStencil = &depthStencil;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    if (dynamic.size() > 0) {
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamic.size());
        dynamicState.pDynamicStates = dynamic.data();
    } else {
        dynamicState.dynamicStateCount = 0;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaders->numStages();
    pipelineInfo.pStages = shaders->stages();
    pipelineInfo.pVertexInputState = &vertexInfo;
    pipelineInfo.pInputAssemblyState = &asmInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline;
    auto sts = vkCreateGraphicsPipelines(
        this->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
        &pipeline);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create graphics pipeline!");
    }

    return pipeline;
}


/******************** local utility functions ********************/

// A helper function for determining the extensions that are required
// when creating an instance. These include the extensions required
// by GLFW and the extensions required for debugging support when
// `debug` is true.
//
static std::vector<const char *> requiredExtensions (bool debug)
{
    uint32_t extCount;

    // extensions required by GLFW
    const char **glfwReqExts = glfwGetRequiredInstanceExtensions(&extCount);

    // in debug mode we need the debug utilities
    uint32_t debugExtCount = debug ? 1 : 0;

    // initialize the vector of extensions with the GLFW extensions
    std::vector<const char *> reqExts (glfwReqExts, glfwReqExts+extCount);

    reqExts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    reqExts.push_back("VK_KHR_get_physical_device_properties2");

    // add debug extensions
    if (debug) {
        reqExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return reqExts;
}

// check the device's queue families for graphics and presentation support
//
bool Application::_getQIndices (VkPhysicalDevice dev)
{
    // first we figure out how many queue families the device supports
    uint32_t qFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qFamilyCount, nullptr);

    // then we get the queue family info
    std::vector<VkQueueFamilyProperties> qFamilies(qFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qFamilyCount, qFamilies.data());

    Application::Queues<int32_t> indices = { -1, -1 };
    for (int i = 0;  i < qFamilyCount;  ++i) {
        // check for graphics support
        if ((indices.graphics < 0)
        && (qFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.graphics = i;
        }
        // check for presentation support
        if (indices.present < 0) {
            if (glfwGetPhysicalDevicePresentationSupport(this->_instance, dev, i)) {
                indices.present = i;
            }
        }
        // check if we are finished
        if ((indices.graphics >= 0) && (indices.present >= 0)) {
            this->_qIdxs.graphics = static_cast<uint32_t>(indices.graphics);
            this->_qIdxs.present = static_cast<uint32_t>(indices.present);
            return true;
        }
    }


    return false;

}

} // namespace cs237
