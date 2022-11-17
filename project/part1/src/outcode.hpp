/*! \file outcode.hpp
 *
 * A header-only implementation of outcodes for testing against
 * a view frustum.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _OUTCODE_HPP_
#define _OUTCODE_HPP_

#include "cs237.hpp"

struct Outcode {
    bool _culled;       //!< true when the tested object is completely outside the frustum
    uint8_t _planes;    //!< one bit per frustum plane; set to 0 when the tested object is
                        //! completely inside the plane and set to 1 when the tested object
                        //! is at least partially outside the halfspace defined by the plane.

    //! create an initial outcode
    Outcode (bool c = false, uint8_t p = 0x3f) : _culled(c), _planes(p) { }

    //! is the object culled (i.e., completely outside at least one plane)?
    bool culled () const { return this->_culled; }

    //! is the object completely inside the planes?
    bool allIn () const { return (this->_planes == 0); }

    //! is the object completely inside the given plane?
    bool notCulledBy (int p) const { return ((this->_planes & (1 << p)) == 0); }

    //! clear the bit for the plane
    void clearPlane (int p) { this->_planes &= ~(1 << p); }

};

/***** Output *****/

inline std::string to_string (Outcode const &code)
{
    if (code.culled()) {
	return "<culled>";
    } else {
        char buf[] = "<000000>";
	for (int bit = 0x20, i = 1;  bit > 0;  bit >>= 1, ++i) {
	    if ((code._planes & bit) != 0) {
		buf[i] = '1';
	    }
	}
	return std::string(buf);
    }
}

inline std::ostream& operator<< (std::ostream& s, Outcode const &code)
{
    s << to_string(code);
    return s;
}

#endif //! _OUTCODE_HPP_
