/*! \file vao.cpp
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "vao.hpp"

VAO::VAO (cs237::Application *app, struct Chunk const &chunk)
  : _vBuf(new cs237::VertexBuffer(app, chunk.vSize(), chunk.vertices)),
    _iBuf(new cs237::IndexBuffer(app, chunk.nIndices, VK_INDEX_TYPE_UINT16, chunk.indices))
{ }

VAO::~VAO ()
{
    delete this->_vBuf;
    delete this->_iBuf;
}
