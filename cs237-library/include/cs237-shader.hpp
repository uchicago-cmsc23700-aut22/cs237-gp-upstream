/*! \file cs237-shader.hpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_SHADER_HPP_
#define _CS237_SHADER_HPP_

#ifndef _CS237_HPP_
#error "cs237-shader.hpp should not be included directly"
#endif

namespace cs237 {

enum class ShaderKind {
    Vertex = 0, Geometry, TessControl, TessEval, Fragment, Compute
};

//! A wrapper class for loading a pipeline of pre-compiled shaders from
//! the file system.
//
class Shaders {
  public:

    /*! \brief load a pre-compiled shader program from the file system.
     *  \param device   the logical device that will run the shaders
     *  \param stem     the base name of the shader files
     *  \param stages   a vector spcifying the shader stages that form the program.
     *                  These should be in the standard order.
     *
     * This version of the constructor assumes that the shader programs all
     * have the same base filename and only differ in their file extensions.
     */
    Shaders (
        VkDevice device,
        std::string const &stem,
        std::vector<ShaderKind> const &stages);

    /* \brief load a pre-compiled shader program from the file system.
     *  \param device   the logical device that will run the shaders
     *  \param files    a vector of the shader file names
     *  \param stages   a vector spcifying the shader stages that form the program.
     *                  These should be in the standard order and have the same
     *                  number as the vector of files.
     */
    Shaders (
        VkDevice device,
        std::vector<std::string> const &files,
        std::vector<ShaderKind> const &stages);

    ~Shaders ();

    //! return the number of shader stages in the pipeline
    int numStages () const { return this->_stages.size(); }

    //! return a pointer to the array of stage create infos
    VkPipelineShaderStageCreateInfo *stages ()
    {
        return this->_stages.data();
    }

  private:
    VkDevice _device;
    std::vector<VkPipelineShaderStageCreateInfo> _stages;

}; // Shaders

} /* namespace cs237 */

#endif /* !_CS237_SHADER_HPP_ */
