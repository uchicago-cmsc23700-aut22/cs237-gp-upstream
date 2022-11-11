/*! \file mtl-reader.cxx
 *
 * Reader for Wavefront MTL files.  The material reader has been extended with
 * Ke and map_Ke as suggested by
 *
 *      http://exocortex.com/blog/extending_wavefront_mtl_to_support_pbr
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "obj.hpp"
#include <fstream>
#include <stdexcept>
#include <utility>

namespace OBJ {

namespace __details {

// return the directory part of a pathname
static std::string dirName (std::string const &path)
{
    return path.substr(0, path.find_last_of('/'));
}

// scan one or more floats from a string
static bool scanFloats (std::string s, int n, float f[3])
{
    assert ((0 < n) && (n <= 3));
    try {
        for (int i = 0;  i < n;  i++) {
            size_t sz;
            f[i] = std::stof (s, &sz);
            s = s.substr(sz);
        }
    }
    catch (std::invalid_argument) {
        return false;
    }

    return true;
}

static bool scanInt (std::string s, int &n)
{
    try {
        n = std::stoi (s);
    }
    catch (std::invalid_argument) {
        return false;
    }

    return true;
}

static void Error (std::string const &file, int lnum, std::string const &msg)
{
    std::cerr << "Error [" << file << ":" << lnum << "] " << msg << std::endl;
}

static void Warning (std::string const &file, int lnum, std::string const &msg)
{
    std::cerr << "Warning [" << file << ":" << lnum << "] " << msg << std::endl;
}

bool ReadMaterial (
    std::string const &path,
    std::string const &m,
    std::vector<Material> &materials)
{
    std::string file = dirName(path) + "/" + m;
    int lnum = 0;
    std::ifstream inS(file);
    if (inS.fail()) {
        std::cerr << "Error: unable to open \"" << file << "\"" << std::endl;
        return false;
    }

  // parse the materials
    int count = 0;
    struct Material mtl;
    float fvals[3];
    int ival;
    std::string ln;
    while (std::getline(inS, ln).good()) {
        lnum++;
      // if there is a trailing '\r', then remove it
        if ((ln.length() > 0) && (ln.back() == '\r')) {
            ln.pop_back();
        }
      // Note that ln will _not_ include the end-of-line terminator!
        if ((ln.length() == 0) || (ln[0] == '#')) {
            continue;
        }
      // skip initial whitespace and ignore empty lines
        size_t pos = ln.find_first_not_of(" \t");
        if (pos == std::string::npos) {
            continue;
        }
        size_t pos2 = ln.find_first_of(" \t", pos);
        std::string firstTok = ln.substr(pos, pos2-pos);
        std::string rest = ln.substr(ln.find_first_not_of(" \t", pos2));
        if (firstTok.compare("newmtl") == 0) { // newmtl name
            if (count > 0) {
                materials.push_back(mtl);
            }
            ++count;
          // initialize mtl to default values
            mtl.name = (rest.empty() ? "default" : rest);
            mtl.illum = NoLight;
            mtl.ambientC = DefaultComponent;
            mtl.emissiveC = DefaultComponent;
            mtl.diffuseC = DefaultComponent;
            mtl.specularC = DefaultComponent;
            mtl.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
            mtl.emissive = glm::vec3(0.0f, 0.0f, 0.0f);
            mtl.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
            mtl.specular = glm::vec3(0.0f, 0.0f, 0.0f);
            mtl.shininess = 0.0f;
            mtl.ambientMap.clear();
            mtl.diffuseMap.clear();
            mtl.specularMap.clear();
            mtl.emissiveMap.clear();
            mtl.normalMap.clear();
        }
        else if (firstTok.compare("Ns") == 0) {  // Ns <float>
            if (scanFloats(rest, 1, fvals)) {
#ifdef WAVEFRONT_SHINY
              /* wavefront shininess is from [0, 1000], so scale for OpenGL */
                mtl.shininess = 128.0f * (fvals[0] / 1000.0f);
#else
                mtl.shininess = fvals[0];
#endif
            }
            else {
                Error (file, lnum, "expected \"Ns <float>\"");
            }
        }
#ifdef MORE_OBJ_FEATURES
        else if (firstTok.compare("Ni") == 0) {  // Ni <float>
            /* ignore refraction index */
        }
        else if (firstTok.compare("Tf") == 0) {  // Tf ...
            /* ignore transmission factor */
        }
        else if (firstTok.compare("d") == 0) {  // d <float> or d -halo <float>
            /* ignore dissolve */
        }
