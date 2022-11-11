/*! \file cs237-aabb.hpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * This file implements axis-aligned bounding boxes.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_AABB_HPP_
#define _CS237_AABB_HPP_

#ifndef _CS237_HPP_
#  error "cs237-aabb.hpp should not be included directly"
#endif

namespace cs237 {

namespace __detail {

//! Axis-Aligned Bounding Box
template <typename T>
struct AABB {
    using vec3 = glm::vec<3, T, glm::defaultp>;

    vec3 _min, _max;
    bool _empty;

    AABB () : _min(), _max(), _empty(true) {}
    AABB (AABB const & bbox) : _min(bbox._min), _max(bbox._max), _empty(bbox._empty) {}
    AABB (vec3 const &pt) : _min(pt), _max(pt), _empty(false) {}
    AABB (vec3 const &min, vec3 const &max) : _min(min), _max(max), _empty(false) {}

  //! is the box empty
    bool isEmpty () const { return this->_empty; }

  //! clear the box (i.e., make it empty)
    void clear () { this->_empty = true; }

  //! is a point inside the box?
    bool includesPt (vec3 const &pt) const
    {
        return (! this->_empty)
            && (this->_min.x <= pt.x) && (pt.x <= this->_max.x)
            && (this->_min.y <= pt.y) && (pt.y <= this->_max.y)
            && (this->_min.z <= pt.z) && (pt.z <= this->_max.z);
    }

  //! distance to a point; will be 0.0 if the point is inside the box
    T distanceToPt (vec3 const &pt) const
    {
#define DELTA(A)							\
        (pt.A < this->_min.A)						\
            ? (this->_min.A - pt.A)					\
            : ((this->_max.A < pt.A) ? (pt.A - this->_max.A) : T(0))

        vec3 dv(DELTA(x), DELTA(y), DELTA(z));
        T lenSq = dot(dv, dv);
        if (lenSq == 0.0)
            return lenSq;
        else
            return sqrt(lenSq);
#undef DELTA
    }

  //! extend this box as necessary to include the point
    void addPt (vec3 const &pt)
    {
        if (this->_empty) {
            this->_empty = false;
            this->_min = pt;
            this->_max = pt;
        }
        else if (pt.x < this->_min.x) this->_min.x = pt.x;
        else if (this->_max.x < pt.x) this->_max.x = pt.x;
        if (pt.y < this->_min.y) this->_min.y = pt.y;
        else if (this->_max.y < pt.y) this->_max.y = pt.y;
        if (pt.z < this->_min.z) this->_min.z = pt.z;
        else if (this->_max.z < pt.z) this->_max.z = pt.z;
    }

  //! merge the bounding box into this bounding box
  //! \param bb the bounding box to merge into this box
    AABB & operator+= (AABB const &bb)
    {
        if ((! this->_empty) && (! bb._empty)) {
            this->_min.x = MIN(this->_min.x, bb._min.x);
            this->_min.y = MIN(this->_min.y, bb._min.y);
            this->_min.z = MIN(this->_min.z, bb._min.z);
            this->_max.x = MAX(this->_max.x, bb._max.x);
            this->_max.y = MAX(this->_max.y, bb._max.y);
            this->_max.z = MAX(this->_max.z, bb._max.z);
        }
        else if (this->_empty) {
            this->_empty = false;
            this->_min = bb._min;
            this->_max = bb._max;
        }

        return *this;
    }

  //! extend this box as necessary to include the point
  //! \param pt the point to include in this box
    AABB & operator+= (vec3 const &pt)
    {
        this->addPt (pt);
        return *this;
    }

  /***** Warning: the following functions are undefined on empty boxes! *****/

  //! minimum extents of the box
    vec3 const & min() const
    {
        assert (! this->_empty);
        return this->_min;
    }

  //! maximum extents of the box
    vec3 const & max() const
    {
        assert (! this->_empty);
        return this->_max;
    }

  //! center of the box
    vec3 center () const
    {
        assert (! this->_empty);
        return T(0.5) * (this->_min + this->_max);
    }

  //! minimum X-coordinate of the box
    T minX () const
    {
        assert (! this->_empty);
        return this->_min.x;
    }
  //! minimum Y-coordinate of the box
    T minY () const
    {
        assert (! this->_empty);
        return this->_min.y;
    }
  //! minimum Z-coordinate of the box
    T minZ () const
    {
        assert (! this->_empty);
        return this->_min.z;
    }
  //! maximum X-coordinate of the box
    T maxX () const
    {
        assert (! this->_empty);
        return this->_max.x;
    }
  //! maximum Y-coordinate of the box
    T maxY () const
    {
        assert (! this->_empty);
        return this->_max.y;
    }
  //! maximum Z-coordinate of the box
    T maxZ () const
    {
        assert (! this->_empty);
        return this->_max.z;
    }

  //! return the coordinates of the i'th corner of the box, where
  //! _min is corner 0 and _max is corner 7.
    vec3 corner (int i) const
    {
        assert (! this->_empty);
        assert ((0 <= i) && (i < 8));
        return vec3(
            (i & 4) ? this->_max.x : this->_min.x,
            (i & 2) ? this->_max.y : this->_min.y,
            (i & 1) ? this->_max.z : this->_min.z);
    }
};

//! print the axis-aligned bounding box to the output stream
template <typename T>
std::ostream& operator<< (std::ostream& s, __detail::AABB<T> const &bb);

//! compute the union of two axis-aligned bounding boxes
//! \param bb1 an axis-aligned bounding box
//! \param bb2 an axis-aligned bounding box
//! \return the smallest AABB that contains both \arg bb1 and \arg bb2.
template <typename T>
inline AABB<T> operator+ (AABB<T> const &bb1, AABB<T> &bb2)
{
    if ((! bb1._empty) && (! bb2.empty)) {
        return AABB<T>(glm::min(bb1._min, bb2._min), glm::max(bb1._min, bb2._min));
    }
    else if (bb1.empty) {
        return bb2;
    } else {
        return bb1;
    }
}

} // namespace __detail

//! Single-precision axis-aligned bounding boxes
typedef __detail::AABB<float> AABBf;
//! Double-precision axis-aligned bounding boxes
typedef __detail::AABB<double> AABBd;

} //namespace cs237

#endif /* !_CS237_AABB_HPP_ */
