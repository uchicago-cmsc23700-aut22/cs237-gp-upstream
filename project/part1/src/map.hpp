/*! \file map.hpp
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

#ifndef _MAP_HPP_
#define _MAP_HPP_

#include "cs237.hpp"

class Cell; // cells in the map grid
class Objects; // objects on the map

//! Information about a heightfield map.
class Map {
  public:

    Map ();
    ~Map ();

  //! \brief load a map
  //! \param path the name of the directory that contains that map
  //! \param verbose when true (the default), the loader prints information about
  //!        the map to \c std::clog.
  //! \return true if there are no errors, false if there was an error
  //!         reading the map.
    bool LoadMap (std::string const &path, bool verbose=true);

  // return the descriptive name of the map
    std::string Name () const { return this->_name; }
  //! return the number of rows in the map's grid of cells (rows increase to the south)
    uint32_t nRows () const { return this->_nRows; }
  //! return the number of columns in the map's grid of cells (columns increase to the east)
    uint32_t nCols () const { return this->_nCols; }
  //! return the width of a cell in hScale units.  This value will be a power of 2.
  //! Note that the size measured in number of vertices is cellWidth()+1
    uint32_t CellWidth () const { return this->_cellSize; }
  //! get the map width (east/west dimension) in hScale units (== nCols() * cellWidth())
    uint32_t Width () const { return this->_width; }
  //! get the map height (north/south dimension) in hScale units (== nRows() * cellWidth())
    uint32_t Height () const { return this->_height; }
  //! get the map horizontal scale
    float hScale () const { return this->_hScale; }
  //! get the map vertical scale
    float vScale () const { return this->_vScale; }
  //! get the base elevation (what 0 maps to)
    float BaseElevation () const { return this->_baseElev; }
  //! get the minimum elevation in the map
    float MinElevation () const { return this->_minElev; }
  //! get the maximum elevation in the map
    float MaxElevation () const { return this->_maxElev; }
  //! get the bottom of the skybox
    float SkyBottom () const { return this->_minSky; }
  //! get the top of the skybox
    float SkyTop () const { return this->_maxSky; }
  //! does the map have a color-map texture?
    bool hasColorMap () const { return this->_hasColor; }
  //! does the map have a normal-map texture?
    bool hasNormalMap () const { return this->_hasNormals; }
  //! does the map have a water mask?
    bool hasWaterMask () const { return this->_hasWater; }
  //! unit direction vector toward sun
    glm::vec3 SunDirection () const { return this->_sunDir; }
  //! intensity of sunlight
    cs237::color3f SunIntensity () const { return this->_sunI; }
  //! intensity of ambient light
    cs237::color3f AmbientIntensity () const { return this->_ambI; }
  //! does the map have fog information?
    bool hasFog () const { return this->_hasFog; }
  //! return the fog color (assuming hasFog() is true)
    cs237::color3f FogColor () const { return this->_fogColor; }
  //! return the fog density constant (assuming hasFog() is true)
    float FogDensity () const { return this->_fogDensity; }
  //! does a map have an 'objects' directory?
    bool hasObjects () const { return (this->_objects != nullptr); }

  //! return the cell at grid cell (row, col)
    class Cell *Cell (uint32_t row, uint32_t col) const;

  //! return the grid cell that contains the position (x, 0, z)
    class Cell *CellAt (double x, double z) const;

  //! return the size of a cell in world coordinates (note that the Y component will be 0)
    cs237::vec3d CellSize () const;

  //! return the NW corner of a cell in world coordinates (note that the Y component will be 0)
    cs237::vec3d NWCellCorner (uint32_t row, uint32_t col) const;

  //! return the north side's Z coordinate of the map in world coordinates
    double North () const;

  //! return the east side's X coordinate of the map in world coordinates
    double East () const;

  //! return the south side's Z coordinate of the map in world coordinates
    double South () const;

  //! return the west side's X coordinate of the map in world coordinates
    double West () const;

  //! the minimum cell width
    static const uint32_t MIN_CELL_SIZE = (1 << 8);
  //! the maximum cell width
    static const uint32_t MAX_CELL_SIZE = (1 << 14);

  private:
    std::string _path;          //!< path to the map directory
    std::string _name;          //!< title of map
    float       _hScale;        //!< horizontal scale in meters
    float       _vScale;        //!< vertical scale in meters
    float       _baseElev;      //!< base elevation in meters
    float       _minElev;       //!< minimum elevation in meters
    float       _maxElev;       //!< maximum elevation in meters
    float       _minSky;        //!< bottom of skybox in meters
    float       _maxSky;        //!< top of skybox in meters
    uint32_t    _width;         //!< map width in _hScale units; note that width measured
                                //!  in number of vertices is _width+1
    uint32_t    _height;        //!< map height in _hScale units;  note that width measured
                                //!  in number of vertices is _height+1
    uint32_t    _cellSize;      //!< size of cell in _hScale units; must be a power of 2.
                                //!  Note that the size measured in number of vertices
                                //!  is _cellSize+1
    uint32_t    _nRows;         //!< height of map in number of cells
    uint32_t    _nCols;         //!< width of map in number of cells
    class Cell  **_grid;        //!< cells in column-major order
    bool        _hasColor;      //!< true if the map has a color-map texture
    bool        _hasNormals;    //!< true if the map has a normal-map texture
    bool        _hasWater;      //!< true if the map has a water mask.
    glm::vec3 _sunDir;       //!< unit vector pointing toward the sun
    cs237::color3f _sunI;       //!< intensity of the sun light
    cs237::color3f _ambI;       //!< intensity of ambient light
    bool        _hasFog;        //!< true if the map specification includes fog info
    cs237::color3f _fogColor;   //!< the color of the fog at full strength
    float       _fogDensity;    //!< the density factor for the fog; will be 0 for no fog
    Objects     *_objects;      //!< repository of object meshes and materials that
                                //!< are placed on the map

  //! the number of cells in the map
    uint32_t _nCells () const { return this->_nRows * this->_nCols; }

  //! the index of the cell at the given row and column
    uint32_t _cellIdx (uint32_t row, uint32_t col) const { return this->_nCols * row + col; }

    friend class Cell;
    friend class Objects;
};

/***** Utility functions *****/

