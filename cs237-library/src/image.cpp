/*! \file cs237-image.cpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * Operations on image values.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237.hpp"
#include "png.h"
#include <fstream>

namespace cs237 {

//! \brief helper function to convert texture format to number of channels per pixel
static uint32_t numChannels (Channels fmt)
{
    switch (fmt) {
    case Channels::R: return 1;
    case Channels::RG: return 2;
    case Channels::RGB:
    case Channels::BGR: return 3;
    case Channels::RGBA:
    case Channels::BGRA: return 4;
    case Channels::UNKNOWN:
        ERROR("unknown format specified for image");
    }

} /* numChannels */

//! \brief helper function to convert element type to size in bytes
static size_t sizeOfType (ChannelTy ty)
{
    switch (ty) {
    case ChannelTy::U8:
    case ChannelTy::S8:
        return 1;
    case ChannelTy::U16:
    case ChannelTy::S16:
        return 2;
    case ChannelTy::U32:
    case ChannelTy::S32:
    case ChannelTy::F32:
        return 4;
    case ChannelTy::UNKNOWN:
        ERROR("uknown channel type specified for image");
    }

} /* sizeOfType */

//! \brief read function wrapper around an istream.
static void readData (png_struct *pngPtr, png_bytep data, png_size_t length)
{
    std::istream *inS = reinterpret_cast<std::istream*>(png_get_io_ptr(pngPtr));
    inS->read(reinterpret_cast<char *>(data), length);
    if (! inS->good()) {
#if ((PNG_LIBPNG_VER_MAJOR == 1) && (PNG_LIBPNG_VER_MINOR < 5))
        longjmp(pngPtr->jmpbuf, 1);
#else
        png_longjmp (pngPtr, 1);
#endif
    }
}

