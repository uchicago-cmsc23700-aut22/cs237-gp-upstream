/*! \file plane.hxx
 *
 * Representation of a plane in 3D space.
 *
 * \author John Reppy
 */

/* COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_PLANE_HPP_
#define _CS237_PLANE_HPP_

#ifndef _CS237_HPP_
#error "cs237-plane.hpp should not be included directly"
#endif

namespace cs237 {

//! representation of a oriented 3D plane
struct Plane {
    glm::dvec4 _nd;     //! unit normal and signed distance from origin

    Plane () { }

    //! \brief specify a plane as a unit-length normal vector and
    //!        signed distance from origin (single precision)
    //! \param n unit-length plane normal vector
    //! \param d signed distance from origin to the plane
    Plane (glm::vec3 n, float d) : _nd(glm::dvec4(glm::dvec3(n), double(d))) { }

    //! \brief specify a plane as a unit-length normal vector and
    //!        signed distance from origin (double precision)
    //! \param n unit-length plane-normal vector
    //! \param d signed distance from origin to the plane
    Plane (glm::dvec3 n, double d) : _nd(glm::dvec4(n, d)) { }

    //! \brief specify a plane as a normal vector and point on the plane (single precision)
    //! \param n plane-normal vector (does not have to be unit length)
    //! \param p a point on the plane
    Plane (glm::vec3 n, glm::vec3 p)
    {
        glm::dvec3 norm = glm::normalize(glm::dvec3(n));
        double d = -glm::dot(norm, glm::dvec3(p));
        this->_nd = glm::dvec4(norm, d);
    }

    //! \brief specify a plane as a normal vector and point on the plane (double precision)
    //! \param n plane-normal vector (does not have to be unit length)
    //! \param p a point on the plane
    Plane (glm::dvec3 n, glm::dvec3 p)
    {
        glm::dvec3 norm = glm::normalize(n);
        double d = -glm::dot(norm, p);
        this->_nd = glm::dvec4(norm, d);
    }

    //! \brief get the plane normal vector (double precision)
    glm::dvec3 normd () const { return glm::dvec3(this->_nd); }

    //! \brief get the plane normal vector (single precision)
    glm::vec3 normf () const { return glm::vec3(this->normd()); }

    //! \brief signed distance from origin to plane (double precision)
    double distd () const { return this->_nd.w; }

    //! \brief signed distance from origin to plane (single precision)
    float distf () const { return float(this->_nd.w); }

    //! \brief signed distance from a point to plane
    //! \param p a point in space
    //! \return the signed distance from `p` to the plane
    double distanceToPt (glm::dvec3 const &p) const
    {
	return glm::dot(this->_nd, glm::dvec4(p, 1.0));
    }
};

/***** Output *****/

inline std::ostream& operator<< (std::ostream& s, Plane const &plane)
{
    return (s << "Plane(" << glm::to_string(glm::dvec3(plane._nd)) << ", " << plane._nd.w << ")");
}

} // namespace cs237

#endif // !_CS237_PLANE_HPP_
