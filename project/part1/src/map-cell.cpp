/*! \file map-cell.cpp
 *
 * \author John Reppy
 *
 * Map cells.
 */

/* CMSC23700 Final Project sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hpp"
#include "map.hpp"
#include "map-cell.hpp"
#include "qtree-util.hpp"
#include <fstream>
#include <vector>
#include <iomanip>

// A cell file has the following layout on disk.  All data is in little-endian layout.
//
//      uint32_t magic;         // Magic number; should be 0x63656C6C ('cell')
//      uint32_t compressed;    // true if the chunks are compressed
//      uint32_t size;          // cell width (will be width+1 vertices wide)
//      uint32_t nLODs;
//      uint64_t toc[N];        // file offsets of chunks
//
// Each chunk has the layout
//
//      float maxError;         // maximum geometric error for this chunk
//      uint32_t nVerts;        // number of vertices
//      uint32_t nIndices;      // number of indices
//      int16_t minY;           // minimum active elevation value in this chunk
//      int16_t maxY;           // maximum active elevation value in this chunk
//      Vertex verts[nVerts];
//      uint16_t indices[nIndices];
//
// Each Vertex is represented by four 16-bit signed integers.

// A generic helper function for reading binary values from a input file
template <typename T>
inline T readVal (std::ifstream &inS)
{
    T v;

    if (inS.read(reinterpret_cast<char *>(&v), sizeof(T)).fail()) {
#ifndef NDEBUG
        std::cerr << "Cell::load: error reading file\n";
#endif
        exit (1);
    }
    return v;
}

inline int16_t readI16 (std::ifstream &inS) { return readVal<int16_t>(inS); }
inline uint32_t readUI32 (std::ifstream &inS) { return readVal<uint32_t>(inS); }
inline float readF32 (std::ifstream &inS) { return readVal<float>(inS); }
inline uint64_t readUI64 (std::ifstream &inS) { return readVal<uint64_t>(inS); }


/***** class Cell member functions *****/

Cell::Cell (Map *map, uint32_t r, uint32_t c, std::string const &stem)
    : _map(map), _row(r), _col(c), _stem(stem), _nLODs(0), _nTiles(0), _tiles(nullptr),
      _colorTQT(nullptr), _normTQT(nullptr)
{
}

Cell::~Cell () { }

// load the cell data
void Cell::load ()
{
    if (this->isLoaded())
        return;

    std::string file = this->_stem + "/hf.cell";
    std::ifstream inS(file, std::ifstream::in | std::ifstream::binary);
    if (inS.fail()) {
#ifndef NDEBUG
        std::cerr << "Cell::load: unable to open \"" << file << "\"\n";
#endif
        exit (1);
    }

  // get header info
    uint32_t magic = readUI32(inS);
    bool compressed = (readUI32(inS) != 0);
    uint32_t size = readUI32(inS);
    uint32_t nLODs = readUI32(inS);
    if (magic != Cell::kMagic) {
#ifndef NDEBUG
        std::cerr << "Cell::load: bogus magic number in header\n";
#endif
        exit (1);
    }
    else if (this->_map->_cellSize != size) {
#ifndef NDEBUG
        std::cerr << "Cell::load: expected cell size " << this->_map->_cellSize
            << " but found " << size << "\n";
#endif
        exit (1);
    }
    else if ((nLODs < Cell::kMinLODs) || (Cell::kMaxLODs < nLODs)) {
#ifndef NDEBUG
        std::cerr << "Cell::load: unsupported number of LODs\n";
#endif
        exit (1);
    }

    if (compressed) {
        std::cerr << "Cell::load: compressed files are not supported yet\n";
        exit (1);
    }

    uint32_t qtreeSize = qtree::fullSize(nLODs);
    std::vector<std::streamoff> toc(qtreeSize);
    for (int i = 0;  i < qtreeSize;  i++) {
        toc[i] = static_cast<std::streamoff>(readUI64(inS));
    }

  // allocate and load the tiles.  Note that tiles are numbered in a breadth-first
  // order in the LOD quadtree.
    this->_nLODs = nLODs;
    this->_nTiles = qtreeSize;
    this->_tiles = new class Tile[qtreeSize];

    this->_tiles[0]._init (this, 0, 0, 0, 0);

  // load the tile mesh data
    for (uint32_t id = 0;  id < qtreeSize;  id++) {
        Chunk *cp = &(this->_tiles[id]._chunk);
      // find the beginning of the chunk in the input file
        inS.seekg(toc[id]);
      // read the chunk's header
        cp->maxError = readF32(inS);
        uint32_t nVerts = readUI32(inS);
        uint32_t nIndices = readUI32(inS);
        cp->minY = readI16(inS);
        cp->maxY = readI16(inS);
      // allocate space for the chunk data
        this->_tiles[id]._allocChunk(nVerts, nIndices);
      // read the vertex data
        if (inS.read(reinterpret_cast<char *>(cp->vertices), cp->vSize()).fail()) {
            std::cerr << "Cell::load: error reading vertex data for tile " << id << "\n";
            exit (1);
        }
      // read the index array
        if (inS.read(reinterpret_cast<char *>(cp->indices), cp->iSize()).fail()) {
            std::cerr << "Cell::load: error reading index data for tile " << id << "\n";
            exit (1);
        }
      // compute the tile's bounding box.  We use double precision here, so that we can
      // support large worlds.
        glm::dvec3 nwCorner =
            this->_map->nwCellCorner(this->_row, this->_col) +
            glm::dvec3(
                this->_map->hScale() * double(this->_tiles[id]._col),
                double(this->_map->baseElevation() + this->_map->vScale() * float(cp->minY)),
                this->_map->hScale() * double(this->_tiles[id]._row));
        double w = this->_map->hScale() * this->_tiles[id].width();
        glm::dvec3 seCorner = nwCorner + glm::dvec3(w, 0.0, w);
        seCorner.y = static_cast<double>(
            this->_map->baseElevation() + this->_map->vScale() * float(cp->maxY));
        this->_tiles[id]._bbox = cs237::AABBd(nwCorner, seCorner);
    }

}

