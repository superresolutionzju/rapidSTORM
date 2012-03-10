#include "TIFF.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "debug.h"

#ifdef HAVE_TIFFIO_H

#include <algorithm>
#include <cassert>
#include <errno.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <tiffio.h>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/units/base_units/us/inch.hpp>

#include <simparm/ChoiceEntry_Impl.hh>
#include <simparm/Entry.hh>
#include <simparm/FileEntry.hh>
#include <simparm/Set.hh>
#include <simparm/Structure.hh>
#include <simparm/TriggerEntry.hh>

#include <dStorm/engine/Image.h>
#include <dStorm/Image.h>
#include <dStorm/image/MetaInfo.h>
#include <dStorm/image/slice.h>
#include <dStorm/input/InputMutex.h>
#include <dStorm/input/Source.h>
#include <dStorm/signals/InputFileNameChange.h>

#include "dejagnu.h"
#include "TIFFOperation.h"
#include "OpenFile.h"

using namespace std;

namespace dStorm {
namespace tiff {

using namespace dStorm::input;

/** The Source provides images from a TIFF file.
    *
    *  The TIFF source is parameterized by the output pixel
    *  type, which can be one of unsigned char, unsigned short,
    *  unsigned int and float. The loaded TIFF file must match
    *  this data type exactly; no up- or downsampling is per-
    *  formed. */
class Source : public simparm::Set,
                public input::Source< engine::ImageStack >
{
    typedef engine::StormPixel Pixel;
    typedef engine::ImageStack Image;
    typedef engine::Image2D Plane;
    typedef input::Source<engine::ImageStack> BaseSource;
    typedef typename BaseSource::iterator base_iterator;
    typedef typename BaseSource::TraitsPtr TraitsPtr;

    simparm::Node& node() { return *this; }

    public:
    class iterator;
    Source(boost::shared_ptr<OpenFile> file);
    virtual ~Source();

    base_iterator begin();
    base_iterator end();
    TraitsPtr get_traits(typename BaseSource::Wishes);

    Object& getConfig() { return *this; }
    void dispatch(typename BaseSource::Messages m) { assert( ! m.any() ); }
    typename BaseSource::Capabilities capabilities() const 
        { return typename BaseSource::Capabilities(); }

    private:
    boost::shared_ptr<OpenFile> file;

    static void TIFF_error_handler(const char*, 
        const char *fmt, va_list ap);
    
    void throw_error();
};

/** Config class for Source. Simple config that adds
    *  the sif extension to the input file element. */
class ChainLink
: public input::FileInput<ChainLink,OpenFile>, protected simparm::Listener
{
    public:
    ChainLink();

    ChainLink* clone() const { return new ChainLink(*this); }
    BaseSource* makeSource();

    private:
    simparm::Structure<Config> config;
    friend class input::FileInput<ChainLink,OpenFile>;
    OpenFile* make_file( const std::string& ) const;
    void modify_meta_info( MetaInfo& info );
    simparm::Object& getNode() { return config; }

    protected:
    void operator()(const simparm::Event&);
};


const std::string test_file_name = "special-debug-value-rapidstorm:file.tif";

Source::Source( boost::shared_ptr<OpenFile> file )
: simparm::Set("TIFF", "TIFF image reader"),
  file(file)
{
}

class Source::iterator 
: public boost::iterator_facade<iterator,Image,std::random_access_iterator_tag>
{
    mutable OpenFile* src;
    mutable simparm::Node* msg;
    int directory;
    mutable boost::optional<Image> img;

    void check_params() const;

  public:
    iterator() : src(NULL), msg(NULL) {}
    iterator(Source &s) : src(s.file.get()), msg(&s), directory(0) {}

    Image& dereference() const; 
    bool equal(const iterator& i) const {
        DEBUG( "Comparing " << src << " " << i.src << " " << directory << " " << i.directory );
        return (src == i.src) && (src == NULL || i.src == NULL || directory == i.directory);
    }
    void increment() { 
        DEBUG("Incrementing TIFF iterator " << this);
        src->seek_to_image( *msg, directory);
        img.reset(); 
        bool success = src->next_image( *msg );
        if ( ! success ) 
            src = NULL;
        else {
            ++directory;
        }
    }
    void decrement() { 
        DEBUG("Decrementing TIFF iterator " << this);
        img.reset(); 
        if ( directory == 0 ) 
            src = NULL; 
        else {
            --directory;
            src->seek_to_image( *msg, directory );
        }
    }
    void advance(int n) { 
        DEBUG("Advancing TIFF iterator " << this);
        if (n) {
            img.reset(); 
            directory += n;
            src->seek_to_image(*msg, directory);
        }
    }
    int distance_to(const iterator& i) {
        return i.directory - directory;
    }
};

void Source::iterator::check_params() const
{
}

Source::Image&
Source::iterator::dereference() const
{ 
    DEBUG("Dereferencing TIFF iterator " << this);
    if ( ! img.is_initialized() ) {
        src->seek_to_image(*msg, directory);
        OpenFile::Image three_d = src->read_image( *msg );
        img = engine::ImageStack( directory * camera::frame );
        for (int z = 0; z < three_d.depth_in_pixels(); ++z)
            img->push_back( three_d.slice( 2, z * camera::pixel ) );
    }

    return *img;
}

Source::base_iterator
Source::begin() {
    return base_iterator( iterator(*this) );
}

Source::base_iterator
Source::end() {
    return base_iterator( iterator() );
}

Config::Config()
: simparm::Object("TIFF", "TIFF file"),
  ignore_warnings("IgnoreLibtiffWarnings",
    "Ignore libtiff warnings", true),
  determine_length("DetermineFileLength",
    "Determine length of file", false)
{
}

ChainLink::ChainLink() 
: simparm::Listener( simparm::Event::ValueChanged )
{
    receive_changes_from( config.ignore_warnings.value );
    receive_changes_from( config.determine_length.value );
}

BaseSource*
ChainLink::makeSource()
{
    return new Source( get_file() );
}

Source::TraitsPtr 
Source::get_traits(typename BaseSource::Wishes) {
    simparm::Entry<long> count( "EntryCount", "Number of images in TIFF file", 0 );
    count.editable = false;
    push_back(count);
    DEBUG("Creating traits from file object");
    TraitsPtr rv = TraitsPtr( file->getTraits(true, count).release() );
    DEBUG("Returning traits " << rv.get());
    return rv;
}

Source::~Source() {}

void ChainLink::operator()(const simparm::Event& e) {
    InputMutexGuard lock( global_mutex() );
    republish_traits();
}

void unit_test( TestState& s ) {
    ChainLink l;
    l.publish_meta_info();
    s.testrun( l.current_meta_info().get() 
            && l.current_meta_info()->provides_nothing(),
        "Traits are right for empty file" );
    l.current_meta_info()->get_signal< signals::InputFileNameChange >()( test_file_name );
    s.testrun( l.current_meta_info().get() 
            && ! l.current_meta_info()->provides_nothing(),
        "Traits are right for test file" );
}

void ChainLink::modify_meta_info( MetaInfo& i ) {
    i.accepted_basenames.push_back( make_pair("extension_tif", ".tif") );
    i.accepted_basenames.push_back( make_pair("extension_tiff", ".tiff") );
}
OpenFile* ChainLink::make_file( const std::string& n ) const
{
    DEBUG( "Creating file structure for " << n );
    return new OpenFile( n, config, const_cast<simparm::Node&>(static_cast<const simparm::Node&>(config)) );
}

std::auto_ptr< input::Link > make_input()
{
    return std::auto_ptr< input::Link >( new ChainLink() );
}

}
}

#endif