#endif
        else if (firstTok.compare("Kd") == 0) {  // Kd <float> <float> <float>
            if (scanFloats(rest, 3, fvals)) {
                mtl.diffuse = glm::vec3(fvals[0], fvals[1], fvals[2]);
                mtl.diffuseC |= UniformComponent;
            }
            else {
                Error (file, lnum, "expected \"Kd <float> <float> <float>\"");
            }
        }
        else if (firstTok.compare("Ka") == 0) {  // Ka <float> <float> <float>
            if (scanFloats(rest, 3, fvals)) {
                mtl.ambient = glm::vec3(fvals[0], fvals[1], fvals[2]);
                mtl.ambientC |= UniformComponent;
            }
            else {
                Error (file, lnum, "expected \"Ka <float> <float> <float>\"");
            }
        }
        else if (firstTok.compare("Ke") == 0) {  // Ke <float> <float> <float>
            if (scanFloats(rest, 3, fvals)) {
                mtl.emissive = glm::vec3(fvals[0], fvals[1], fvals[2]);
                mtl.emissiveC |= UniformComponent;
            }
            else {
                Error (file, lnum, "expected \"Ke <float> <float> <float>\"");
            }
       }
        else if (firstTok.compare("Ks") == 0) {  // Ks <float> <float> <float>
            if (scanFloats(rest, 3, fvals)) {
                mtl.specular = glm::vec3(fvals[0], fvals[1], fvals[2]);
                mtl.specularC |= UniformComponent;
            }
            else {
                Error (file, lnum, "expected \"Ks <float> <float> <float>\"");
            }
        }
        else if ((firstTok.compare("d") == 0) || (firstTok.compare("Tr") == 0)) {  // d <float> or Tr <float>
            if (scanFloats(rest, 1, fvals)) {
                if (fvals[0] < 1.0) {
                  /* ignore transparency filter */
                    Warning (file, lnum, "ignoring \"" + ln + "\"");
                }
            }
            else {
                Error (file, lnum, "expected \"d <float>\"");
            }
        }
#ifdef MORE_OBJ_FEATURES
        else if (firstTok.compare("Tf") == 0) {  // Tf <float> <float> <float>
            /* ignore transparency filter */
        }
#endif
        else if (firstTok.compare("illum") == 0) {  // illumn <int>
            if (scanInt(rest, ival)) {
                if ((0 <= ival) && (ival < 10)) {
                    mtl.illum = (ival <= Specular) ? ival : Specular;
                }
                else {
                    Warning (file, lnum, "expected \"illum [0-10]\"");
                    mtl.illum = Specular;
                }
            }
            else {
                Error (file, lnum, "expected \"illum <int>\"");
            }
        }
        else if (firstTok.compare("sharpness") == 0) {  // sharpness <float>
            if (scanFloats(rest, 1, fvals)) {
              /* wavefront shininess is from [0, 1000], so scale for OpenGL */
                mtl.shininess = 128.0f * (fvals[0] / 1000.0f);
            }
            else {
                Error (file, lnum, "expected \"sharpness <float>\"");
            }
        }
        else if (firstTok.compare("map_Kd") == 0) {  // map_Kd <name>
            mtl.diffuseMap = rest;
            mtl.diffuseC |= MapComponent;
        }
        else if (firstTok.compare("map_Ka") == 0) {  // map_Ka <name>
            mtl.ambientMap = rest;
            mtl.ambientC |= MapComponent;
        }
        else if (firstTok.compare("map_Ks") == 0) {  // map_Ks <name>
            mtl.specularMap = rest;
            mtl.specularC |= MapComponent;
        }
        else if (firstTok.compare("map_Ke") == 0) {  // map_Ke <name>
            mtl.emissiveMap = rest;
            mtl.emissiveC |= MapComponent;
        }
        else if ((firstTok.compare("map_Bump") == 0)
        || (firstTok.compare("map_bump") == 0)
        || (firstTok.compare("bump") == 0))
        {
            mtl.normalMap = rest;
        }
#ifdef MORE_OBJ_FEATURES
        else if ((firstTok.compare("map_Refl") == 0) || (firstTok.compare("map_refl") == 0)) {
            /* ignore reflection map */
        }
        else if (firstTok.compare("map_d") == 0) {  // map_d <name>
            /* ignore transparency map */
        }
#endif
        else {
            Warning (file, lnum, "ignoring \"" + ln + "\"");
        }
    } // while

    if (!inS.eof()) {
        Error (file, lnum, "input error");
        return false;
    }

    if (count > 0) {
        materials.push_back(mtl);
    }

    return true;

} // ReadMaterial

} // namespace __details
} // namespace OBJ
