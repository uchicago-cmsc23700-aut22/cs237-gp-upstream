/*! \file shader.cpp
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
#include <fstream>

namespace cs237 {

// read the contents of a pre-compiled shader file
static std::vector<char> _readFile (std::string const &name)
{
    std::ifstream file(name, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        ERROR("unable to open shader file!");
    }

    // determine the file size; note that the current position is at the *end* of the file.
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    // move back to the beginning and read the contents
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;

}

struct Stage {
    Stage (VkDevice device, std::string const &file, ShaderKind k);

    VkPipelineShaderStageCreateInfo StageInfo ();

    ShaderKind kind;
    VkShaderModule module;
};

static struct {
    std::string suffix;
    VkShaderStageFlagBits bit;
} _stageInfo[] = {
    { ".vert.spv", VK_SHADER_STAGE_VERTEX_BIT },
    { ".geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT },
    { ".tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT },
    { ".tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT },
    { ".frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT },
    { ".comp.spv", VK_SHADER_STAGE_COMPUTE_BIT },
};

Stage::Stage (VkDevice dev, std::string const &name, ShaderKind k)
    : kind(k)
{
    // get the code for the shader
    auto code = _readFile (name);

    // create the shader module
    VkShaderModuleCreateInfo moduleInfo;
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.pNext = nullptr;
    moduleInfo.flags = 0;
    moduleInfo.codeSize = code.size();
    moduleInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shModule;
    if (vkCreateShaderModule(dev, &moduleInfo, nullptr, &this->module) != VK_SUCCESS) {
        ERROR("unable to create shader module!");
    }

}

VkPipelineShaderStageCreateInfo Stage::StageInfo ()
{
    // info to specify the pipeline stage
    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.pNext = nullptr;
    stageInfo.flags = 0;
    stageInfo.stage = _stageInfo[static_cast<int>(this->kind)].bit;
    stageInfo.module = this->module;
    stageInfo.pName = "main";
    stageInfo.pSpecializationInfo = nullptr;

    return stageInfo;
}

Shaders::Shaders (
    VkDevice device,
    std::string const &stem,
    std::vector<ShaderKind> const &stages)
  : _device(device)
{
    std::vector<Stage> stageVec;
    stageVec.reserve(stages.size());
    for (auto k : stages) {
        std::string name = stem + _stageInfo[static_cast<int>(k)].suffix;
        stageVec.push_back(Stage(device, name, k));
    }

    this->_stages.reserve(stageVec.size());
    for (auto stage : stageVec) {
        this->_stages.push_back(stage.StageInfo());
    }

}

Shaders::Shaders (
    VkDevice device,
    std::vector<std::string> const &files,
    std::vector<ShaderKind> const &stages)
  : _device(device)
{
    if (files.size() != stages.size()) {
        ERROR("mismatch in number of files/stages");
    }

    std::vector<Stage> stageVec;
    stageVec.reserve(stages.size());
    for (int i = 0;  i < stages.size();  ++i) {
        stageVec.push_back(Stage(device, files[i], stages[i]));
    }

    this->_stages.reserve(stageVec.size());
    for (auto stage : stageVec) {
        this->_stages.push_back(stage.StageInfo());
    }

}

Shaders::~Shaders ()
{
    for (auto stage : this->_stages) {
        vkDestroyShaderModule (this->_device, stage.module, nullptr);
    }
}

} // namespace cs237
