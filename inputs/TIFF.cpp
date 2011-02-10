#define DSTORM_TIFFLOADER_CPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_TIFFIO_H

#include "debug.h"

#include <stdexcept>
#include <cassert>
#include <errno.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <stdint.h>

#include <simparm/ChoiceEntry_Impl.hh>

#include <dStorm/Image.h>
#include <tiffio.h>

#include "TIFF.h"
#include <dStorm/input/Source.h>
#include <dStorm/input/Source_impl.h>
#include <dStorm/engine/Image.h>
#include <dStorm/ImageTraits.h>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/units/base_units/us/inch.hpp>

#include "TIFFOperation.h"
#include <dStorm/input/chain/FileContext.h>
#include <dStorm/input/InputMutex.h>

using namespace std;

namespace dStorm {
namespace TIFF {

template<typename Pixel, int Dimensions>
Source<Pixel,Dimensions>::Source( boost::shared_ptr<OpenFile> file )
: simparm::Set("TIFF", "TIFF image reader"),
  BaseSource( static_cast<simparm::Node&>(*this),    
      Flags() ),
  file(file)
{
}

template<typename Pixel, int Dimensions>
class Source<Pixel,Dimensions>::iterator 
: public boost::iterator_facade<iterator,Image,std::random_access_iterator_tag>
{
    mutable OpenFile* src;
    mutable simparm::Node* msg;
    int directory;
    mutable Image img;

    void go_to_position() const {
        TIFFOperation op( "in reading TIFF file",
                          *msg, src->ignore_warnings );
        int rv = TIFFSetDirectory(src->tiff, directory);
        if ( rv == 1 ) {
            src->current_directory = directory;
        } else {
            op.throw_exception_for_errors();
        }
    }
    void check_position() const {
        if ( src->current_directory != directory )
            go_to_position();
    }
    void check_params() const;

  public:
    iterator() : src(NULL), msg(NULL) {}
    iterator(Source &s) : src(s.file.get()), msg(&s), directory(0) {}

    Image& dereference() const; 
    bool equal(const iterator& i) const {
        DEBUG( "Comparing " << src << " " << i.src << " " << directory << " " << i.directory );
        return (src == i.src) && (src == NULL || directory == i.directory);
    }
    void increment() { 
        TIFFOperation op( "in reading TIFF file",
                          *msg, src->ignore_warnings );
        check_position();
        img.invalidate(); 
        if ( TIFFReadDirectory(src->tiff) != 1 ) {
            op.throw_exception_for_errors();
            /* Code from here only executed when no error
             * was encountered */
            DEBUG( "Setting iterator to NULL" );
            src = NULL;
        } else {
            directory++; 
            src->current_directory = directory;
        }
    }
    void decrement() { 
        img.invalidate(); 
        if ( directory == 0 ) 
            src = NULL; 
        else {
            --directory;
            go_to_position();
        }
    }
    void advance(int n) { 
        if (n) {
            img.invalidate(); 
            directory += n;
            go_to_position();
        }
    }
    int distance_to(const iterator& i) {
        return i.directory - directory;
    }
};

template<typename Pixel, int Dimensions>
void
Source<Pixel,Dimensions>::iterator::check_params() const
{
    ::TIFF *tiff = src->tiff;
    uint32_t width, height, depth;
    uint16_t bitspersample;
    TIFFGetField( tiff, TIFFTAG_IMAGEWIDTH, &width );
    TIFFGetField( tiff, TIFFTAG_IMAGELENGTH, &height );
    if ( ! TIFFGetField( tiff, TIFFTAG_IMAGEDEPTH, &depth ) )
        depth = 1;
    TIFFGetField( tiff, TIFFTAG_BITSPERSAMPLE, &bitspersample );

    if ( int(width) != src->size[0] || int(height) != src->size[1] || int(depth) != src->size[2] ) {
        std::stringstream error;
        error << "TIFF image no. " << TIFFCurrentDirectory(tiff)
            << " has dimensions (" << width << ", " << height << ", " << depth
            << ") different from first image (" << src->size[0]
            << ", " << src->size[1] << ", " << src->size[2] << "). Aborting.";
        throw std::runtime_error( error.str() );
    }
    if ( bitspersample != sizeof(Pixel)*8 ) {
        std::stringstream error;
        error << "TIFF image no. " << TIFFCurrentDirectory(tiff) << " has "
                << bitspersample << " bits per pixel, but "
                << sizeof(Pixel)*8 << " bits are necessary.";
        throw std::runtime_error( error.str() );
    }
}

template<typename Pixel, int Dim>
typename Source<Pixel,Dim>::Image&
Source<Pixel,Dim>::iterator::dereference() const
{ 
    if ( img.is_invalid() ) {
        TIFFOperation op( "in reading TIFF file",
                          *msg, src->ignore_warnings );
        check_position();
        check_params();

        typename Image::Size sz;
        ::TIFF *tiff = src->tiff;
        tsize_t strip_size = TIFFStripSize( tiff );
        tstrip_t strip_count = TIFFNumberOfStrips( tiff );
        assert( sz.rows() <= 3 );
        for (int i = 0; i < sz.rows(); ++i)
            sz[i] = src->size[i] * camera::pixel;
        Image i( sz, directory * camera::frame);

        DEBUG("Reading image " << directory << " of size " << i.size() << " from TIFF with " << strip_count << " strips with " << strip_size / sizeof(Pixel) << " pixels each for an image sized " << src->_width << " " << src->_height);

        Pixel* end_of_data = i.ptr() + i.size_in_pixels();
        for (tstrip_t strip = 0; strip < strip_count; strip++) {
            Pixel *data = i.ptr() + (strip * strip_size / sizeof(Pixel));
            tsize_t remaining_space = sizeof(Pixel) * (end_of_data - data);
            TIFFReadEncodedStrip( tiff, strip, data,
                std::min( remaining_space, strip_size ) );
        }
        img = i;

        op.throw_exception_for_errors();
    }

    return img;
}

template<typename Pixel, int Dim>
typename Source<Pixel,Dim>::base_iterator
Source<Pixel,Dim>::begin() {
    return base_iterator( iterator(*this) );
}

template<typename Pixel, int Dim>
typename Source<Pixel,Dim>::base_iterator
Source<Pixel,Dim>::end() {
    return base_iterator( iterator() );
}

Config::Config()
: simparm::Object("TIFF", "TIFF file"),
  ignore_warnings("IgnoreLibtiffWarnings",
    "Ignore libtiff warnings", false),
  determine_length("DetermineFileLength",
    "Determine length of file", false)
{
    ignore_warnings.userLevel 
        = simparm::Object::Intermediate;
}

ChainLink::ChainLink() 
: simparm::Listener( simparm::Event::ValueChanged )
{
    receive_changes_from( config.ignore_warnings.value );
    receive_changes_from( config.determine_length.value );
}

ChainLink::ChainLink(const ChainLink& o) 
: chain::Terminus(o), 
  simparm::Listener( simparm::Event::ValueChanged ),
  config(o.config), file(o.file)
{
    receive_changes_from( config.ignore_warnings.value );
    receive_changes_from( config.determine_length.value );
}

BaseSource*
ChainLink::makeSource()
{
    return new Source<engine::Image::Pixel,engine::Image::Dim>( file );
}

ChainLink::AtEnd
ChainLink::context_changed( ContextRef ocontext, Link* link )
{
    Terminus::context_changed( ocontext, link );

    this->context = boost::dynamic_pointer_cast
        <const input::chain::FileContext, const input::chain::Context>
        ( ocontext );
    if ( ! file.get() || file->for_file() != context->input_file ) {
        open_file();
        /* open_file will have published traits, if necessary */
        return AtEnd();
    } else {
        /* No change, and we have previously published traits
         * since file is not NULL. */
        return AtEnd();
    }
}

template<typename Pixel, int Dimensions>
typename Source<Pixel,Dimensions>::TraitsPtr 
Source<Pixel,Dimensions>::get_traits() {
    return TraitsPtr( file->getTraits<Pixel,Dimensions>().release() );
}

template<typename Pixel, int Dimensions>
Source<Pixel,Dimensions>::~Source() {}

void ChainLink::operator()(const simparm::Event& e) {
    ost::MutexLock lock( global_mutex() );
    if ( file.get() ) {
        open_file();
    }
}

void ChainLink::open_file() {
  file.reset();
  boost::shared_ptr<chain::FileMetaInfo> info;
  if ( context->input_file != "" ) {
    try {
        file.reset( new OpenFile( context->input_file, config, config ) );
        info.reset( new chain::FileMetaInfo() );

        info->set_traits( file->getTraits<engine::Image::Pixel, engine::Image::Dim>().release() );
        info->accepted_basenames.push_back( make_pair("extension_tif", ".tif") );
        info->accepted_basenames.push_back( make_pair("extension_tiff", ".tiff") );
    } catch(...) {
        if ( context->throw_errors )
            throw;
    }
  }
  this->notify_of_trait_change( info );
}

}
}

#endif
