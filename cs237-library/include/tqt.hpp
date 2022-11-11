/*! \file tqt.hpp
 *
 * Support for texture quadtrees.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _TQT_HPP_
#define _TQT_HPP_

#include "cs237.hpp"
#include <vector>

namespace tqt {

  //! Manages a disk-based texture quadtree and supports loading individual
  //! texture images at different levels and locations in the tree.
    class TextureQTree {
      public:

        TextureQTree (const char* filename);
        ~TextureQTree();

      //! is this a valid TQT?
        bool isValid () const { return this->_source != nullptr; }
      //! the depth of the TQT
        int depth() const { return this->_depth; }
      //! the size of a texture tile measured in pixels (tiles are always square)
        int tileSize() const { return this->_tileSize; }

      //! return the image tile at the specified quadtree node.
      //! \param[in] level the level of the node in the tree (root = 0)
      //! \param[in] row the row of the node on its level (north == 0)
      //! \param[in] col the column of the node on its level (west == 0)
      //! \param[in] flip should the image be flipped to match OpenGL (default true)
      //! \return a pointer to the image; nullptr is returned if there is
      //!         an error.  It is the caller's responsibility to manage the
      //!         image's storage.
        cs237::Image2D *loadImage (int level, int row, int col, bool flip = true);

      //! return true if the file looks like a TQT file of the right version
        static bool isTQTFile (std::string const &filename);

      private:
        std::vector<std::streamoff> _toc;       //!< stream offsets for images
        int _depth;                             //!< the depth of the TQT
        int _tileSize;                          //!< the size of a texture tile in pixels
        std::ifstream *_source;                 //!< the source file for the textures

    };  // class TextureQTree

} // namespace tqt

#endif // !_TQT_HPP_
