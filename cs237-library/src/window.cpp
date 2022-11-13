/*! \file window.cpp
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

/******************** local helper functions ********************/

// wrapper function for Refresh callback
static void refreshCB (GLFWwindow *win)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->refresh ();
}

// wrapper function for Reshape callback
static void reshapeCB (GLFWwindow *win, int wid, int ht)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->reshape (wid, ht);
}

// wrapper function for Iconify callback
static void iconifyCB (GLFWwindow *win, int iconified)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->iconify (iconified != 0);
}

// wrapper function for Key callback
static void keyCB (GLFWwindow *win, int key, int scancode, int action, int mods)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->key (key, scancode, action, mods);
}

// wrapper function for CursorPos callback
static void cursorPosCB (GLFWwindow *win, double xpos, double ypos)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->cursorPos (xpos, ypos);
}

// wrapper function for CursorEnter callback
static void cursorEnterCB (GLFWwindow *win, int entered)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->cursorEnter (entered == GLFW_TRUE);
}

// wrapper function for MouseButton callback
static void mouseButtonCB (GLFWwindow *win, int button, int action, int mods)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->mouseButton (button, action, mods);
}

// wrapper function for Scroll callback
static void scrollCB (GLFWwindow *win, double xoffset, double yoffset)
{
    auto winObj = static_cast<Window *>(glfwGetWindowUserPointer (win));
    winObj->scroll (xoffset, yoffset);
}

/******************** class Window methods ********************/

Window::Window (Application *app, CreateWindowInfo const &info)
    : _app(app), _win(nullptr), _swap(app->_device)
{
    glfwWindowHint(GLFW_RESIZABLE, info.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(
        info.wid, info.ht,
        info.title.c_str(),
        nullptr, nullptr);
    if (window == nullptr) {
        ERROR("unable to create window!");
    }

    // set the user data for the window to this object
    glfwSetWindowUserPointer (window, this);

    // set up window-system callbacks
    glfwSetWindowRefreshCallback (window, refreshCB);
    if (info.resizable) {
        glfwSetWindowSizeCallback (window, reshapeCB);
    }
    glfwSetWindowIconifyCallback (window, iconifyCB);

    this->_win = window;
    this->_wid = info.wid;
    this->_ht = info.ht;
    this->_isVis = true;
    this->_keyEnabled = false;
    this->_cursorPosEnabled = false;
    this->_cursorEnterEnabled = false;
    this->_mouseButtonEnabled = false;
    this->_scrollEnabled = false;

    // set up the Vulkan surface for the window
    if (glfwCreateWindowSurface(app->_instance, window, nullptr, &this->_surf) != VK_SUCCESS) {
        ERROR("unable to create window surface!");
    }

    // set up the swap chain for the surface
    this->_createSwapChain (info.depth, info.stencil);

}

Window::~Window ()
{
    // destroy the swap chain as associated state
    this->_swap.cleanup ();

    // delete the surface
    vkDestroySurfaceKHR (this->_app->_instance, this->_surf, nullptr);

    glfwDestroyWindow (this->_win);
}

void Window::reshape (int wid, int ht)
{
    this->_wid = wid;
    this->_ht = ht;
}

void Window::iconify (bool iconified)
{
    this->_isVis = !iconified;
}

/* default event handlers just ignore the events */
void Window::key (int key, int scancode, int action, int mods) { }
void Window::cursorPos (double xpos, double ypos) { }
void Window::cursorEnter (bool entered) { }
void Window::mouseButton (int button, int action, int mods) { }
void Window::scroll (double xoffset, double yoffset) { }

void Window::enableKeyEvent (bool enable)
{
    if (this->_keyEnabled && (! enable)) {
        // disable the callback
        this->_keyEnabled = false;
        glfwSetKeyCallback(this->_win, nullptr);
    }
    else if ((! this->_keyEnabled) && enable) {
        // enable the callback
        this->_keyEnabled = true;
        glfwSetKeyCallback(this->_win, keyCB);
    }

}

void Window::setCursorMode (int mode)
{
    glfwSetInputMode (this->_win, GLFW_CURSOR, mode);
}

void Window::enableCursorPosEvent (bool enable)
{
    if (this->_cursorPosEnabled && (! enable)) {
        // disable the callback
        this->_cursorPosEnabled = false;
        glfwSetCursorPosCallback(this->_win, nullptr);
    }
    else if ((! this->_cursorPosEnabled) && enable) {
        // enable the callback
        this->_cursorPosEnabled = true;
        glfwSetCursorPosCallback(this->_win, cursorPosCB);
    }

}

void Window::enableCursorEnterEvent (bool enable)
{
    if (this->_cursorEnterEnabled && (! enable)) {
        // disable the callback
        this->_cursorEnterEnabled = false;
        glfwSetCursorEnterCallback(this->_win, nullptr);
    }
    else if ((! this->_cursorEnterEnabled) && enable) {
        // enable the callback
        this->_cursorEnterEnabled = true;
        glfwSetCursorEnterCallback(this->_win, cursorEnterCB);
    }

}

void Window::enableMouseButtonEvent (bool enable)
{
    if (this->_mouseButtonEnabled && (! enable)) {
        // disable the callback
        this->_mouseButtonEnabled = false;
        glfwSetMouseButtonCallback(this->_win, nullptr);
    }
    else if ((! this->_mouseButtonEnabled) && enable) {
        // enable the callback
        this->_mouseButtonEnabled = true;
        glfwSetMouseButtonCallback(this->_win, mouseButtonCB);
    }

}

void Window::enableScrollEvent (bool enable)
{
    if (this->_scrollEnabled && (! enable)) {
        // disable the callback
        this->_scrollEnabled = false;
        glfwSetScrollCallback(this->_win, nullptr);
    }
    else if ((! this->_scrollEnabled) && enable) {
        // enable the callback
        this->_scrollEnabled = true;
        glfwSetScrollCallback(this->_win, scrollCB);
    }

}

Window::SwapChainDetails Window::_getSwapChainDetails ()
{
    auto dev = this->_app->_gpu;
    auto surf = this->_surf;
    Window::SwapChainDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surf, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            dev, surf,
            &presentModeCount, details.presentModes.data());
    }

    return details;

}

