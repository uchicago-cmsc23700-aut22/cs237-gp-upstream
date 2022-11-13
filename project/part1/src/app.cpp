/*! \file app.cpp
 *
 * CS23700 Autumn 2022 Sample Code for Group Project
 *
 * The main application class for Project 2.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "app.hpp"
#include "window.hpp"
#include <cstdlib>
#include <unistd.h>

constexpr uint32_t kWindowWidth = 1024;
constexpr uint32_t kWindowHeight = 768;

static void usage (int sts)
{
    std::cerr << "usage: proj2 [options] <scene>\n";
    exit (sts);
}

Project::Project (std::vector<const char *> &args)
  : cs237::Application (args, "CS237 Group Project"), _map(this)
{
    // the last argument is the name of the map that we should render
    if (args.size() < 2) {
        usage(EXIT_FAILURE);
    }
    std::string mapName = args.back();

    // verify that the scene path exists
    if (access(mapName.c_str(), F_OK) < 0) {
        std::cerr << "map '" << mapName
            << "' is not accessible or does not exist\n";
        exit(EXIT_FAILURE);
    }

    // load the scene
    if (this->_map.load(mapName)) {
        std::cerr << "cannot load map from '" << mapName << "'\n";
        exit(EXIT_FAILURE);
    }
}

Project::~Project () { }

void Project::run ()
{
    // create the application window
    cs237::CreateWindowInfo cwInfo(
        kWindowWidth, kWindowHeight,
        this->_map.name(),
        false, true, false);
    Window *win = new Window (this, cwInfo, &this->_map);

    // we keep track of the time between frames for morphing and for
    // any time-based animation
    double lastFrameTime = glfwGetTime();

    // wait until the window is closed
    while(! win->windowShouldClose()) {
        // how long since the last frame?
        double now = glfwGetTime();
        float dt = float(now - lastFrameTime);
        lastFrameTime = now;

        /** HINT: Update camera if necessary */

        win->render (dt);

        // update animation state as necessary
        win->animate (now);

        glfwPollEvents();
    }

    // wait until any in-flight rendering is complete
    vkDeviceWaitIdle(this->_device);

    // cleanup
    delete win;
}
