/*! \file cs237-image.hpp
 *
 * Support code for CMSC 23700 Autumn 2022.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2022 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_IMAGE_HPP_
#define _CS237_IMAGE_HPP_

#ifndef _CS237_HPP_
#error "cs237-image.hpp should not be included directly"
#endif

#include <fstream>

namespace cs237 {

//! the channels of an image
enum class Channels {
    UNKNOWN,        //!< unknown
    R,              //!< single-channel image
    RG,             //!< two-channel image
    RGB,            //!< three-channel image in red-green-blue order
    BGR,            //!< three-channel image in blue-green-red order
    RGBA,           //!< four-channel image in red-green-n-blue-alpha order
    BGRA            //!< four-channel image in blue-green-red-alpha order
};

//! convert a Channels value to a printable string
std::string to_string (Channels ch);

//! the type used to represent the channels
enum class ChannelTy {
    UNKNOWN,        //!< unknown type
    U8,             //!< unsigned byte
    S8,             //!< signed byte
    U16,            //!< unsigned 16-bit int
    S16,            //!< signed 16-bit int
    U32,            //!< unsigned 32-bit int
    S32,            //!< signed 32-bit int
    F32             //!< signed 32-bit float
};

//! convert a ChannelTy value to a printable string
std::string to_string (ChannelTy ty);

namespace __detail {

    //! \brief convert an image format and channel type to a Vulkan image format
    VkFormat toVkFormat (Channels chans, ChannelTy ty, bool sRGB);

    class ImageBase {
    public:
        //! the number of dimensions (1, 2, or 3)
        uint32_t nDims () const { return this->_nDims; }
        //! return the format of the pixels.
        Channels channels () const { return this->_chans; }
        //! returns the type of the channels
        ChannelTy type () const { return this->_type; }
        //! return the vulkan format of the image data
        VkFormat format () const { return toVkFormat(this->_chans, this->_type, this->_sRGB); }
        //! the data pointer
        void *data () const { return this->_data; }
        //! the total number of bytes of image data
        size_t nBytes () const { return this->_nBytes; }

        //! the number of channels (1, 2, 3, or 4)
        unsigned int nChannels () const;

        //! the number of bytes per pixel
        size_t nBytesPerPixel () const;

        //! add an opaque alpha channel to the imag
        //!
        //! This operation only works on images with RGB or BGR pixel format;
        //! and is a no-op for other formats.
        //! It is necessary, because many Vulkan implementations do not
        //! support 24-bit pixels.
        void addAlphaChannel ();

    protected:
        uint32_t _nDims;        //!< the number of dimensions (1 or 2)
        Channels _chans;        //!< the texture format
        ChannelTy _type;        //!< the representation type of the data
        bool _sRGB;             //!< should the image be interpreted as an sRGB encoded image?
        size_t _nBytes;         //!< size in bytes of image data
        void *_data;            //!< the raw image data

        explicit ImageBase ()
          : _nDims(0), _chans(Channels::UNKNOWN), _type(ChannelTy::UNKNOWN), _sRGB(false),
            _nBytes(0), _data(nullptr)
        { }
        explicit ImageBase (uint32_t nd)
          : _nDims(nd), _chans(Channels::UNKNOWN), _type(ChannelTy::UNKNOWN), _sRGB(false),
            _nBytes(0), _data(nullptr)
        { }
        explicit ImageBase (uint32_t nd, Channels chans, ChannelTy ty, size_t nPixels);

        virtual ~ImageBase ();

    };

} /* namespace __detail */

/* 1D images */
class Image1D : public __detail::ImageBase {
  public:
  //! create and allocate space for an uninitialized image
  //! \param wid the width of the image
  //! \param chans the image format
  //! \param ty the type of the elements
    Image1D (uint32_t wid, Channels chans, ChannelTy ty);

  //! create and initialize an image from a PNG file.
  //! \param file the name of the PNG file
    Image1D (std::string const &file);

  //! return the width of the image
    size_t width () const { return this->_wid; }

