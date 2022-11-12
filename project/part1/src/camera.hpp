/*! \file camera.hpp
 *
 * \author John Reppy
 *
 * The camera class encapsulates the current view and projection matrices.
 */

/* CMSC23700 Group Project sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CAMERA_HPP_
#define _CAMERA_HPP_

#include "cs237.hpp"

//! The camera class encapsulates the current view and projection matrices.
//! Note that we track the camera's position using double-precision so that
//! we can support large worlds.
//
class Camera {
  public:

  //! constuctor for uninitialized camera
    Camera ();

  //! the current camera position.
    glm::dvec3 position () const { return this->_pos; }
  //! the direction that the camera it looking at.
    glm::vec3 direction () const { return this->_dir; }
  //! the camera's current up vector
    glm::vec3 up () const { return this->_up; }

  //! the distance to the near plane of the view frustum
    float near () const { return this->_nearZ; }
  //! the distance to the far plane of the view frustum
    float far () const { return this->_farZ; }
  //! the aspect ratio of the viewport (height/width)
    float aspect () const { return this->_aspect; }
  //! the horizontal field-of-view of the view frustum
    float fov () const { return glm::degrees(2.0 * this->_halfFOV); }

  //! translate a point to the coordinate system that has the camera as the origin, but is
  //! oriented and scaled the same as the world coordinates.
    glm::dvec3 translate (glm::dvec3 const &p) const;

  //! a transformation matrix that assumes that the camera is at the origin.
    glm::mat4 viewTransform () const;

  //! the projection transform for the camera
    glm::mat4 projTransform () const;

  //! update the camera for the aspect ratio of the given viewport.  This operation changes
  //! the aspect ratio, but not the field of view.
    void setViewport (int wid, int ht);

  //! set the horizontal field of view in degrees
    void setFOV (float angle);

  //! set the near and far planes
    void setNearFar (double nearZ, double farZ);

  //! move the camera to a new position while maintaining its heading
    void move (glm::dvec3 const &pos);

  //! move the camera to a new position and heading
    void move (glm::dvec3 const &pos, glm::dvec3 const &at);

  //! move the camera to a new position, heading, and up vector
    void move (glm::dvec3 const &pos, glm::dvec3 const &at, glm::dvec3 const &up);

  //! change the direction of the camera
    void look (glm::vec3 const &dir);

  //! change the direction of the camera
    void look (glm::vec3 const &dir, glm::vec3 const &up);

  //! compute the screen-space error for a geometric error
  //! \param dist the distance to the geometric error
  //! \param the geometric error
  //! \return the screen-space error
    float screenError (float dist, float err) const;

  private:
    glm::dvec3 _pos;            //!< position is double precision to allow large worlds
    glm::vec3 _dir;             //!< the current direction that the camera is pointing towards
    glm::vec3 _up;              //! camera up vector
    float _nearZ;               //!< distance to the near plane
    float _farZ;                //!< distance to the far plane
    float _aspect;              //! the aspect ratio of the viewport (height / width)
    float _halfFOV;             //!< horizontal field of view / 2 (in radians)
    mutable float _errorFactor; //!< viewport width/(2 * tan(_halfFOV)); set to -1 when invalid
    int _wid;                   //!< the width of the viewport

};

/***** Output *****/

//! output the state of the camera
std::ostream& operator<< (std::ostream& s, Camera const &cam);

#endif // !_CAMERA_HPP_