void Window::_createSwapChain (bool depth, bool stencil)
{
    // determine the required depth/stencil-buffer format
    VkFormat dsFormat = this->_app->_depthStencilBufferFormat(depth, stencil);
    if ((dsFormat == VK_FORMAT_UNDEFINED) && (depth || stencil)) {
        ERROR("depth/stencil buffer requested but not supported by device");
    }
    this->_swap.numAttachments = (dsFormat == VK_FORMAT_UNDEFINED) ? 1 : 2;

    SwapChainDetails swapChainSupport = this->_getSwapChainDetails ();

    // choose the best aspects of the swap chain
    VkSurfaceFormatKHR surfaceFormat = swapChainSupport.chooseSurfaceFormat();
    VkPresentModeKHR presentMode = swapChainSupport.choosePresentMode();
    VkExtent2D extent = swapChainSupport.chooseExtent(this->_win);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if ((swapChainSupport.capabilities.maxImageCount > 0)
    && (imageCount > swapChainSupport.capabilities.maxImageCount)) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapInfo{};
    swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.surface = this->_surf;

    swapInfo.minImageCount = imageCount;
    swapInfo.imageFormat = surfaceFormat.format;
    swapInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapInfo.imageExtent = extent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto qIdxs = this->_app->_qIdxs;
    uint32_t qIndices[] = {qIdxs.graphics, qIdxs.present};

    // check if the graphics and presentation queues are distinct
    if (qIdxs.graphics != qIdxs.present) {
        swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapInfo.queueFamilyIndexCount = 2;
        swapInfo.pQueueFamilyIndices = qIndices;
    }
    else {
        swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = presentMode;
    swapInfo.clipped = VK_TRUE;

    swapInfo.oldSwapchain = VK_NULL_HANDLE;

    VkDevice dev = this->device();
    auto sts = vkCreateSwapchainKHR(dev, &swapInfo, nullptr, &this->_swap.chain);
    if (sts != VK_SUCCESS) {
        ERROR("unable to create swap chain!");
    }

    // get the vector of images that represent the swap chain
    vkGetSwapchainImagesKHR(dev, this->_swap.chain, &imageCount, nullptr);
    this->_swap.images.resize(imageCount);
    vkGetSwapchainImagesKHR(
        dev, this->_swap.chain,
        &imageCount, this->_swap.images.data());

    this->_swap.imageFormat = surfaceFormat.format;
    this->_swap.extent = extent;

    // create an image view per swap-chain image
    this->_swap.views.resize(this->_swap.images.size());
    for (int i = 0; i < this->_swap.images.size(); ++i) {
        this->_swap.views[i] = this->_app->_createImageView(
            this->_swap.images[i],
            this->_swap.imageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    if (dsFormat != VK_FORMAT_UNDEFINED) {
        // initialize the depth/stencil-buffer
        DepthStencilBuffer dsBuf;
        dsBuf.format = dsFormat;
        dsBuf.image = this->_app->_createImage(
            extent.width, extent.height,
            dsFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            1);
        dsBuf.imageMem = this->_app->_allocImageMemory(
            dsBuf.image,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        dsBuf.view = this->_app->_createImageView (
            dsBuf.image,
            dsFormat,
            VK_IMAGE_ASPECT_DEPTH_BIT);
        this->_swap.dsBuf = dsBuf;
    }
}

void Window::_initAttachments (
    std::vector<VkAttachmentDescription> &descs,
    std::vector<VkAttachmentReference> &refs)
{
    descs.resize(this->_swap.numAttachments);
    refs.resize(this->_swap.numAttachments);

    // the output color buffer
    descs[0].format = this->_swap.imageFormat;
    descs[0].samples = VK_SAMPLE_COUNT_1_BIT;
    descs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    descs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    descs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    descs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    descs[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    refs[0].attachment = 0;
    refs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if (this->_swap.dsBuf.has_value()) {
        descs[1].format = this->_swap.dsBuf->format;
        descs[1].samples = VK_SAMPLE_COUNT_1_BIT;
        descs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        descs[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
/* FIXME: if we need stencil support, then the following is incorrect! */
        descs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        descs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        descs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        descs[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        refs[1].attachment = 1;
        refs[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
}

void Window::_setViewportCmd (VkCommandBuffer cmdBuf,
    int32_t x, int32_t y,
    int32_t wid, int32_t ht)
{
    VkViewport viewport{};
    viewport.x = float(x);
    viewport.y = float(y);
    viewport.width = float(wid);
    viewport.height = float(ht);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {
            static_cast<uint32_t>(std::abs(wid)),
            static_cast<uint32_t>(std::abs(ht))
        };
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
}

void Window::_setViewportCmd (VkCommandBuffer cmdBuf)
{
    /* NOTE: we negate the height and set the Y origin to ht because Vulkan's
     * viewport coordinates are from top-left down, instead of from bottom-left up.
     * See https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport
     */
    this->_setViewportCmd(cmdBuf,
        0, this->_swap.extent.height,
        this->_swap.extent.width, -this->_swap.extent.height);
}


/******************** struct Window::SwapChainDetails methods ********************/

// choose the surface format for the buffers
VkSurfaceFormatKHR Window::SwapChainDetails::chooseSurfaceFormat ()
{
    for (const auto& fmt : this->formats) {
        if ((fmt.format == VK_FORMAT_B8G8R8A8_SRGB)
        && (fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
            return fmt;
        }
    }

    return this->formats[0];
}

// choose a presentation mode for the buffers
VkPresentModeKHR Window::SwapChainDetails::choosePresentMode ()
{
    for (const auto& mode : this->presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

//! compute the extent of the buffers
VkExtent2D Window::SwapChainDetails::chooseExtent (GLFWwindow *win)
{
    if (this->capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return this->capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(win, &width, &height);

        VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

        actualExtent.width = std::clamp(
            actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(
            actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actualExtent;
    }

}

/******************** struct Window::SwapChain methods ********************/

std::vector<VkFramebuffer> Window::SwapChain::framebuffers (VkRenderPass renderPass)
{
    assert (this->size() > 0);

    // the framebuffer attachments; currently we only have color, but we will add
    // a depth buffer
    VkImageView attachments[this->numAttachments];

    if (this->dsBuf.has_value()) {
        // include the depth buffer
        attachments[1] = this->dsBuf->view;
    }

    // initialize the invariant parts of the create info structure
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = renderPass;
    fbInfo.attachmentCount = this->numAttachments;
    fbInfo.pAttachments = attachments;
    fbInfo.width = this->extent.width;
    fbInfo.height = this->extent.height;
    fbInfo.layers = 1;

    // create a frambuffer per swap-chain image
    std::vector<VkFramebuffer> fbs(this->size());
    for (size_t i = 0; i < this->size(); i++) {
        attachments[0] = this->views[i];
        if (vkCreateFramebuffer(this->device, &fbInfo, nullptr, &fbs[i]) != VK_SUCCESS) {
            ERROR("unable to create framebuffer");
        }
    }

    return fbs;
}

void Window::SwapChain::cleanup ()
{
    for (auto view : this->views) {
        vkDestroyImageView(this->device, view, nullptr);
    }
    /* note that the images are owned by the swap chain object, so we do not have
     * to destroy them.
     */

    // cleanup the depth buffer (if present)
    if (this->dsBuf.has_value()) {
        vkDestroyImageView(this->device, this->dsBuf->view, nullptr);
        vkDestroyImage(this->device, this->dsBuf->image, nullptr);
        vkFreeMemory(this->device, this->dsBuf->imageMem, nullptr);
    }

    vkDestroySwapchainKHR(this->device, this->chain, nullptr);
}

/******************** struct Window::SyncObjs methods ********************/

void Window::SyncObjs::allocate ()
{
    // this is for backward compatibility; we used to do the allocation as a separate
    // call, the there is no reason for that.
    if (this->imageAvailable != VK_NULL_HANDLE) {
        return;
    }

    // allocate synchronization objects
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    auto device = this->win->device();
    if ((vkCreateSemaphore(device, &semInfo, nullptr, &this->imageAvailable) != VK_SUCCESS)
    ||  (vkCreateSemaphore(device, &semInfo, nullptr, &this->renderFinished) != VK_SUCCESS)
    ||  (vkCreateFence(device, &fenceInfo, nullptr, &this->inFlight) != VK_SUCCESS))
    {
        ERROR("unable to create synchronization objects");
    }
}

Window::SyncObjs::~SyncObjs ()
{
    assert (this->imageAvailable != VK_NULL_HANDLE);

    auto device = this->win->device();

    // delete synchronization objects
    vkDestroyFence(device, this->inFlight, nullptr);
    vkDestroySemaphore(device, this->imageAvailable, nullptr);
    vkDestroySemaphore(device, this->renderFinished, nullptr);

}

VkResult Window::SyncObjs::acquireNextImage (uint32_t &imageIndex)
{
    assert (this->inFlight != VK_NULL_HANDLE);

    vkWaitForFences(this->win->device(), 1, &this->inFlight, VK_TRUE, UINT64_MAX);

    auto sts = vkAcquireNextImageKHR(
        this->win->device(),
        this->win->_swap.chain,
        UINT64_MAX,
        this->imageAvailable,
        VK_NULL_HANDLE,
        &imageIndex);

    return sts;
}

//! reset the in-flight fence of this frame
void Window::SyncObjs::reset ()
{
    assert (this->inFlight != VK_NULL_HANDLE);

    vkResetFences(this->win->device(), 1, &this->inFlight);
}

//! submit a command buffer to a queue using this frame's synchronization objects
//! \param q        the queue to submit the commands to
//! \param cmdBuf   the command buffer to submit
void Window::SyncObjs::submitCommands (VkQueue q, VkCommandBuffer const &cmdBuf)
{
    assert (this->imageAvailable != VK_NULL_HANDLE);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSems[1] = { this->imageAvailable };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSems;
    VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    VkSemaphore signalSems[1] = { this->renderFinished };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSems;

    if (vkQueueSubmit(q, 1, &submitInfo, this->inFlight) != VK_SUCCESS) {
        ERROR("unable to submit draw command buffer!");
    }
}

//! \brief present the frame
//! \param q  the presentation queue
//! \return the return status of presenting the image
VkResult Window::SyncObjs::present (VkQueue q, const uint32_t *imageIndices)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore waitSems[1] = { this->renderFinished };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSems;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &this->win->_swap.chain;

    presentInfo.pImageIndices = imageIndices;

    auto sts = vkQueuePresentKHR(q, &presentInfo);

    return sts;
}

} // namespace cs237