  //! write the texture to a file in PNG format
  //! \param file the name of the PNG file
  //! \return true if successful, false otherwise
    bool write (const char *file);

  private:
    uint32_t _wid;      //!< the width of the image in pixels
};

//! A 2D Image; by default, it is assumed to store color data, which means that
//! RGB data will be interpreted as being sRGB encoded.  Use the `DataImage2D`
//! class to represent unencoded image data.
class Image2D : public __detail::ImageBase {
  public:
  //! create and allocate space for an uninitialized image
  //! \param wid the width of the image
  //! \param ht the height of the image
  //! \param chans the image format
  //! \param ty the type of the elements
    Image2D (uint32_t wid, uint32_t ht, Channels chans, ChannelTy ty);

  //! create and initialize an image from a PNG file.
  //! \param file the name of the PNG file
  //! \param flip set to true if the image should be flipped vertically to match OpenGL
  //!        texture coordinates (default true)
    Image2D (std::string const &file, bool flip = true);

  //! create and initialize an image from a PNG-format input stream
  //! \param inS the input stream
  //! \param flip set to true if the image should be flipped vertically to match OpenGL
  //!        texture coordinates (default true)
    Image2D (std::ifstream &inS, bool flip = true);

  //! return the width of the image
    size_t width () const { return this->_wid; }

  //! return the height of the image
    size_t height () const { return this->_ht; }

  //! write the texture to a file in PNG format
  //! \param file the name of the PNG file
  //! \param flip set to true if the image should be flipped vertically to match standard
  //!        image-file coordinates (default true)
  //! \return true if successful, false otherwise
  //!
  //! Note that the type of the image must be either GL_UNSIGNED_BYTE,
  //! or GL_UNSIGNED_SHORT to write it as a PNG file.
    bool write (const char *file, bool flip = true);

  //! write the texture to an output stream in PNG format
  //! \param outS the output stream to write the PNG image to
  //! \param flip set to true if the image should be flipped vertically to match standard
  //!        image-file coordinates (default true)
  //! \return true if successful, false otherwise
  //!
  //! Note that the type of the image must be either GL_UNSIGNED_BYTE,
  //! or GL_UNSIGNED_SHORT to write it to an output stream.
    bool write (std::ofstream &outS, bool flip = true);

  //! copy the contents of another image into this image
  //! \param src the image to blt into this image
  //! \param row the row of this image where the first row of src is copied
  //! \param col the column of this image where the leftmost column of src is copied
  //!
  //! Note that this function requires that the two images have the same
  //! format and sample type.
    void bitblt (Image2D const &src, uint32_t row, uint32_t col);

  protected:
    uint32_t _wid;      //!< the width of the image in pixels
    uint32_t _ht;       //!< the height of the image in pixels
};

//! A 2D Image used to store 2D data, such as a normal map.
class DataImage2D : public Image2D {
  public:
  //! create and allocate space for an uninitialized image
  //! \param wid the width of the image
  //! \param ht the height of the image
  //! \param chans the image format
  //! \param ty the type of the elements
    DataImage2D (uint32_t wid, uint32_t ht, Channels chans, ChannelTy ty)
      : Image2D (wid, ht, chans, ty)
    {
        this->_sRGB = false;
    }

  //! create and initialize an image from a PNG file.
  //! \param file the name of the PNG file
  //! \param flip set to true if the image should be flipped vertically to match OpenGL
  //!        texture coordinates (default true)
    DataImage2D (std::string const &file, bool flip = true)
      : Image2D (file, flip)
    {
        this->_sRGB = false;
    }

  //! create and initialize an image from a PNG-format input stream
  //! \param inS the input stream
  //! \param flip set to true if the image should be flipped vertically to match OpenGL
  //!        texture coordinates (default true)
    DataImage2D (std::ifstream &inS, bool flip = true)
      : Image2D (inS, flip)
    {
        this->_sRGB = false;
    }

};

} /* namespace cs237 */

#endif /* !_CS237_IMAGE_HPP_ */
