/*! \file texture-cache.cpp
 *
 * \author John Reppy
 */

/* CMSC23700 Final Project sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "texture-cache.hpp"
#include <utility>

#define ONE_MEG         (1024*1024)
#define ONE_GIG         (1024*ONE_MEG)

// initialize the texture cache
TextureCache::TextureCache (cs237::Application *app)
    : _app(app), _residentLimit(ONE_GIG), _residentSzb(0), _clock(0)
{ }

TileTexture *TextureCache::make (tqt::TextureQTree *tree, int level, int row, int col)
{
    TextureCache::Key key(tree, level, row, col);
    TextureCache::TextureTbl::const_iterator got = this->_textureTbl.find(key);
    if (got == this->_textureTbl.end()) {
        TileTexture *txt = new TileTexture(this, tree, level, row, col);
        this->_textureTbl.insert(std::pair<TextureCache::Key,TileTexture *>(key, txt));
        return txt;
    }
    else {
        return got->second;
    }

}

// record that the given texture is now active
void TextureCache::_makeActive (TileTexture *txt)
{
    assert (! txt->_active);
    if (txt->_activeIdx >= 0) {
        assert (this->_inactive[txt->_activeIdx] == txt);

      // first remove txt from the inactive list by moving the last element to where it is
        TileTexture *last = this->_inactive.back();
        this->_inactive[txt->_activeIdx] = last;
        this->_inactive.pop_back();
        last->_activeIdx = txt->_activeIdx;
    }
    // else txt did not have a texture associated with it, so it is not on the inactive list

  // add txt to the active list
    txt->_activeIdx = this->_active.size();
    this->_active.push_back(txt);

}

// record that the given texture is now inactive
void TextureCache::_release (TileTexture *txt)
{
    assert (txt->_active);
    assert (this->_active[txt->_activeIdx] == txt);

  // first remove txt from the active list by moving the last element to where it is
    TileTexture *last = this->_active.back();
    this->_active[txt->_activeIdx] = last;
    this->_active.pop_back();
    last->_activeIdx = txt->_activeIdx;

  // add txt to the inactive list
    txt->_activeIdx = this->_inactive.size();
    this->_inactive.push_back(txt);
}

cs237::Texture2D *TextureCache::_allocTex2D (cs237::Image2D *img)
{
/* FIXME: eventually, we should reuse inactive textures to reduce GPU memory pressure */
    cs237::Texture2D *txt = new cs237::Texture2D (this->_app, img, true);

    cs237::Application::SamplerInfo info(
        VK_FILTER_LINEAR,
        VK_FILTER_LINEAR,
        VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_BORDER_COLOR_INT_OPAQUE_BLACK);

    this->_sampler = this->_app->createSampler (info);

    return txt;
}

/***** class TileTexture member functions *****/

TileTexture::TileTexture (TextureCache *cache, tqt::TextureQTree *tree, int level, int row, int col)
    : _txt(nullptr), _cache(cache), _tree(tree), _level(level), _row(row), _col(col),
      _lastUsed(0), _activeIdx(-1), _active(false)
{ }

TileTexture::~TileTexture ()
{
/* FIXME: remove from cache data structures as needed */
    if (this->_txt != nullptr)
        delete this->_txt;
}

// preload the texture data into OpenGL; this operation is a hint to the texture
// cache that the texture cache that the texture is going to be used soon.
void TileTexture::activate ()
{
    assert (! this->_active);
    if (this->_txt == nullptr) {
      // load the image data from the TQT and create a texture for it
        cs237::Image2D *img = this->_tree->loadImage (this->_level, this->_row, this->_col);
        this->_txt = this->_cache->_allocTex2D (img);
    }

    this->_cache->_makeActive (this);
    this->_active = true;

}

// hint to the texture cache that this texture is not needed.
void TileTexture::release ()
{
    assert (this->_active);
    this->_cache->_release (this);
    this->_active = false;
}