//! return the integer log2 of a power of 2
//! \param[in] n the number to compute the log of (should be a power of 2)
//! \return k, where n = 2^k; otherwise return -1.
int ilog2 (uint32_t n);


/***** Inline methods *****/

// return a pointer to the cell at the given (row, column) of the map
//
inline class Cell *Map::Cell (uint32_t row, uint32_t col) const
{
    if ((row < this->_nRows) && (col < this->_nCols)) {
        return this->_grid[this->_cellIdx(row, col)];
    }
    else
        return nullptr;
}

// return a pointer to the cell containing the give (x,z) coordinate
//
inline class Cell *Map::CellAt (double x, double z) const
{
    if ((x < 0.0) || (z < 0.0))
        return nullptr;
    else
        return this->Cell(
            static_cast<uint32_t>(z / this->_hScale),
            static_cast<uint32_t>(x / this->_vScale));
}

// return the NW corner of a cell in world coordinates (note that the Y component will be 0)
inline cs237::vec3d Map::NWCellCorner (uint32_t row, uint32_t col) const
{
    assert ((row < this->_nRows) && (col < this->_nCols));
    double w = static_cast<double>(this->_hScale) * static_cast<double>(this->_cellSize);
    return cs237::vec3d(
        w * static_cast<double>(col),
        0.0,
        w * static_cast<double>(row));
}

// return the north side's Z coordinate of the map in world coordinates
inline double Map::North () const
{
    return 0.0;
}

// return the east side's X coordinate of the map in world coordinates
inline double Map::East () const
{
    return static_cast<double>(_hScale) * static_cast<double>(this->_width);
}

// return the south side's Z coordinate of the map in world coordinates
inline double Map::South () const
{
    return static_cast<double>(_hScale) * static_cast<double>(this->_height);
}

// return the west side's X coordinate of the map in world coordinates
inline double Map::West () const
{
    return 0.0;
}

// return the size of a cell in world coordinates (note that the Y component will be 0)
inline cs237::vec3d Map::CellSize () const
{
    double w = static_cast<double>(_hScale) * static_cast<double>(this->_cellSize);
    return cs237::vec3d(w, 0.0, w);
}

#endif // !_MAP_HPP_
