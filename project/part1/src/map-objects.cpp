/*! \file map-objects.cpp
 *
 * Implementation of the Objects class.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "map-objects.hpp"
#include "map.hpp"
#include "map-cell.hpp"
#include "mesh.hpp"
#include "json.hpp"
#include <unistd.h>

/* helper functions to make extracting values from the JSON easier */

//! load a vec3f from a JSON object.
//! \return false if okay, true if there is an error.
bool loadVec3 (json::Object const *jv, glm::vec3 &vec)
{
    if (jv == nullptr) {
        return true;
    }

    const json::Number *x = jv->fieldAsNumber("x");
    const json::Number *y = jv->fieldAsNumber("y");
    const json::Number *z = jv->fieldAsNumber("z");
    if ((x == nullptr) || (y == nullptr) || (z == nullptr)) {
        return true;
    }

    vec = glm::vec3 (
        static_cast<float>(x->realVal()),
        static_cast<float>(y->realVal()),
        static_cast<float>(z->realVal()));

    return false;
}

//! load a color3f from a JSON object.
//! \return false if okay, true if there is an error.
bool loadColor (json::Object const *jv, glm::vec3 &color)
{
    if (jv == nullptr) {
        return true;
    }

    const json::Number *r = jv->fieldAsNumber("r");
    const json::Number *g = jv->fieldAsNumber("g");
    const json::Number *b = jv->fieldAsNumber("b");
    if ((r == nullptr) || (g == nullptr) || (b == nullptr)) {
        return true;
    }

    color = glm::vec3 (
        static_cast<float>(r->realVal()),
        static_cast<float>(g->realVal()),
        static_cast<float>(b->realVal()));

    return false;
}

/***** class Objects member functions *****/

Objects::Objects (cs237::Application *app, const Map *map)
  : _app(app), _map(map), _objsDir(map->_path + "objects/"), _objs(), _texs()
{ }

// load the objects instances for a map cell
//
bool Objects::loadObjects (std::string const &cell, std::vector<Instance *> &objs)
{
    std::string objsFile = cell + "/objects.json";

  // make sure that objs is empty
    objs.clear();

  // if the objects.json file does not exist, then we return the empty
  // vector
    if (access(objsFile.c_str(), F_OK) != 0) {
        return false;
    }

  // load the objects list
    json::Value *root = json::parseFile(objsFile);

  // check for errors
    if (root == nullptr) {
        std::cerr << "Unable to load objects list \"" << objsFile << "\"\n";
        return true;
    } else if (! root->isArray()) {
        std::cerr << "Invalid object list in \"" << objsFile
            << "\"; root is not an array\n";
        return true;
    }
    const json::Array *rootObj = root->asArray();

    if (rootObj->length() != 0) {
      // allocate space for the objects in the scene
        objs.reserve(rootObj->length());

      // load the object instances in the cell
        for (int i = 0;  i < rootObj->length();  i++) {
            json::Object const *object = (*rootObj)[i]->asObject();
            if (object == nullptr) {
                std::cerr << "Expected array of JSON objects in \""
                    << objsFile << "\"\n";
                return true;
            }
            json::String const *file = object->fieldAsString("file");
            json::Object const *frame = object->fieldAsObject("frame");
            glm::vec3 pos, xAxis, yAxis, zAxis;
            glm::vec3 color;
            if ((file == nullptr) || (frame == nullptr)
            ||  loadVec3 (object->fieldAsObject("pos"), pos)
            ||  loadVec3 (frame->fieldAsObject("x-axis"), xAxis)
            ||  loadVec3 (frame->fieldAsObject("y-axis"), yAxis)
            ||  loadVec3 (frame->fieldAsObject("z-axis"), zAxis)
            ||  loadColor (object->fieldAsObject("color"), color)) {
                std::cerr << "Invalid object description in \"" << objsFile << "\"\n";
                return true;
            }
            Instance *inst = this->_makeInstance(
                file->value(),
                glm::mat4 (
                    glm::vec4 (xAxis, 0.0f),
                    glm::vec4 (yAxis, 0.0f),
                    glm::vec4 (zAxis, 0.0f),
                    glm::vec4 (pos, 1.0f)),
                color);
          // add to objs vector
            objs.push_back (inst);
        }

    }

    return false;
}

