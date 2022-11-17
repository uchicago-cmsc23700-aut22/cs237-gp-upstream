/*! \file map.cpp
 *
 * \author John Reppy
 *
 * Information about heightfield maps.
 */

/* CMSC23700 Final Project sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "json.hpp"
#include "cs237.hpp"
#include "map.hpp"
#include "map-cell.hpp"
#include <unistd.h>

/***** class Map member functions *****/

Map::Map (cs237::Application *app) : _app(app), _grid(nullptr), _objects(nullptr) { }

Map::~Map ()
{
    if (this->_grid != nullptr) {
        for (int i = 0;  i < this->_nCells();  i++) {
            if (this->_grid[i] != nullptr)
                delete this->_grid[i];
        }
        delete this->_grid;
    }

}

static void error (std::string file, std::string msg)
{
    std::cerr << "error reading map file \"" << file << "\": " << msg << "\n";
}

// helper function to get an optional array of three numbers from the map object; returns
// true if there is an error (but false if the array is null)
//
static bool getThreeFloats (const json::Array *arr, float out[3])
{
    if (arr == nullptr) {
      // array is not present
        return true;
    }

    if (arr->length() != 3) {
        return true;
    }

    for (int i = 0;  i < 3;  i++) {
        const json::Number *num = (*arr)[i]->asNumber();
        if (num == nullptr) {
            return false;
        }
        out[i] = static_cast<float>(num->realVal());
    }

    return false;
}

