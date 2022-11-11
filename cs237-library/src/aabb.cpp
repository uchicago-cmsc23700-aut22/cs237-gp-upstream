/*! \file aabb.cpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * Operations on AABB values.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hpp"

namespace cs237 {

    namespace __detail {
      //! \brief output the bounding box to a stream
      //! \param s the stream to print to
      //! \param bb the box to print
#define PROTO(T) std::ostream& operator<< (std::ostream& s, AABB<T> const &bb)
	template <typename T> PROTO(T)
	{
	    if (bb._empty) {
		return (s << "<empty>");
	    }
	    else {
		return (s << "<" << glm::to_string(bb.min())
                    << ", " << glm::to_string(bb.max()) << ">");
	    }
	}
	template PROTO(float);
	template PROTO(double);
#undef PROTO

    } // namespace __detail

} // namespace cs237