// load an OBJ model from a file
//
GObject *Objects::loadModel (
    std::string const &dir,
    std::string const &file,
    cs237::AABBf &bbox)
{
    GObject *gObj;
    auto it = this->_objs.find (file);
    if (it == this->_objs.end()) {
      // load the model from the OBJ file
        OBJ::Model *model = new OBJ::Model (dir + file);
        bbox = model->BBox();
      // preload any textures in the materials of the model
        for (auto grpIt = model->beginGroups();  grpIt != model->endGroups();  grpIt++) {
            const OBJ::Material *mat = &model->Material((*grpIt).material);
            /* we ignore the ambient map */
            this->_loadTexture (dir, mat->emissiveMap, true);
            this->_loadTexture (dir, mat->diffuseMap, true);
            this->_loadTexture (dir, mat->specularMap, true, false);
            this->_loadTexture (dir, mat->normalMap, false, false);
        }
      // create the meshes
        gObj = new std::vector<TriMesh *>();
        gObj->reserve(model->NumGroups());
        for (auto git = model->beginGroups();  git != model->endGroups();  git++) {
            TriMesh *mesh = new TriMesh (this, model, *git);
            gObj->push_back(mesh);
        }
      // cache the meshes
        this->_objs[file] = std::pair<cs237::AABBf, GObject *>(bbox, gObj);
      // we can release the storage for the model
        delete model;
    }
    else {
        bbox = it->second.first;
        gObj = it->second.second;
    }

    return gObj;
}

// helper function for making instances of objects
Instance *Objects::_makeInstance (
    std::string const &file,
    glm::mat4 const &toCell,
    glm::vec3 const &color)
{
  // first we need to get the GObject and bounding box for the file
    cs237::AABBf bbox;
    GObject *gObj = this->loadModel (this->_objsDir, file, bbox);

    assert (gObj != nullptr);

  // create the instance
    Instance *inst = new Instance;
    inst->meshes = gObj;
    inst->toCell = toCell;
    inst->normToWorld = glm::inverseTranspose(glm::mat3x3(toCell));
    inst->normFromWorld = glm::transpose(inst->normToWorld);
    inst->color = color;

  // compute the bounding box after transformation to cell coordinates
    inst->bbox.clear();
    for (int j = 0;   j < 8;  j++) {
        glm::vec3 pt = glm::vec3(inst->toCell * glm::vec4(bbox.corner(j), 1));
        inst->bbox.addPt(pt);
    }

    return inst;

}

// return the pointer to a pre-loaded 2D texture
//
cs237::Texture2D *Objects::loadTexture2D (std::string const &name) const
{
    if (! name.empty()) {
        auto it = this->_texs.find(name);
        if (it != this->_texs.end()) {
            return it->second;
        }
    }
    return nullptr;

}

// helper function for loading textures
//
void Objects::_loadTexture (std::string path, std::string name, bool genMipmaps, bool sRGB)
{
    if (name.empty()) {
        return;
    }
  // have we already loaded this texture?
    if (this->_texs.find(name) != this->_texs.end()) {
        return;
    }
  // load the image data;
    cs237::Image2D *img;
    if (sRGB) {
        img = new cs237::Image2D(path + name);
    } else {
        img = new cs237::DataImage2D(path + name);
    }
    if (img == nullptr) {
        std::cerr << "Unable to find texture-image file \"" << path+name << "\"\n";
        exit (1);
    }
    cs237::Texture2D *txt = new cs237::Texture2D(this->_app, img, genMipmaps);
  // add to _texs map
    this->_texs.insert (std::pair<std::string, cs237::Texture2D *>(name, txt));
  // free image
    delete img;

}
