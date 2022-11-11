/*! \file obj.cxx
 *
 * OBJ file support code.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "obj.hpp"
#include "obj-reader.hpp"
#include <unordered_map>
#include <cstdlib>

namespace OBJ {

namespace __details {
// helper function for reading MTL files
bool ReadMaterial (
    std::string const &path,            // path to directory containing material files
    std::string const &m,               // material file name
    std::vector<Material> &materials);  // vector of materials
}

struct VInfo {
    uint32_t _v, _n, _t;

    VInfo (uint32_t v, uint32_t n, uint32_t t) : _v(v), _n(n), _t(t) { }

    struct Hash {
        std::size_t operator() (VInfo const &v) const
        {
            return static_cast<std::size_t>(
                v._v +
                (v._n << 5) * 101 +
                (v._t << 10) * 101);
        }
    };
    struct Equal {
        bool operator()(VInfo const &v1, VInfo const &v2) const
        {
            return (v1._v == v2._v) && (v1._n == v2._n) && (v1._t == v2._t);
        }
    };

}; // VInfo

typedef std::unordered_map<VInfo,uint32_t,VInfo::Hash,VInfo::Equal> VertexMap_t;

Model::Model (std::string file)
    : _path(file), _bbox()
{
  // read the file
    OBJmodel *model = OBJReadOBJ (file.c_str());
    if (model == 0) {
        std::cerr << "unable to read model \"" << file << "\"" << std::endl;
        exit (1);
    }

  // load materials
    if (model->mtllibname != nullptr) {
        this->_mtlLibName = model->mtllibname;
        if (! __details::ReadMaterial (this->_path, this->_mtlLibName, this->_materials)) {
            std::cerr << "warning: error reading material library \""
                << this->_mtlLibName << "\"" << std::endl;
            this->_materials.clear();
        }
    }
    else {
        this->_materials.clear();
    }

  // compute the bounding box
    for (uint32_t i = 0;  i < model->numvertices;  i++) {
        this->_bbox.addPt (model->vertices[i]);
    }

  // build mesh data structures for the groups.  We need to identify unique v/n/t
  // triplets
    VertexMap_t map;
    std::vector<VInfo> verts;
    std::vector<uint32_t> indices;
    uint32_t idx;
    for (OBJgroup *grp = model->groups;  grp != nullptr;  grp = grp->next) {
        for (uint32_t i = 0;  i < grp->numtriangles;  i++) {
            for (int j = 0;  j < 3;  j++) {
                OBJtriangle *tri = &(model->triangles[i]);
                VInfo v(tri->vindices[j], tri->nindices[j], tri->tindices[j]);
                VertexMap_t::const_iterator got = map.find (v);
                if (got == map.end()) {
                    idx = verts.size();
                    verts.push_back (v);
                    map.insert (std::pair<VInfo,uint32_t>(v, idx));
                }
                else {
                    idx = got->second;
                }
                indices.push_back(idx);
            }
        }
      // here we have identified the mesh vertices for the group
        struct Group g;
        g.name = std::string(grp->name);
        g.material = -1;
        if (grp->material == nullptr) {
            for (uint32_t i = 0;  i < this->_materials.size();  i++) {
                if (this->_materials[i].name.compare("default") == 0) {
                    g.material = i;
                    break;
                }
            }
            if (g.material == -1) {
                std::cerr << "warning: no default material for group \""
                    << g.name << "\"" << std::endl;
            }
        }
        else {
            for (uint32_t i = 0;  i < this->_materials.size();  i++) {
                if (this->_materials[i].name.compare(grp->material) == 0) {
                    g.material = i;
                    break;
                }
            }
            if (g.material == -1) {
                std::cerr << "warning: unable to find material \"" << grp->material
                    << "\" for group \"" << g.name << "\"" << std::endl;
            }
        }
      // initialize the vertex data arrays
        g.nVerts = verts.size();
        g.verts = new glm::vec3[g.nVerts];
        if ((model->numnormals > 0) && (model->numtexcoords > 0)) {
          // has all three components
            g.norms = new glm::vec3[g.nVerts];
            g.txtCoords = new glm::vec2[g.nVerts];
            for (uint32_t i = 0;  i < g.nVerts;  i++) {
                g.verts[i] = model->vertices[verts[i]._v];
                g.norms[i] = model->normals[verts[i]._n];
                g.txtCoords[i] = model->texcoords[verts[i]._t];
            }
        }
        else if (model->numnormals > 0) {
            g.norms = new glm::vec3[g.nVerts];
            g.txtCoords = nullptr;
            for (uint32_t i = 0;  i < g.nVerts;  i++) {
                g.verts[i] = model->vertices[verts[i]._v];
                g.norms[i] = model->normals[verts[i]._n];
            }
        }
        else if (model->numtexcoords > 0) {
            g.norms = nullptr;
            g.txtCoords = new glm::vec2[g.nVerts];
            for (uint32_t i = 0;  i < g.nVerts;  i++) {
                g.verts[i] = model->vertices[verts[i]._v];
                g.txtCoords[i] = model->texcoords[verts[i]._t];
            }
        }
        else {
            g.norms = nullptr;
            g.txtCoords = nullptr;
            for (uint32_t i = 0;  i < g.nVerts;  i++) {
                g.verts[i] = model->vertices[verts[i]._v];
            }
        }
      // initialize the group's indices array
        g.nIndices = indices.size();
        g.indices = new uint32_t[g.nIndices];
        for (uint32_t i = 0;  i < g.nIndices;  i++) {
            g.indices[i] = indices[i];
        }
      // add to this model
        this->_groups.push_back (g);
      // cleanup
        map.clear();
        verts.clear();
        indices.clear();
    }

    delete model;

} // Model::Model

Model::~Model ()
{
  // free the storage for the groups
    for (uint32_t i = 0;  i < this->_groups.size();  i++) {
        assert (this->_groups[i].verts != nullptr);
        delete[] this->_groups[i].verts;
        if (this->_groups[i].norms != nullptr) {
            delete[] this->_groups[i].norms;
        }
        if (this->_groups[i].txtCoords != nullptr) {
            delete[] this->_groups[i].txtCoords;
        }
        assert (this->_groups[i].indices != nullptr);
        delete[] this->_groups[i].indices;
    }
} // Model::~Model

} // namespace OBJ
