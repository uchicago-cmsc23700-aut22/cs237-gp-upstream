/*! \file mesh.cxx
 *
 * Implementation of the TriMesh class.
 *
 * \author John Reppy
 */

/* CMSC23700 Final Project sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hxx"
#include "mesh.hxx"
#include "map-objects.hxx"

//  Helper function to enable attribute locations
//
static inline void InitAttributeLoc (bool enable, GLuint loc)
{
    if (enable) {
        CS237_CHECK( glEnableVertexAttribArray (loc) );
    }
    else {
        CS237_CHECK( glDisableVertexAttribArray (loc) );
    }

}

/***** struct TriMesh member functions *****/

//! create a TriMesh object by initializing its buffer Ids.  The buffer data is
//! loaded separately.
TriMesh::TriMesh (GLenum p)
  : _vaoId(0), _prim(p), _nIndices(0),
    _hasEmissive(false), _hasSpecular(false),
    _eMap(nullptr), _dMap(nullptr), _sMap(nullptr), _nMap(nullptr)
{
  // generate the vertex array object for the mesh
    CS237_CHECK( glGenVertexArrays (1, &(this->_vaoId)) );
}

//! create a TriMesh object by initializing its buffer Ids.  The buffer data is
//! loaded separately.
TriMesh::TriMesh (const Objects *mapObjs, OBJ::Model const *model, OBJ::Group const &grp)
  : _vaoId(0), _prim(GL_TRIANGLES), _nIndices(grp.nIndices),
    _hasEmissive(false), _hasSpecular(false),
    _eMap(nullptr), _dMap(nullptr), _sMap(nullptr), _nMap(nullptr)
{
  // get the material for the group
    const OBJ::Material *mtl;
    if (grp.material < 0) {
        mtl = nullptr;
    }
    else {
        mtl = &model->Material(grp.material);
    }

  /***** get the material properties for the group *****/

  // diffuse properties
    if ((mtl->diffuseC & OBJ::UniformComponent) == 0) {
        std::cerr << "Missing diffuse color specification in material "
            << mtl->name << "\n";
        exit (1);
    }
    this->_diffuseC = mtl->diffuse;
    if ((mtl->diffuseC & OBJ::MapComponent) != 0) {
        this->_dMap = mapObjs->LoadTexture2D(mtl->diffuseMap);
    }
  // specular properties
    if ((mtl->specularC & OBJ::UniformComponent) != 0) {
        this->_hasSpecular = true;
        this->_specularC = mtl->specular;
        this->_sharpness = mtl->shininess;
    }
    if ((mtl->specularC & OBJ::MapComponent) != 0) {
        this->_sMap = mapObjs->LoadTexture2D(mtl->specularMap);
        this->_sharpness = mtl->shininess;
    }
  // emissive properties
    if ((mtl->emissiveC & OBJ::UniformComponent) != 0) {
        this->_hasEmissive = true;
        this->_emissiveC = mtl->emissive;
    }
    if ((mtl->emissiveC & OBJ::MapComponent) != 0) {
        this->_eMap = mapObjs->LoadTexture2D(mtl->emissiveMap);
    }
  // normal map
    this->_nMap = mapObjs->LoadTexture2D(mtl->normalMap);

  // generate the vertex array object for the mesh
    CS237_CHECK( glGenVertexArrays (1, &(this->_vaoId)) );
    CS237_CHECK( glBindVertexArray (this->_vaoId) );

  // assign the vertex-buffer ID
    CS237_CHECK( glGenBuffers (1, &this->_vBufId) );

  // assign the normal-buffer ID (if needed)
    if (grp.norms != nullptr) {
        this->_hasNorms = true;
        CS237_CHECK( glGenBuffers (1, &this->_nBufId) );
    }
    else {
        this->_hasNorms = false;
    }

  // assign the texture-coordinate-buffer ID (if needed)
    if (grp.txtCoords != nullptr) {
        this->_hasTxtCoords = true;
        CS237_CHECK( glGenBuffers (1, &this->_tcBufId) );
    }
    else {
        this->_hasTxtCoords = false;
    }

  // assign the tangent-buffer ID (if needed)
    if (this->_hasNorms && (this->_nMap != nullptr)) {
        this->_hasTans = true;
        CS237_CHECK( glGenBuffers (1, &this->_tanBufId) );
    }
    else {
        this->_hasTans = false;
    }

  // assign the element-buffer ID
    CS237_CHECK( glGenBuffers (1, &this->_eBufId) );

  // vertex buffer initialization
    CS237_CHECK( glBindBuffer (GL_ARRAY_BUFFER, this->_vBufId) );
    CS237_CHECK( glBufferData (
            GL_ARRAY_BUFFER,
            grp.nVerts*sizeof(cs237::vec3f),
            grp.verts,
            GL_STATIC_DRAW)
        );
    CS237_CHECK( glVertexAttribPointer (CoordAttrLoc, 3, GL_FLOAT, GL_FALSE, 0, 0) );
    CS237_CHECK( glEnableVertexAttribArray (CoordAttrLoc) );

  // normal buffer initialization
    if (this->_hasNorms) {
        CS237_CHECK( glBindBuffer (GL_ARRAY_BUFFER, this->_nBufId) );
        CS237_CHECK( glBufferData (
                GL_ARRAY_BUFFER,
                grp.nVerts*sizeof(cs237::vec3f),
                grp.norms,
                GL_STATIC_DRAW)
            );
        CS237_CHECK( glVertexAttribPointer (NormAttrLoc, 3, GL_FLOAT, GL_FALSE, 0, 0) );
    }

  // texture-coordinate buffer initialization
    if (this->_hasTxtCoords) {
        CS237_CHECK( glBindBuffer (GL_ARRAY_BUFFER, this->_tcBufId) );
        CS237_CHECK( glBufferData (
                GL_ARRAY_BUFFER,
                grp.nVerts*sizeof(cs237::vec2f),
                grp.txtCoords,
                GL_STATIC_DRAW)
            );
        CS237_CHECK( glVertexAttribPointer (TexCoordAttrLoc, 2, GL_FLOAT, GL_FALSE, 0, 0) );
    }

  // tangent-vector buffer initialization
    if (this->_hasTans) {
        cs237::vec3f *tan = new cs237::vec3f[grp.nVerts];
        cs237::vec3f *bitan = new cs237::vec3f[grp.nVerts];
        for (int i = 0;  i < grp.nVerts;  i++) {  // clear arrays
            tan[i] = cs237::vec3f();
            bitan[i] = cs237::vec3f();
        }
        cs237::vec4f *tangents = new cs237::vec4f[grp.nVerts];
        uint32_t nTris = grp.nIndices / 3;
        assert (nTris * 3 == grp.nIndices);
        for (int tri = 0;  tri < nTris;  tri++) {
          // get the indices for the triangle
            uint32_t i1 = grp.indices[3*tri + 0];
            uint32_t i2 = grp.indices[3*tri + 1];
            uint32_t i3 = grp.indices[3*tri + 2];
          // get the vertices for the triangle
            cs237::vec3f v1 = grp.verts[i1];
            cs237::vec3f v2 = grp.verts[i2];
            cs237::vec3f v3 = grp.verts[i3];
          // get the texture coordinates for the triangle
            cs237::vec2f vt1 = grp.txtCoords[i1];
            cs237::vec2f vt2 = grp.txtCoords[i2];
            cs237::vec2f vt3 = grp.txtCoords[i3];
          // the sides of the triangle as a 3x2 matrix
            cs237::mat3x2f Q = cs237::mat3x2f(
                v2.x - v1.x, v3.x - v1.x,   // column one
                v2.y - v1.y, v3.y - v2.y,   // column two
                v2.z - v1.z, v3.z - v1.z);  // column three
          // the sides in tangent space as a 2x2 matrix
            cs237::mat2x2f ST = cs237::mat2x2f (
                vt2.x - vt1.x, vt3.x - vt1.x,       // first column
                vt2.y - vt1.y, vt3.y - vt1.y);      // second column
          // Q = ST * [T B]^T, so multiply Q by ST^{-1}
            cs237::mat3x2f TB = ST.inverse() * Q;
          // extract rows T and B
            cs237::vec3f t = cs237::vec3f(TB[0][0], TB[1][0], TB[2][0]);
            cs237::vec3f b = cs237::vec3f(TB[0][1], TB[1][1], TB[2][1]);
          // add to vector sums
            tan[i1] += t;
            tan[i2] += t;
            tan[i3] += t;
            bitan[i1] += b;
            bitan[i2] += b;
            bitan[i3] += b;
        }
      // compute extended tangents for vertices
        for (int i = 0;  i < grp.nVerts;  i++) {
            cs237::vec3f n = grp.norms[i];
            cs237::vec3f t = tan[i];
          // orthogonalize
            t = normalize(t - n * dot(n, t));
            float w = (dot(cross(n, t), bitan[i]) < 0.0f ? -1.0f : 1.0f);
            tangents[i] = cs237::vec4f(t, w);
        }
        CS237_CHECK( glBindBuffer (GL_ARRAY_BUFFER, this->_tanBufId) );
        CS237_CHECK( glBufferData (GL_ARRAY_BUFFER, grp.nVerts*sizeof(cs237::vec4f), tangents, GL_STATIC_DRAW) );
        CS237_CHECK( glVertexAttribPointer (TanAttrLoc, 4, GL_FLOAT, GL_FALSE, 0, 0) );
        delete[] tan;
        delete[] bitan;
        delete[] tangents;
    }

  // index-array buffer initialization
    CS237_CHECK( glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, this->_eBufId) );
    CS237_CHECK( glBufferData (GL_ELEMENT_ARRAY_BUFFER, grp.nIndices*sizeof(uint32_t), grp.indices, GL_STATIC_DRAW) );

  // cleanup
    CS237_CHECK( glBindBuffer (GL_ARRAY_BUFFER, 0) );
    CS237_CHECK( glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0) );
    CS237_CHECK( glBindVertexArray (0) );

}

TriMesh::~TriMesh ()
{
    /* TODO */
}

void TriMesh::Draw (bool enableNorms, bool enableTexs, bool enableTans)
{
    CS237_CHECK( glBindVertexArray (this->_vaoId) );

  // bind the indices array
    CS237_CHECK( glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, this->_eBufId) );

  // enable attribute buffers
    CS237_CHECK( glEnableVertexAttribArray (CoordAttrLoc) );
    InitAttributeLoc (this->_hasNorms && enableNorms, NormAttrLoc);
    InitAttributeLoc (this->_hasTxtCoords && enableTexs, TexCoordAttrLoc);
    InitAttributeLoc (this->_hasTans && enableTans, TanAttrLoc);

  // render
    CS237_CHECK( glDrawElements (this->_prim, this->_nIndices, GL_UNSIGNED_INT, 0) );

  // cleanup
    CS237_CHECK( glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0) );
    CS237_CHECK( glBindVertexArray (0) );

}
