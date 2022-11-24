/*! \file cs237-application.hpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_APPLICATION_HPP_
#define _CS237_APPLICATION_HPP_

#ifndef _CS237_HPP_
#error "cs237-application.hpp should not be included directly"
#endif

namespace cs237 {

namespace __detail { class TextureBase; }

//! the base class for applications
class Application {

friend class Window;
friend class Buffer;
friend class __detail::TextureBase;
friend class Texture1D;
friend class Texture2D;

public:

    //! \brief constructor for application base class
    //! \param args     vector of the command-line arguments
    //! \param name     optional name of the application
    Application (std::vector<const char *> &args, std::string const &name = "CS237 App");

    virtual ~Application ();

    //! main function for running the application
    virtual void run () = 0;

    //! \brief return the application name
    std::string name () const { return this->_name; }

    //! \brief is the program in debug mode?
    bool debug () const { return this->_debug; }
    //! \brief is the program in verbose mode?
    bool verbose () const
    {
        return this->_messages == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    }

    //! \brief Get the list of supported Vulkan instance extensions
    //! \return The vector of VkExtensionProperties for the supported extensions
    static std::vector<VkExtensionProperties> supportedInstanceExtensions ();

    //! \brief Get the list of supported Vulkan device extensions for the selected
    //!        physical device.
    //! \return The vector of VkExtensionProperties for the supported extensions
    std::vector<VkExtensionProperties> supportedDeviceExtensions ();

    //! \brief Get the list of supported layers
    //! \return The vector of VkLayerProperties for the supported layers
    static std::vector<VkLayerProperties> supportedLayers ();

    //! Information for creating a sampler object.  This is more limited than Vulkan's
    //! VkSamplerCreateInfo structure, but should cover the common cases used in this
    //! class.
    struct SamplerInfo {
        VkFilter magFilter;
        VkFilter minFilter;
        VkSamplerMipmapMode mipmapMode;
        VkSamplerAddressMode addressModeU;
        VkSamplerAddressMode addressModeV;
        VkSamplerAddressMode addressModeW;
        VkBorderColor borderColor;

        SamplerInfo ()
          : magFilter(VK_FILTER_LINEAR), minFilter(VK_FILTER_LINEAR),
            mipmapMode(VK_SAMPLER_MIPMAP_MODE_LINEAR),
            addressModeU(VK_SAMPLER_ADDRESS_MODE_REPEAT),
            addressModeV(VK_SAMPLER_ADDRESS_MODE_REPEAT),
            addressModeW(VK_SAMPLER_ADDRESS_MODE_REPEAT),
            borderColor(VK_BORDER_COLOR_INT_OPAQUE_BLACK)
        { }

        //! sampler info for 1D texture
        SamplerInfo (
            VkFilter magF, VkFilter minF, VkSamplerMipmapMode mm,
            VkSamplerAddressMode am, VkBorderColor color)
          : magFilter(magF), minFilter(minF), mipmapMode(mm),
            addressModeU(am), addressModeV(VK_SAMPLER_ADDRESS_MODE_REPEAT),
            addressModeW(VK_SAMPLER_ADDRESS_MODE_REPEAT), borderColor(color)
        { }

        //! sampler info for 2D texture
        SamplerInfo (
            VkFilter magF, VkFilter minF, VkSamplerMipmapMode mm,
            VkSamplerAddressMode am1, VkSamplerAddressMode am2,
            VkBorderColor color)
          : magFilter(magF), minFilter(minF), mipmapMode(mm),
            addressModeU(am1), addressModeV(am2),
            addressModeW(VK_SAMPLER_ADDRESS_MODE_REPEAT), borderColor(color)
        { }

    };

    //! \brief Create a texture sampler as specified
    //! \param info  a simplified sampler specification
    //! \return the created sampler
    VkSampler createSampler (SamplerInfo const &info);

    //! \brief get the logical device
    VkDevice device () const { return this->_device; }

    //! \brief access function for the physical device limits
    const VkPhysicalDeviceLimits *limits () const { return &this->_props()->limits; }

    //! \brief access function for the properties of an image format
    VkFormatProperties formatProps (VkFormat fmt) const
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(this->_gpu, fmt, &props);
        return props;
    }

    //! \brief Create a pipeline layout
    //! \param descSets the descriptor sets for the pipeline
    //! \param pcrs the push-constant ranged for the pipeline
    //! \return the created pipeline layout object
    //!
    //! Note that the Vulkan specification recommends using multiple
    //! descriptor sets when you have uniforms that are updted at different
    //! frequencies (e.g., per-scene vs. per-object).  It also recommends
    //! putting the least frequently changing descriptor sets at the
    //! beginning.
    VkPipelineLayout createPipelineLayout (
        std::vector<VkDescriptorSetLayout> descSets,
        std::vector<VkPushConstantRange> pcrs);

    //! \brief Create a pipeline layout for a single descriptor set
    VkPipelineLayout createPipelineLayout (VkDescriptorSetLayout descSet);

    //! \brief Allocate a graphics pipeline
    //! \param shaders     shaders for the pipeline
    //! \param vertexInfo  vertex info
    //! \param prim        primitive topology
    //! \param primRestart true if primitive restart should be enabled
    //! \param viewports   vector of viewports; ignored if the viewport state is dynamic
    //! \param scissors    vector of scissor rectangles; ignored if the scissor state is dynamic
    //! \param polyMode    polygon mode
    //! \param cullMode    primitive culling mode
    //! \param front       the winding order that defines the front face of a triangle
    //! \param layout      the pipeline layout
    //! \param renderPass  a render pass that is compatible with the render pass to be used
    //! \param subPass     the index of the subpass in the render pass where the pipeline
    //!                    will be used
    //! \param dynamic     vector that specifies which parts of the pipeline can be
    //!                    dynamically set during the
    //! \return the created pipeline
    VkPipeline createPipeline (
        cs237::Shaders *shaders,
        VkPipelineVertexInputStateCreateInfo const &vertexInfo,
        VkPrimitiveTopology prim,
        bool primRestart,
        std::vector<VkViewport> const &viewports,
        std::vector<VkRect2D> const &scissors,
        VkPolygonMode polyMode,
        VkCullModeFlags cullMode,
        VkFrontFace front,
        VkPipelineLayout layout,
        VkRenderPass renderPass,
        uint32_t subPass,
        std::vector<VkDynamicState> const &dynamic);

    //! \brief create and initialize a command buffer
    //! \return the fresh command buffer
    VkCommandBuffer newCommandBuf ();

    //! \brief begin recording commands in the given command buffer
    //! \param cmdBuf the command buffer to use for recording commands
    //! \param oneTime true if this command buffer is only going to be used once
    void beginCommands (VkCommandBuffer cmdBuf, bool oneTime = false);

    //! \brief end the recording of commands in the give command buffer
    //! \param cmdBuf the command buffer that we are recording in
    void endCommands (VkCommandBuffer cmdBuf);

    //! \brief end the commands and submit the buffer to the graphics queue.
    //! \param cmdBuf the command buffer to submit
    void submitCommands (VkCommandBuffer cmdBuf);

    //! \brief free the command buffer
    //! \param cmdBuf the command buffer to free
    void freeCommandBuf (VkCommandBuffer & cmdBuf)
    {
        vkFreeCommandBuffers(this->_device, this->_cmdPool, 1, &cmdBuf);
    }

protected:
    //! information about queue families
    template <typename T>
    struct Queues {
        T graphics;             //!< the queue family that supports graphics
        T present;              //!< the queue family that supports presentation
    };

    // information about swap-chain support
    struct SwapChainDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    std::string _name;          //!< the application name
    int _messages;              //!< set to the message severity level
    bool _debug;                //!< set when validation layers should be enabled
    VkInstance _instance;       //!< the Vulkan instance used by the application
    VkPhysicalDevice _gpu;      //!< the graphics card (aka device) that we are using
    mutable VkPhysicalDeviceProperties *_propsCache;
                                //!< a cache of the physical device properties
    VkDevice _device;           //!< the logical device that we are using to render
    Queues<uint32_t> _qIdxs;    //!< the queue family indices
    Queues<VkQueue> _queues;    //!< the device queues that we are using
    VkCommandPool _cmdPool;     //!< pool for allocating command buffers

    //! \brief A helper function to create and initialize the Vulkan instance
    //! used by the application.
    void _createInstance ();

    //! get the physical-device properties pointer
    const VkPhysicalDeviceProperties *_props () const
    {
        if (this->_propsCache == nullptr) {
            this->_getPhysicalDeviceProperties();
        }
        return this->_propsCache;
    }

    //! \brief function that gets the physical-device properties and caches the
    //!        pointer in the `_propsCache` field.
    void _getPhysicalDeviceProperties () const;

    //! \brief A helper function to select the GPU to use
    //! \param reqFeatures  points to a structure specifying the required features
    //!                     of the selected device.
    void _selectDevice (VkPhysicalDeviceFeatures *reqFeatures = nullptr);

    //! \brief A helper function to identify the index of a device memory type
    //!        that has the required type and properties
    //! \param reqTypeBits  bit mask that specifies the possible memory types
    //! \param reqProps     memory property bit mask
    //! \return the index of the lowest set bit in reqTypeBits that has the
    //!         required properties.  If no such memory exists, then -1 is returned.
    int32_t _findMemory (uint32_t reqTypeBits, VkMemoryPropertyFlags reqProps) const;

    //! \brief A helper function to identify the best image format supported by the
    //!        device from an ordered list of candidate formats
    //! \param candidates   list of candidates in order of preference
    //! \param tiling       how pixels are to be tiled in the image (linear vs optimal)
    //! \param features     required features for the format
    //! \return the first format in the list of candidates that has the required features
    //!         for the specified tiling.  VK_FORMAT_UNDEFINED is returned if there is
    //!         no valid candidate
    VkFormat _findBestFormat (
        std::vector<VkFormat> candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features);

    //! \brief allocate the command pool for the application
    void _initCommandPool ();

    //! \brief A helper function to identify the best depth/stencil-buffer attachment
    //!        format for the device
    //! \param depth    set to true if requesting depth-buffer support
    //! \param stencil  set to true if requesting stencil-buffer support
    //! \return the format that has the requested buffer support and the best precision.
    //!         Returns VK_FORMAT_UNDEFINED is `depth` and `stencil` are both false or
    //!         if there s no depth-buffer support
    VkFormat _depthStencilBufferFormat (bool depth, bool stencil);

    //! \brief A helper function to identify the queue-family indices for the
    //!        physical device that we are using.
    //! \return `true` if the device supports all of the required queue types and `false`
    //!        otherwise.
    //!
    //! If this function returns `true`, then the `_qIdxs` instance variable will be
    //! initialized to the queue family indices that were detected.
    bool _getQIndices (VkPhysicalDevice dev);

    //! \brief A helper function to create the logical device during initialization
    //!
    //! This function initializes the `_device`, `_qIdxs`, and `_queues`
    //! instance variables.
    void _createLogicalDevice ();

    //! \brief A helper function for creating a Vulkan image that can be used for
    //!        textures or depth buffers
    //! \param wid      the image width
    //! \param ht       the image height
    //! \param format   the pixel format for the image
    //! \param tiling   the tiling method for the pixels (device optimal vs linear)
    //! \param usage    flags specifying the usage of the image
    //! \param mipLvls  number of mipmap levels for the image (default = 1)
    //! \return the created image
    VkImage _createImage (
        uint32_t wid,
        uint32_t ht,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        uint32_t mipLvls);

    //! \brief A helper function for allocating and binding device memory for an image
    //! \param img    the image to allocate memory for
    //! \param props  requred memory properties
    //! \return the device memory that has been bound to the image
    VkDeviceMemory _allocImageMemory (VkImage img, VkMemoryPropertyFlags props);

    //! \brief A helper function for creating a Vulkan image view object for an image
    VkImageView _createImageView (
        VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    //! \brief A helper function for changing the layout of an image
    void _transitionImageLayout (
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout);

    //! \brief create a VkBuffer object
    //! \param size   the size of the buffer in bytes
    //! \param usage  the usage of the buffer
    //! \return the allocated buffer
    VkBuffer _createBuffer (size_t size, VkBufferUsageFlags usage);

    //! \brief A helper function for allocating and binding device memory for a buffer
    //! \param buf    the buffer to allocate memory for
    //! \param props  requred memory properties
    //! \return the device memory that has been bound to the buffer
    VkDeviceMemory _allocBufferMemory (VkBuffer buf, VkMemoryPropertyFlags props);

    //! \brief copy data from one buffer to another using the GPU
    //! \param dstBuf the destination buffer
    //! \param srcBuf the source buffer
    //! \param offset the offset in the destination buffer to copy to
    //! \param size   the size (in bytes) of data to copy
    void _copyBuffer (VkBuffer dstBuf, VkBuffer srcBuf, size_t offset, size_t sz);

    //! \brief copy data from a buffer to an image
    //! \param dstImg the destination image
    //! \param srcBuf the source buffer
    //! \param size   the size (in bytes) of data to copy
    //! \param wid    the image width
    //! \param ht     the image height (default 1)
    //! \param depth  the image depth (default 1)
    void _copyBufferToImage (
        VkImage dstImg, VkBuffer srcBuf, size_t size,
        uint32_t wid, uint32_t ht=1, uint32_t depth=1);

};

} // namespace cs237

#endif // !_CS237_APPLICATION_HPP_
