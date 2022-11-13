/*! \file app.hpp
 *
 * CS23700 Autumn 2022 Sample Code for Group Project
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _APP_HPP_
#define _APP_HPP_

#include "cs237.hpp"
#include "map.hpp"

//! The main Application class
class Project : public cs237::Application {
public:
    Project (std::vector<const char *> &args);
    ~Project ();

    //! run the application
    void run () override;

protected:
    Map _map;           //!< holds the map to be rendered

};

#endif // !_APP_HPP_

