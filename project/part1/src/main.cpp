/*! \file main.cpp
 *
 * CS23700 Autumn 2022 Sample Code for Group Project
 *
 * This file contains the main program.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "app.hpp"

int main(int argc, char *argv[])
{
    std::vector<const char *> args(argv, argv+argc);
    Project app(args);

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
