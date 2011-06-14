#ifndef DSTORM_INPUT_ROIFILTER_H
#define DSTORM_INPUT_ROIFILTER_H

#include "debug.h"
#ifdef VERBOSE
#include <boost/units/io.hpp>
#endif
#include <simparm/optional.hh>
#include <simparm/OptionalEntry.hh>
#include <simparm/ChoiceEntry.hh>
#include <simparm/Structure.hh>
#include <dStorm/UnitEntries/FrameEntry.h>
#include <dStorm/units/frame_count.h>
#include <dStorm/input/Source_impl.h>
#include <dStorm/input/chain/MetaInfo.h>
#include <boost/variant/variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <dStorm/input/chain/Context.h>
#include <dStorm/input/chain/Filter.h>
#include <dStorm/localization_file/record.h>
#include <dStorm/Localization.h>
#include <dStorm/Image.h>
#include <dStorm/ImageTraits.h>
#include <dStorm/output/LocalizedImage.h>
#include <dStorm/image/extend.h>
#include <dStorm/image/slice.h>

namespace dStorm {
namespace ROIFilter {

struct Config : public simparm::Object {
    typedef input::chain::DefaultTypes SupportedTypes;

    IntFrameEntry first_frame;
    simparm::OptionalEntry< frame_index > last_frame;
    simparm::ChoiceEntry which_plane;

    Config();
    void registerNamedEntries() { 
        push_back( first_frame ); 
        push_back( last_frame ); 
        push_back( which_plane );
    }
};

struct ImageNumber
: public boost::static_visitor<frame_index>
{
    frame_index operator()( const LocalizationFile::EmptyLine& e )
        { return e.number; }
    frame_index operator()( const Localization& l )
        { return l.frame_number(); }
    template <typename PixelType, int Dim>
    frame_index operator()( const Image<PixelType,Dim>& l )
        { return l.frame_number(); }
    frame_index operator()( const LocalizationFile::Record& r )
        { return boost::apply_visitor(*this, r); }
    frame_index operator()( const output::LocalizedImage& r )
        { return r.forImage; }
};

template <typename Ty>
class Source
: public input::Source<Ty>
{
    const frame_index from;
    const simparm::optional<frame_index> to;
    const boost::optional<int> plane;
    const boost::shared_ptr< input::Source<Ty> > _upstream;
    typedef typename input::Source<Ty>::iterator base_iterator;

    struct _iterator;

    inline bool is_in_range(const Ty& t) const;

  public:
    Source( std::auto_ptr< input::Source<Ty> > upstream,
            frame_index from, simparm::optional<frame_index> to, boost::optional<int> plane)
        : input::Source<Ty>(*upstream, upstream->flags), from(from), to(to),
          plane(plane), _upstream(upstream) {}
    Source* clone() const { return new Source(*this); }

    input::Source<Ty>& upstream() { return *_upstream; }

    base_iterator begin();
    base_iterator end();
    typename input::Source<Ty>::TraitsPtr get_traits()
    {
        typename input::Source<Ty>::TraitsPtr p = _upstream->get_traits();
        frame_index from = std::max( this->from, *p->image_number().range().first );
        if ( p->image_number().range().second.is_set() && to.is_set() ) {
            p->image_number().range().second = std::min(*to, *p->image_number().range().second);
        }
        p->image_number().range().first = from;
        DEBUG("First frame of traits is " << *p->image_number().range().first << ", last frame set is " << p->image_number().range().second.is_set());
        return p;
    }
    void dispatch(typename input::Source<Ty>::Messages m) {
        _upstream->dispatch(m);
    }

};

template <typename Ty>
class Source<Ty>::_iterator 
  : public boost::iterator_adaptor< 
        Source<Ty>::_iterator,
        typename input::Source<Ty>::iterator >
{
    const Source<Ty>& s;
    typedef typename input::Source<Ty>::iterator Base;
    const Base end;
    mutable Ty i;

    friend class boost::iterator_core_access;
    void increment() { ++this->base_reference(); seek_valid(); }

    void select_plane();
    void seek_valid() {
        DEBUG("Seeking valid image");
        while ( this->base() != end && ! s.is_in_range(*this->base()) ) {
            ++this->base_reference();
        }
        DEBUG("Got valid image");
        if ( this->base() != end ) {
            select_plane();
        }
    }

    Ty& dereference() const { return i; }
    
  public:
    explicit _iterator(const Source<Ty>& s, const Base& from, const Base& end)
      : _iterator::iterator_adaptor_(from), s(s), end(end) 
    {
        seek_valid(); 
    }
};

template <>
void Source< dStorm::engine::Image >::_iterator::select_plane()
{
    if ( s.plane.is_initialized() ) {
        i = extend( base()->slice(2, *s.plane * camera::pixel ), engine::Image() );
    } else {
        i = *this->base();
    }
}

template <class Ty>
void Source<Ty>::_iterator::select_plane() { i = *this->base(); }

template <class Ty>
bool Source<Ty>::is_in_range(const Ty& t) const
{
    frame_index f = ImageNumber()(t);
    bool rv = f >= from && (!to.is_set() || f <= *to);
    DEBUG("ROI filter returns " << rv << " for " << f);
    return rv;
}

template <typename Ty>
typename Source<Ty>::base_iterator
Source<Ty>::begin() { 
    return typename Source<Ty>::base_iterator( 
        _iterator( *this, _upstream->begin(), _upstream->end() ) ); 
}

template <typename Ty>
typename Source<Ty>::base_iterator
Source<Ty>::end() 
    { return typename Source<Ty>::base_iterator( 
        _iterator( *this, _upstream->end(), _upstream->end() ) ); }

class ChainLink 
: public input::chain::Filter
{
    typedef input::chain::DefaultVisitor< Config > Visitor;
    friend class input::chain::DelegateToVisitor;

    simparm::Structure<Config> config;
    simparm::Structure<Config>& get_config() { return config; }

  public:
    ChainLink* clone() const { return new ChainLink(*this); }
    simparm::Node& getNode() { return config; }

    Link::AtEnd traits_changed( Link::TraitsRef r, Link* l );
    AtEnd context_changed( ContextRef c, Link* l )
        { Link::context_changed(c,l); return notify_of_context_change(c); }
    input::BaseSource* makeSource();
};

}
}

#endif