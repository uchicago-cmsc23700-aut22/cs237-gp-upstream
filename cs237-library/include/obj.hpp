/*! \file obj.hxx
 *
 * \author John Reppy
 *
 * This file specifies data structures to represent a subset of OBJ files.  The format
 * is limited to triangle meshes.
 */

/*
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _OBJ_HXX_
#define _OBJ_HXX_

#include "cs237.hpp"

namespace OBJ {

//! Illumination modes define how to interpret the material values.
//! Note that other modes are mapped to Specular by the loader
enum {
    NoLight = 0,                //!< just use color component
    Diffuse = 1,                //!< ambient + diffuse
    Specular = 2                //!< include specular highlights
    /* 3	Reflection on and Ray trace on */
    /* 4	Transparency: Glass on
		Reflection: Ray trace on */
    /* 5	Reflection: Fresnel on and Ray trace on */
    /* 6	Transparency: Refraction on
		Reflection: Fresnel off and Ray trace on */
    /* 7	Transparency: Refraction on
		Reflection: Fresnel on and Ray trace on */
    /* 8	Reflection on and Ray trace off */
    /* 9	Transparency: Glass on
		Reflection: Ray trace off */
    /* 10	Casts shadows onto invisible surfaces */
};

//! bit mask that records how a component is specified
enum {
    DefaultComponent = 0,       //!< the component is not specified in the material
    UniformComponent = 1,       //!< there is a uniform value for the component
    MapComponent = 2            //!< there is a map for the component
};

//! Structure that defines a material in a model.
struct Material {
    std::string         name;           //!< name of material
    int                 illum;          //!< illumination mode (NoLight, etc.)
    int                 ambientC;       //!< how is the ambient light specified?
    int                 emissiveC;      //!< how is the emissive light specified?
    int                 diffuseC;       //!< how is the diffuse light specified?
    int                 specularC;      //!< how is the specular light specified?
    glm::vec3           ambient;        //!< uniform ambient component
    glm::vec3           emissive;       //!< uniform emissive component
    glm::vec3           diffuse;        //!< uniform diffuse component
    glm::vec3           specular;       //!< uniform specular component
    float               shininess;      //!< uniform specular exponent
    std::string         ambientMap;     //!< optional texture map for ambient
    std::string         emissiveMap;    //!< optional texture map for emissive lighting
    std::string         diffuseMap;     //!< optional texture map for diffuse lighting
    std::string         specularMap;    //!< optional texture map for specular highlights
    std::string         normalMap;      //!< optional normal map for bump mapping
}; // struct Material

//! A Group is a connected mesh that has a single material.  It is represented
//! by per-vertex data (position, normal, and texture coordinate) and an index
//! array that defines a list of triangles.
struct Group {
    std::string         name;           //!< name of this group */
    int                 material;       //!< index to material for group (-1 for no material)
    uint32_t            nVerts;         //!< the number of vertices in this group.
    uint32_t            nIndices;       //!< the number of indices (3 * number of triangles)
    glm::vec3           *verts;         //!< array of nVerts vertex coordinates
    glm::vec3           *norms;         //!< array of nVerts normal vectors (or nullptr)
    glm::vec2           *txtCoords;     //!< array of nVerts texture coordinates (or nullptr)
    uint32_t            *indices;       //!< array of nIndices element indices that can be used
                                        //!  to render the group
}; // struct Group

//! A model from an OBJ file
class Model {
  public:

  //! create a Model by loading it from the specified OBJ file
  //! \param filename the path of the OBJ file to be loaded
    Model (std::string filename);
    ~Model ();

  //! the model's axis-aligned bounding box
    const cs237::AABBf &BBox () const { return this->_bbox; }

  //! the number of materials associated with this model
    int NumMaterials () const { return this->_materials.size(); }
  //! get a material
    const OBJ::Material & Material (int i) const { return this->_materials[i]; }

  //! the number of groups in this model
    int NumGroups () const { return this->_groups.size(); }
  //! get a group by index
    const OBJ::Group & Group (int i) const { return this->_groups[i]; }
  //! iterator for looping over the groups in the model
    std::vector<OBJ::Group>::const_iterator beginGroups () const { return this->_groups.begin(); }
  //! terminator for looping over the groups in the model
    std::vector<OBJ::Group>::const_iterator endGroups () const { return this->_groups.end(); }

  private:
    std::string         _path;          //!< path to the obj file that this model came from
    std::string         _mtlLibName;    //!< name of the material library for this model
    cs237::AABBf        _bbox;          //!< bounding box for model

    std::vector<OBJ::Material> _materials;
    std::vector<OBJ::Group> _groups;

  // read a material library
    bool readMaterial (std::string m);

}; // class Model

} // namespace OBJ

#endif // !_OBJ_HXX_