bool Map::load (std::string const &mapName, bool verbose)
{
    if (this->_grid != nullptr) {
      // map file has already been loaded, so return false
        return false;
    }

    this->_path = mapName + "/";

    json::Value *map = json::parseFile(this->_path + "map.json");
    const json::Object *root = (map != nullptr) ? map->asObject() : nullptr;

    if (root == nullptr) {
        error (mapName, "expected object");
        return false;
    }

  // get map name
    const json::String *name = root->fieldAsString("name");
    if (name == nullptr) {
        error (mapName, "missing/bogus name field");
        return false;
    }
    this->_name = name->value();

    const json::Number *num;

  // get hScale
    num = root->fieldAsNumber("h-scale");
    if (num == nullptr) {
        error (mapName, "missing/bogus h-scale field");
        return false;
    }
    this->_hScale = static_cast<float>(num->realVal());

  // get vScale
    num = root->fieldAsNumber("v-scale");
    if (num == nullptr) {
        error (mapName, "missing/bogus v-scale field");
        return false;
    }
    this->_vScale = static_cast<float>(num->realVal());

  // get base elevation (optional)
    json::Value *v = (*root)["base-elev"];
    if (v != nullptr) {
        num = v->asNumber();
        if (num == nullptr) {
            error (mapName, "bogus base-elev field");
            return false;
        }
        this->_baseElev = static_cast<float>(num->realVal());
    }
    else
        this->_baseElev = 0.0;

  // get minimum elevation
    num = root->fieldAsNumber("min-elev");
    if (num == nullptr) {
        error (mapName, "missing/bogus min-elev field");
        return false;
    }
    this->_minElev = static_cast<float>(num->realVal());

  // get maximum elevation
    num = root->fieldAsNumber("max-elev");
    if (num == nullptr) {
        error (mapName, "missing/bogus max-elev field");
        return false;
    }
    this->_maxElev = static_cast<float>(num->realVal());

  // get bottom of skybox
    num = root->fieldAsNumber("min-sky");
    if (num == nullptr) {
        error (mapName, "missing/bogus min-sky field");
        return false;
    }
    this->_minSky = static_cast<float>(num->realVal());

  // get top of skybox
    num = root->fieldAsNumber("max-sky");
    if (num == nullptr) {
        error (mapName, "missing/bogus max-sky field");
        return false;
    }
    this->_maxSky = static_cast<float>(num->realVal());

  // get map width
    const json::Integer *intnum = root->fieldAsInteger("width");
    if ((intnum == nullptr) || (intnum->intVal() < 1)) {
        error (mapName, "missing/bogus width field");
        return false;
    }
    this->_width = static_cast<uint32_t>(intnum->intVal());

  // get map height
    intnum = root->fieldAsInteger("height");
    if ((intnum == nullptr) || (intnum->intVal() < 1)) {
        error (mapName, "missing/bogus height field");
        return false;
    }
    this->_height = static_cast<uint32_t>(intnum->intVal());

  // get cell size
    intnum = root->fieldAsInteger("cell-size");
    if ((intnum == nullptr) || (intnum->intVal() < kMinCellSize)
    || (kMaxCellSize < intnum->intVal()))
    {
        error (mapName, "missing/bogus cell-size field");
        return false;
    }
    this->_cellSize = static_cast<uint32_t>(intnum->intVal());

  // check properties
    v = (*root)["color-map"];
    if (v != nullptr) {
        const json::Bool *b = v->asBool();
        if (b == nullptr) {
            error (mapName, "bogus color-map field");
            return false;
        }
        this->_hasColor = b->value();
    }
    else
        this->_hasColor = false;

    v = (*root)["normal-map"];
    if (v != nullptr) {
        const json::Bool *b = v->asBool();
        if (b == nullptr) {
            error (mapName, "bogus normal-map field");
            return false;
        }
        this->_hasNormals = b->value();
    }
    else
        this->_hasNormals = false;

    v = (*root)["water-map"];
    if (v != nullptr) {
        const json::Bool *b = v->asBool();
        if (b == nullptr) {
            error (mapName, "bogus water-map field");
            return false;
        }
        this->_hasWater = b->value();
    }
    else
        this->_hasWater = false;

  // lighting info (optional)
    {
        float sunDir[3] = { 0.0, 1.0, 0.0 };  // default is high noon
        float sunI[3] = { 0.9, 0.9, 0.9 };  // bright sun
        float ambI[3] = { 0.1, 0.1, 0.1 };

        if (getThreeFloats (root->fieldAsArray("sun-dir"), sunDir)) {
            error (mapName, "bogus sun-dir field");
            return false;
        }

        if (getThreeFloats (root->fieldAsArray("sun-intensity"), sunI)) {
            error (mapName, "bogus sun-intensity field");
            return false;
        }

        if (getThreeFloats (root->fieldAsArray("ambient"), ambI)) {
            error (mapName, "bogus ambient field");
            return false;
        }

        this->_sunDir = normalize (glm::vec3(sunDir[0], sunDir[1], sunDir[2]));
        this->_sunI = glm::vec3(sunI[0], sunI[1], sunI[2]);
        this->_ambI = glm::vec3(ambI[0], ambI[1], ambI[2]);
    }

  // fog (optional)
    if ((*root)["fog-color"] != nullptr) {
        float fogColor[3];
        this->_hasFog = true;
        if (getThreeFloats (root->fieldAsArray("fog-color"), fogColor)) {
            error (mapName, "bogus fog-color field");
            return false;
        }
        this->_fogColor = glm::vec3(fogColor[0], fogColor[1], fogColor[2]);

        num = root->fieldAsNumber("fog-density");
        if (num == nullptr) {
            error (mapName, "missing/bogus fog-density field");
            return false;
        }
        this->_fogDensity = num->realVal();
    }
    else {
        this->_hasFog = false;
        this->_fogColor = glm::vec3(0, 0, 0);
        this->_fogDensity = 0;
    }

  // are there any objects?
    /* PART 2 code goes here */

  // compute and check other map info
    int cellShft = ilog2(this->_cellSize);
    if ((cellShft < 0)
    || (this->_cellSize < Map::kMinCellSize)
    || (Map::kMaxCellSize < this->_cellSize)) {
        error (mapName, "cellSize must be power of 2 in range");
        return false;
    }

    this->_nRows = this->_height >> cellShft;
    this->_nCols = this->_width >> cellShft;
    if ((this->_nRows << cellShft) != this->_height) {
        error (mapName, "map height must be multiple of cell size");
        return false;
    }
    if ((this->_nCols << cellShft) != this->_width) {
        error (mapName, "map width must be multiple of cell size");
        return false;
    }

    if (verbose) {
        std::clog << "name = " << this->_name << "\n";
        std::clog << "h-scale = " << this->_hScale << "\n";
        std::clog << "v-scale = " << this->_vScale << "\n";
        std::clog << "base-elev = " << this->_baseElev << "\n";
        std::clog << "min-elev = " << this->_minElev << "\n";
        std::clog << "max-elev = " << this->_maxElev << "\n";
        std::clog << "min-sky = " << this->_minSky << "\n";
        std::clog << "max-sky = " << this->_maxSky << "\n";
        std::clog << "width = " << this->_width
            << " (" << this->_nCols << " cols)\n";
        std::clog << "height = " << this->_height
            << " (" << this->_nRows << " rows)\n";
        std::clog << "cell-size = " << this->_cellSize << "\n";
        std::clog << "sun-dir = " << this->_sunDir << "\n";
        std::clog << "sun-intensity = " << this->_sunI << "\n";
        std::clog << "ambient = " << this->_ambI << "\n";
        std::clog << "fog-color = " << this->_fogColor << "\n";
        std::clog << "fog-density = " << this->_fogDensity << "\n";
    }

  // get array of grid filenames
    const json::Array *grid = root->fieldAsArray("grid");
    if (num == nullptr) {
        error (mapName, "missing/bogus h-scale field");
        return false;
    }
    else if (grid->length() != this->_nCells()) {
        error (mapName, "incorrect number of cells in grid field");
        return false;
    }
    this->_grid = new class Cell*[this->_nCells()];
    for (int r = 0;  r < this->_nRows;  r++) {
        for (int c = 0;  c < this->_nCols;  c++) {
            int i = this->_cellIdx(r, c);
            const json::String *s = (*grid)[i]->asString();
            if (s == nullptr) {
                error (mapName, "bogus grid item");
            }
            this->_grid[i] = new class Cell(this, r, c, this->_path + s->value());
        }
    }

    return true;

}


/***** Utility functions *****/

// return the integer log2 of n; if n is not a power of 2, then return -1.
int ilog2 (uint32_t n)
{
    int k = 0, two_k = 1;
    while (two_k < n) {
        k++;
        two_k *= 2;
        if (two_k == n)
            return k;
    }
    return -1;

}