// load objects for a cell
//
void Cell::loadObjects ()
{
    /* Part 2 code to go here */
}

// load textures for a cell
//
void Cell::initTextures (Window *win)
{
  // load textures
    if (this->_map->hasColorMap()) {
        this->_colorTQT = new tqt::TextureQTree (this->datafile("/color.tqt"), true, true);
    }
    if (this->_map->hasNormalMap()) {
        this->_normTQT = new tqt::TextureQTree (this->datafile("/norm.tqt"), true, false);
    }
#ifndef NDEBUG
    if ((this->_colorTQT != nullptr) && (this->_normTQT != nullptr)) {
        assert (this->_colorTQT->depth() == this->_normTQT->depth());
    }
#endif

    /** HINT: add tile-specific texture initialization for the root tile here */

}

/***** class Tile member functions *****/

Tile::Tile ()
{
    this->_chunk.nVertices = 0;
    this->_chunk.nIndices = 0;
    this->_chunk.vertices = nullptr;
    this->_chunk.indices = nullptr;
}

Tile::~Tile ()
{
    delete this->_chunk.vertices;
    delete this->_chunk.indices;
}

void Tile::_allocChunk (uint32_t nv, uint32_t ni)
{
    this->_chunk.nVertices = nv;
    this->_chunk.nIndices = ni;
    this->_chunk.vertices = new HFVertex[nv];
    this->_chunk.indices = new uint16_t[ni];
}

// initialize the _cell, _id, etc. fields of this tile and its descendants.  The chunk and
// bounding box get set later
void Tile::_init (Cell *cell, uint32_t id, uint32_t row, uint32_t col, uint32_t lod)
{
    this->_cell = cell;
    this->_id = id;
    this->_row = row;
    this->_col = col;
    this->_lod = lod;

    if (lod+1 < cell->depth()) {
        uint32_t halfWid = (cell->width() >> (lod+1));
        struct { uint32_t dr, dc; } offset[4] = {
                { 0,       0       }, // NW
                { 0,       halfWid }, // NE
                { halfWid, halfWid }, // SE
                { halfWid, 0       }, // SW
            };
        int kidId = qtree::nwChild(id);
        for (int i = 0;  i < 4;  i++) {
            this->child(i)->_init (cell, kidId+i, row+offset[i].dr, col+offset[i].dc, lod+1);
        }
    }

}

void Tile::dump (std::ostream &outS)
{
    for (int i = 0;  i < this->_lod;  i++) {
        outS << "  ";
    }

    outS << "[" << std::setw(4) << this->_id << "]"
/* ADDITIONAL TILE STATE HERE */
        << "\n";

    for (int i = 0;  i < this->numChildren();  i++) {
        this->child(i)->dump (outS);
    }

}