//! \brief helper function to read a PNG image from an input stream
//! \param inS the input stream
//! \param flip true if the rows of the image should be flipped to match OpenGL coordinates
//! \param widOut output variable for the image width
//! \param htOut output variable for the image height (nullptr for 1D images)
//! \param fmtOut output variable for the channel format
//! \param tyOut output variable for the channel representation type
//! \param sRGBOut output variable set to true if the image should be interpreted as sRGB
//! \return a pointer to the image data, or nullptr on error
void *readPNG (
    std::ifstream &inS, bool flip, uint32_t *widOut, uint32_t *htOut,
    Channels *fmtOut, ChannelTy *tyOut, bool *sRGBOut)
{
  /* check PNG signature */
    unsigned char sig[8];
    inS.read (reinterpret_cast<char *>(sig), sizeof(sig));
    if (! inS.good()) {
#ifndef NDEBUG
        std::cerr << "readPNG: I/O error reading header" << std::endl;
#endif
        return nullptr;
    }
    if (png_sig_cmp(sig, 0, 8)) {
#ifndef NDEBUG
        std::cerr << "readPNG: bogus header" << std::endl;
#endif
        return nullptr;
    }

  /* setup read structures */
    png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (pngPtr == nullptr) {
#ifndef NDEBUG
        std::cerr << "readPNG: error creating read_struct" << std::endl;
#endif
        return nullptr;
    }
    png_infop infoPtr = png_create_info_struct(pngPtr);
    if (infoPtr == nullptr) {
#ifndef NDEBUG
        std::cerr << "readPNG: error creating info_struct" << std::endl;
#endif
        png_destroy_read_struct(&pngPtr, nullptr, nullptr);
        return nullptr;
    }
    png_infop endPtr = png_create_info_struct(pngPtr);
    if (!endPtr) {
#ifndef NDEBUG
        std::cerr << "readPNG: error creating info_struct" << std::endl;
#endif
        png_destroy_read_struct (&pngPtr, &infoPtr, nullptr);
        return nullptr;
    }

  /* error handler */
    if (setjmp (png_jmpbuf(pngPtr))) {
#ifndef NDEBUG
        std::cerr << "readPNG: I/O error" << std::endl;
#endif
        png_destroy_read_struct (&pngPtr, &infoPtr, &endPtr);
        return nullptr;
    }

  /* set up input */
    png_set_read_fn (pngPtr, reinterpret_cast<void *>(&inS), readData);

  /* let the PNG library know that we already checked the signature */
    png_set_sig_bytes (pngPtr, 8);

  /* get file info */
    png_uint_32 width, height, bytesPerPixel;
    int bitDepth, colorType;
    png_read_info (pngPtr, infoPtr);
    png_get_IHDR (pngPtr, infoPtr, &width, &height,
        &bitDepth, &colorType, 0 /* interlace type */,
        0 /* compression type */, 0 /* filter method */);

    Channels fmt;
    ChannelTy ty;
    bool sRGB = false;
    switch (colorType) {
      case PNG_COLOR_TYPE_GRAY:
        fmt = Channels::R;
        ty = ChannelTy::U8;
        bytesPerPixel = 1;
        if (bitDepth < 8) {
            png_set_expand_gray_1_2_4_to_8(pngPtr);
        }
        else if (bitDepth == 16) {
          // PNG files store date in network byte order (big-endian), but the x86 is little-endian
            png_set_swap (pngPtr);
            ty = ChannelTy::U16;
            bytesPerPixel = 2;
        }
        break;
      case PNG_COLOR_TYPE_GRAY_ALPHA:
        fmt = Channels::RG;
        ty = ChannelTy::U8;
        bytesPerPixel = 2;
        if (bitDepth == 16) {
          // PNG files store date in network byte order (big-endian), but the x86 is little-endian
            png_set_swap (pngPtr);
            bytesPerPixel = 4;
            ty = ChannelTy::U16;
        }
        break;
      case PNG_COLOR_TYPE_PALETTE:
        fmt = Channels::RGB;
        ty = ChannelTy::U8;
        bytesPerPixel = 3;
        png_set_palette_to_rgb (pngPtr);
        break;
      case PNG_COLOR_TYPE_RGB:
        fmt = Channels::RGB;
        ty = ChannelTy::U8;
        bytesPerPixel = 3;
        if (bitDepth == 16) {
          // PNG files store data in network byte order (big-endian), but the x86 is little-endian
            png_set_swap (pngPtr);
            bytesPerPixel = 6;
            ty = ChannelTy::U16;
        }
        // assume that any 3-channel color image is sRGB, since figuring this out from the
        // PNG file does not seem reliable
        sRGB = true;
        break;
      case PNG_COLOR_TYPE_RGB_ALPHA:
        fmt = Channels::RGBA;
        ty = ChannelTy::U8;
        bytesPerPixel = 4;
        if (bitDepth == 16) {
          // PNG files store data in network byte order (big-endian), but the x86 is little-endian
            png_set_swap (pngPtr);
            bytesPerPixel = 8;
            ty = ChannelTy::U16;
        }
        // assume that any 3-channel color image is sRGB, since figuring this out from the
        // PNG file does not seem reliable
        sRGB = true;
        break;
      default:
#ifndef NDEBUG
        std::cerr << "unknown color type " << colorType << std::endl;
#endif
        png_destroy_read_struct (&pngPtr, &infoPtr, (png_infopp)0);
        return nullptr;
    }

  /* sanity check the image dimensions: max size is 20k x 20k */
    if ((20*1024 < width) || (20*1024 < height)) {
#ifndef NDEBUG
        std::cerr << "readPNG: image too large" << std::endl;
#endif
        return nullptr;
    }

  /* allocate image data */
    int bytesPerRow = bytesPerPixel * width;
    png_byte *img = (png_bytep) std::malloc (height * bytesPerRow);
    if (img == nullptr) {
#ifndef NDEBUG
        std::cerr << "readPNG: unable to allocate image" << std::endl;
#endif
        png_destroy_read_struct (&pngPtr, &infoPtr, &endPtr);
        return nullptr;
    }

    png_bytepp rowPtrs = new png_bytep[height];
    if (flip) {
      /* setup row pointers so that the texture has OpenGL orientation */
        for (png_uint_32 i = 1;  i <= height;  i++)
            rowPtrs[height - i] = img + (i-1)*bytesPerRow;
    }
    else {
        for (png_uint_32 i = 0;  i < height;  i++)
            rowPtrs[i] = img + i*bytesPerRow;
    }

  /* read the image */
    png_read_image(pngPtr, rowPtrs);

  /* Clean up. */
    png_destroy_read_struct (&pngPtr, &infoPtr, &endPtr);
    delete[] rowPtrs;

    if ((htOut == nullptr) && (height > 1)) {
        width *= height;
    }

    *widOut = width;
    if (htOut != nullptr) *htOut = height;
    *fmtOut = fmt;
    *tyOut = ty;
    if (sRGBOut != nullptr) {
        *sRGBOut = sRGB;
    }

    return img;

} /* readPNG */

