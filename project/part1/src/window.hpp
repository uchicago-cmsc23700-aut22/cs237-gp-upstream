/*! \file window.hpp
 *
 * CS23700 Autumn 2022 Sample Code for Group Project
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _WINDOW_HPP_
#define _WINDOW_HPP_

#include "cs237.hpp"
#include "map.hpp"
#include "app.hpp"
#include "camera.hpp"

//! the different rendering modes
constexpr int kWireframe = 0;           //!< wireframe mode
constexpr int kFlat = 1;                //!< flat shading mode
constexpr int kDiffuse = 2;             //!< diffuse lighting
constexpr int kTextured = 3;            //!< textured shading
constexpr int kNumModes = 4;

//! The Window class
class Window : public cs237::Window {
public:
    Window (Project *app, cs237::CreateWindowInfo const &info, Map *map);

    ~Window () override;

    void draw () override;

    //! method to handle rendering and presentation of the view
    //! \param dt time since last frame
    void render (float dt);

    //! animation call-back method
    //! \param now time of current frame
    void animate (double now);

    //! handle keyboard events
    void key (int key, int scancode, int action, int mods) override;

  //! is the view in wireframe mode?
    bool wireframeMode () const { return this->_wireframe; }

  //! is fog enabled?
    bool fogEnabled () const { return this->_map->hasFog() && this->_fogEnabled; }

  //! toggle the state of fog; returns true if a redraw is required
    bool toggleFog ()
    {
        if (this->_map->hasFog()) {
            this->_fogEnabled = !this->_fogEnabled;
            return true;
        }
        else {
            return false;
        }
    }

  //! the window's current camera state
    class Camera const &camera () const { return this->_cam; }

  //! the window's current error limit
    float errorLimit () const { return this->_errorLimit; }

  //! the cache of VAO objects for representing chunks
    class BufferCache *vaoCache () const { return this->_bCache; }

  //! the cache of textures for the map tiles
    class TextureCache *txtCache () const { return this->_tCache; }

private:
    Map *_map;                          //!< the map being rendered
    Camera _cam;                        //!< tracks viewer position, etc.
    float _errorLimit;                  //!< screen-space error limit
    int _fbWid;                         //!< current framebuffer width
    int _fbHt;                          //!< current framebuffer height
    bool _wireframe;                    //!< true if we are rendering the wireframe
    bool _fogEnabled;                   //!< true when fog is enabled (implies map->hasFog())
    double _lastStep;                   //!< time of last animation step
    cs237::AABBd _mapBBox;              //!< a bounding box around the entire map

    // resource management
    class BufferCache *_bCache;         //!< cache of OpenGL VAO objects used for chunks
    class TextureCache *_tCache;        //!< cache of OpenGL textures

    VkRenderPass _renderPass;                   //!< the render pass for drawing
    std::vector<VkFramebuffer> _framebuffers;   //!< the framebuffers
    VkCommandBuffer _cmdBuffer;                 //!< the command buffer
    SyncObjs _syncObjs;                         //!< synchronization objects for the
                                                //!  swap chain

    /* ADDITIONAL STATE HERE */

    /** HINT: you will need to define any initialization function
     ** that you use to initialize the rendering structures.
     */

};

#endif // !_WINDOW_HPP_