//! \brief write function wrapper around an ostream.
static void writeData (png_struct *pngPtr, png_bytep data, png_size_t length)
{
    std::ostream *outS = reinterpret_cast<std::ostream*>(png_get_io_ptr(pngPtr));
    outS->write(reinterpret_cast<char *>(data), length);
    if (! outS->good()) {
#if ((PNG_LIBPNG_VER_MAJOR == 1) && (PNG_LIBPNG_VER_MINOR < 5))
        longjmp(pngPtr->jmpbuf, 1);
#else
        png_longjmp (pngPtr, 1);
#endif
    }
}

//! \brief flush function wrapper around an ostream.
static void flushData (png_struct *pngPtr)
{
    std::ostream *outS = reinterpret_cast<std::ostream*>(png_get_io_ptr(pngPtr));
    outS->flush();
    if (! outS->good()) {
#if ((PNG_LIBPNG_VER_MAJOR == 1) && (PNG_LIBPNG_VER_MINOR < 5))
        longjmp(pngPtr->jmpbuf, 1);
#else
        png_longjmp (pngPtr, 1);
#endif
    }
}

//! \brief helper function to write a PNG file to an output stream
//! \param outS the output stream
//! \param flip true if the rows of the image should be flipped to match OpenGL
//! \param wid the image width
//! \param ht the image height (1 for 1D images)
//! \param fmt the Vulkan image format
//! \param ty the OpenGL pixel type
//! \param data a pointer to the image data
//! \return true if the write is successful; otherwise false on error.
bool writePNG (
    std::ofstream &outS,
    bool flip, uint32_t wid, uint32_t ht,
    Channels fmt, ChannelTy ty, void *data)
{
  /* setup write structures */
    png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (pngPtr == nullptr) {
#ifndef NDEBUG
        std::cerr << "writePNG: error creating write_struct" << std::endl;
#endif
        return false;
    }
    png_infop infoPtr = png_create_info_struct(pngPtr);
    if (infoPtr == nullptr) {
#ifndef NDEBUG
        std::cerr << "writePNG: error creating info_struct" << std::endl;
#endif
        png_destroy_write_struct(&pngPtr, nullptr);
        return false;
    }

  /* error handler */
    if (setjmp (png_jmpbuf(pngPtr))) {
#ifndef NDEBUG
        std::cerr << "writePNG: I/O error" << std::endl;
#endif
        png_destroy_write_struct (&pngPtr, &infoPtr);
        return false;
    }

  /* set up output functions */
    png_set_write_fn (pngPtr, reinterpret_cast<void *>(&outS), writeData, flushData);

  /* determine image properties */
    int nChannels, bitDepth, colorTy, bytesPerRow;
    int transforms = PNG_TRANSFORM_IDENTITY;
    switch (fmt) {
      case Channels::R:
        nChannels = 1;
        colorTy = PNG_COLOR_TYPE_GRAY;
        break;
      case Channels::RG:
        nChannels = 2;
        colorTy = PNG_COLOR_TYPE_GRAY_ALPHA;
        break;
      case Channels::RGB:
        nChannels = 3;
        colorTy = PNG_COLOR_TYPE_RGB;
        break;
      case Channels::BGR:
        nChannels = 3;
        colorTy = PNG_COLOR_TYPE_RGB;
        transforms |= PNG_TRANSFORM_BGR;
        break;
      case Channels::RGBA:
        nChannels = 4;
        colorTy = PNG_COLOR_TYPE_RGB_ALPHA;
        break;
      case Channels::BGRA:
        nChannels = 4;
        colorTy = PNG_COLOR_TYPE_RGB_ALPHA;
        transforms |= PNG_TRANSFORM_BGR;
        break;
      default:
        std::cerr << "writePNG: invalid format " << to_string(fmt) << std::endl;
        return false;
    }
    switch (ty) {
      case ChannelTy::U8:
        bytesPerRow = wid * nChannels;
        bitDepth = 8;
        break;
      case ChannelTy::U16:
      // PNG files store date in network byte order (big-endian), but the x86 is little-endian
        transforms |= PNG_TRANSFORM_SWAP_ENDIAN;
//          png_set_swap (pngPtr);
        bytesPerRow = 2 * wid * nChannels;
        bitDepth = 16;
        break;
/* FIXME: support for other pixel types */
       default:
        std::cerr << "writePNG: unsupported pixel type " << to_string(ty) << std::endl;
        return false;
    }

    png_set_IHDR (pngPtr, infoPtr, wid, ht, bitDepth, colorTy,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_bytep img = reinterpret_cast<png_bytep>(data);
    png_bytepp rowPtrs = new png_bytep[ht];
    if (flip) {
      /* setup row pointers so that the texture has OpenGL orientation */
        for (int i = 1;  i <= ht;  i++)
            rowPtrs[ht - i] = img + (i-1)*bytesPerRow;
    }
    else {
        for (int i = 0;  i < ht;  i++)
            rowPtrs[i] = img + i*bytesPerRow;
    }
    png_set_rows (pngPtr, infoPtr, rowPtrs);

    png_write_png (pngPtr, infoPtr, transforms, nullptr);

  /* Clean up. */
    png_destroy_write_struct (&pngPtr, &infoPtr);
    delete[] rowPtrs;
    outS.flush();

    return true;

} /* writePNG */

namespace __detail {
VkFormat toVkFormat (Channels chans, ChannelTy ty, bool isRGB)
{
    switch (chans) {
    case Channels::R: switch (ty) {
        case ChannelTy::U8: return VK_FORMAT_R8_UINT;
        case ChannelTy::S8: return VK_FORMAT_R8_SINT;
        case ChannelTy::U16: return VK_FORMAT_R16_UINT;
        case ChannelTy::S16: return VK_FORMAT_R8_SINT;
        case ChannelTy::U32: return VK_FORMAT_R32_UINT;
        case ChannelTy::S32: return VK_FORMAT_R32_SINT;
        case ChannelTy::F32: return VK_FORMAT_R32_SFLOAT;
        case ChannelTy::UNKNOWN: ERROR("unknown channel type");
        };
    case Channels::RG: switch (ty) {
        case ChannelTy::U8: return VK_FORMAT_R8G8_UINT;
        case ChannelTy::S8: return VK_FORMAT_R8G8_SINT;
        case ChannelTy::U16: return VK_FORMAT_R16G16_UINT;
        case ChannelTy::S16: return VK_FORMAT_R16G16_SINT;
        case ChannelTy::U32: return VK_FORMAT_R32G32_UINT;
        case ChannelTy::S32: return VK_FORMAT_R32G32_SINT;
        case ChannelTy::F32: return VK_FORMAT_R32G32_SFLOAT;
        case ChannelTy::UNKNOWN: ERROR("unknown channel type");
        };
    case Channels::RGB: switch (ty) {
        case ChannelTy::U8: return (isRGB ? VK_FORMAT_R8G8B8_SRGB : VK_FORMAT_R8G8B8_UNORM);
        case ChannelTy::S8: return VK_FORMAT_R8G8B8_SINT;
        case ChannelTy::U16: return VK_FORMAT_R16G16B16_UINT;
        case ChannelTy::S16: return VK_FORMAT_R16G16B16_SINT;
        case ChannelTy::U32: return VK_FORMAT_R32G32B32_UINT;
        case ChannelTy::S32: return VK_FORMAT_R32G32B32_SINT;
        case ChannelTy::F32: return VK_FORMAT_R32G32B32_SFLOAT;
        case ChannelTy::UNKNOWN: ERROR("unknown channel type");
        };
    case Channels::BGR: switch (ty) {
        case ChannelTy::U8: return (isRGB ? VK_FORMAT_B8G8R8_SRGB : VK_FORMAT_B8G8R8_UNORM);
        case ChannelTy::S8: return VK_FORMAT_B8G8R8_SINT;
        default: ERROR("invalid channel type for BGR");
        };
    case Channels::RGBA: switch (ty) {
        case ChannelTy::U8: return (isRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
        case ChannelTy::S8: return VK_FORMAT_R8G8B8A8_SINT;
        case ChannelTy::U16: return VK_FORMAT_R16G16B16A16_UINT;
        case ChannelTy::S16: return VK_FORMAT_R16G16B16A16_SINT;
        case ChannelTy::U32: return VK_FORMAT_R32G32B32A32_UINT;
        case ChannelTy::S32: return VK_FORMAT_R32G32B32A32_SINT;
        case ChannelTy::F32: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case ChannelTy::UNKNOWN: ERROR("unknown channel type");
        };
    case Channels::BGRA: switch (ty) {
        case ChannelTy::U8: return (isRGB ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM);
        case ChannelTy::S8: return VK_FORMAT_B8G8R8A8_SINT;
        default: ERROR("invalid channel type for BGRA");
        };
    case Channels::UNKNOWN:
        ERROR("toVkFormat: unknown format");
    }
}

/***** virtual base class __detail::ImageBase member functions *****/

ImageBase::ImageBase (uint32_t nd, Channels chans, ChannelTy ty, size_t npixels)
  : _nDims(nd), _chans(chans), _type(ty),
    _nBytes(numChannels(chans) * npixels * sizeOfType(ty))
{
    this->_data = std::malloc(this->_nBytes);
}

ImageBase::~ImageBase ()
{
    if (this->_data != nullptr) {
        std::free(this->_data);
    }
}

unsigned int ImageBase::nChannels () const
{
    return numChannels (this->_chans);
}

size_t ImageBase::nBytesPerPixel () const
{
    return sizeOfType (this->_type);
}

void ImageBase::addAlphaChannel ()
{
    if (this->_chans == Channels::RGB) {
        this->_chans = Channels::RGBA;
    }
    else if (this->_chans != Channels::BGR) {
        this->_chans = Channels::BGRA;
    }
    else {
        return;
    }

    switch (this->_type) {
    case ChannelTy::U8: {
            uint32_t nPixels = this->_nBytes / 3;
            uint8_t *newImg = (uint8_t *) std::malloc (4 * nPixels);
            uint8_t *dstP = newImg;
            uint8_t *srcP = reinterpret_cast<uint8_t *>(this->_data);
            for (int i = 0;  i < nPixels;  ++i) {
                dstP[0] = srcP[0];
                dstP[1] = srcP[1];
                dstP[2] = srcP[2];
                dstP[3] = 0xff;
                dstP += 4;
                srcP += 3;
            }
            std::free(this->_data);
            this->_data = newImg;
            this->_nBytes = 4 * nPixels;
        } break;
    case ChannelTy::U16: {
            uint32_t nPixels = this->_nBytes / 6;
            uint16_t *newImg = (uint16_t *) std::malloc (8 * nPixels);
            uint16_t *dstP = newImg;
            uint16_t *srcP = reinterpret_cast<uint16_t *>(this->_data);
            for (int i = 0;  i < nPixels;  ++i) {
                dstP[0] = srcP[0];
                dstP[1] = srcP[1];
                dstP[2] = srcP[2];
                dstP[3] = 0xffff;
                dstP += 4;
                srcP += 3;
            }
            std::free(this->_data);
            this->_data = newImg;
            this->_nBytes = 8 * nPixels;
        } break;
    default:
        ERROR ("unsupported channel type");
    }

}

} /* namespace __detail */


/***** class Image1D member functions *****/

Image1D::Image1D (uint32_t wid, Channels chans, ChannelTy ty)
    : __detail::ImageBase (1, chans, ty, wid), _wid(wid)
{ }

Image1D::Image1D (std::string const &file)
    : __detail::ImageBase (1)
{
  // open the image file for reading
    std::ifstream inS(file, std::ifstream::in | std::ifstream::binary);
    if (inS.fail()) {
#ifndef NDEBUG
        std::cerr << "Image2D::Image1D: unable to open \"" << file << "\"" << std::endl;
#endif
        exit (1);
    }

    this->_data = readPNG(
        inS, false, &this->_wid, nullptr, &this->_chans, &this->_type, &this->_sRGB);
    if (this->_data == nullptr) {
        inS.close();
        std::cerr << "Image2D::Image1D: unable to load image file \"" << file << "\"" << std::endl;
        exit (1);
    }
    int nChannels = numChannels(this->_chans);
    this->_nBytes = nChannels * this->_wid * sizeOfType(this->_type);

    // because Vulkan prefers 4-channel images
    if (nChannels == 3) {
        this->addAlphaChannel();
    }

    inS.close();
}

// write the image
bool Image1D::write (const char *file)
{
  // open the image file for writing
    std::ofstream outS(file, std::ofstream::out | std::ifstream::binary);
    if (outS.fail()) {
#ifndef NDEBUG
        std::cerr << "Image1D::write: unable to open \"" << file << "\"" << std::endl;
#endif
        return false;
    }

    bool sts = writePNG (outS, false, this->_wid, 1, this->_chans, this->_type, this->_data);

    outS.close();

    return sts;
}


/***** class Image2D member functions *****/

Image2D::Image2D (uint32_t wid, uint32_t ht, Channels chans, ChannelTy ty)
    : __detail::ImageBase (2, chans, ty, wid * ht), _wid(wid), _ht(ht)
{ }

Image2D::Image2D (std::string const &file, bool flip)
    : __detail::ImageBase (2)
{
  // open the image file for reading
    std::ifstream inS(file, std::ifstream::in | std::ifstream::binary);
    if (inS.fail()) {
#ifndef NDEBUG
        std::cerr << "Image2D::Image2D: unable to open \"" << file << "\"" << std::endl;
#endif
        exit (1);
    }

    this->_data = readPNG(
        inS, flip, &this->_wid, &this->_ht, &this->_chans, &this->_type, &this->_sRGB);
    if (this->_data == nullptr) {
        inS.close();
        std::cerr << "Image2D::Image2D: unable to load image file \"" << file << "\"" << std::endl;
        exit (1);
    }
    int nChannels = numChannels(this->_chans);
    this->_nBytes = nChannels * this->_wid * this->_ht * sizeOfType(this->_type);

    inS.close();

    // because Vulkan prefers 4-channel images
    if (nChannels == 3) {
        this->addAlphaChannel();
    }
}

Image2D::Image2D (std::ifstream &inS, bool flip)
    : __detail::ImageBase (2)
{
    this->_data = readPNG(
        inS, flip, &this->_wid, &this->_ht, &this->_chans, &this->_type, &this->_sRGB);
    if (this->_data == nullptr) {
        std::cerr << "Image2D::Image2D: unable to load 2D image" << std::endl;
        exit (1);
    }
    int nChannels = numChannels(this->_chans);
    this->_nBytes = nChannels * this->_wid * this->_ht * sizeOfType(this->_type);

    // because Vulkan prefers 4-channel images
    if (nChannels == 3) {
        this->addAlphaChannel();
    }
}

// write the image to a file
bool Image2D::write (const char *file, bool flip)
{
  // open the image file for writing
    std::ofstream outS(file, std::ofstream::out | std::ifstream::binary);
    if (outS.fail()) {
#ifndef NDEBUG
        std::cerr << "Image2D::write: unable to open \"" << file << "\"" << std::endl;
#endif
        return false;
    }

    bool sts = writePNG (outS, flip, this->_wid, this->_ht, this->_chans, this->_type, this->_data);

    outS.close();

    return sts;
}

// write the image to an output stream
bool Image2D::write (std::ofstream &outS, bool flip)
{
    bool sts = writePNG (outS, flip, this->_wid, this->_ht, this->_chans, this->_type, this->_data);

    return sts;
}

// copy the contents of another image into this image
//
void Image2D::bitblt (Image2D const &src, uint32_t row, uint32_t col)
{
    if ((this->_chans != src._chans) || (this->_type != src._type)) {
        std::cerr << "Image2D::bitblt: incompatible image formats/types\n";
        exit (1);
    }
    if ((this->_wid < col + src._wid) || (this->_ht < row + src._ht)) {
        std::cerr << "Image2D::bitblt: out of range\n";
        exit (1);
    }

    size_t bytesPerPixel = numChannels(this->_chans) * sizeOfType(this->_type);
    size_t stride = bytesPerPixel * this->_wid;
    size_t bytesPerRow = bytesPerPixel * src._wid;
  // upper-left corner of destination rectangle
    char *dstP = reinterpret_cast<char *>(this->_data)
        + stride * row
        + bytesPerPixel * col;
  // upper-left corner of source
    char *srcP = reinterpret_cast<char *>(src._data);
  // copy the data
    for (uint32_t i = 0;  i < src._ht;  i++) {
        std::memcpy (dstP, srcP, bytesPerRow);
        dstP += stride;
        srcP += bytesPerRow;
    }
}

std::string to_string (Channels ch)
{
    switch (ch) {
    case Channels::UNKNOWN: return "UNKNOWN";
    case Channels::R: return "R";
    case Channels::RG: return "RG";
    case Channels::RGB: return "RGB";
    case Channels::BGR: return "BGR";
    case Channels::RGBA: return "RGBA";
    case Channels::BGRA: return "BGRA";
    }
}

std::string to_string (ChannelTy ty)
{
    switch (ty) {
    case ChannelTy::UNKNOWN: return "UNKNOWN";
    case ChannelTy::U8: return "U8";
    case ChannelTy::S8: return "S8";
    case ChannelTy::U16: return "U16";
    case ChannelTy::S16: return "S16";
    case ChannelTy::U32: return "U32";
    case ChannelTy::S32: return "S32";
    case ChannelTy::F32: return "F32";
    }
}

} /* namespace cs237 */
