#include "debug.h"
#include <boost/units/io.hpp>
#include <sstream>
#include <dStorm/output/Traits.h>

#include "localization_field_impl.h"
#include "unknown_field.h"
#include "children_field.h"

namespace dStorm {
namespace localization_file {

using namespace boost::fusion;


const std::string type_string<float>::ident ()
    { return "floating point with . for decimals and "
      "optional scientific e-notation"; }

const std::string type_string<double>::ident()
    { return type_string<float>::ident(); }

const std::string type_string<int>::ident()
    { return "integer"; }

const std::string str(int i) {
    std::stringstream ss;
    ss << i;
    return ss.str();
}

template <int Index>
Field* Field::create_interface( const TiXmlElement& node, input::Traits<Localization>& traits )
{
    typedef typename result_of::value_at<Localization, boost::mpl::int_<Index> >::type::Traits Traits;
    Field::Ptr p = LocalizationField< Index >::try_to_parse(node, traits);
    if ( p.get() )
        return p.release();
    else
        return create_interface<Index+1>( node, traits );
}

template <>
Field* Field::create_interface<Localization::Fields::Count>( const TiXmlElement& node, input::Traits<Localization>& )
{
    const char* syntax_attrib = node.Attribute("syntax");
    if ( syntax_attrib == NULL )
        throw std::runtime_error("Field is missing "
            "syntax attribute.");
    std::string syntax = syntax_attrib;

    if ( syntax == type_string<int>::ident() )
        return new Unknown<int>();
    else if ( syntax == type_string<double>::ident() )
        return new Unknown<double>();
    else if ( syntax == type_string<float>::ident() )
        return new Unknown<float>();
    else
        throw std::runtime_error("Unknown syntax " + syntax + " in localization file field.");
}

Field::Ptr 
Field::parse(const TiXmlNode& node, input::Traits<Localization>& traits)
{
    const TiXmlElement* element = dynamic_cast<const TiXmlElement*>(&node);
    if ( ! element )
        return Field::Ptr();
    else if ( element->Value() == std::string("field") )
        return Ptr( create_interface<0>( *element, traits ) );
    else if ( element->Value() == std::string("localizations") )
        return Ptr( new ChildrenField( *element, traits ) );
    else
        return Ptr(NULL);
}

template <int Field>
static void create_localization_fields_recursive( const input::Traits<Localization>& traits, Field::Fields& result );

template <>
void create_localization_fields_recursive<Localization::Fields::Count>(const Field::Traits&, Field::Fields&) {
}

template <int LField>
void create_localization_fields_recursive( const input::Traits<Localization>& traits, Field::Fields& result )
{
    boost::ptr_vector<Field> my_nodes = LocalizationField<LField>::make_nodes(traits);
    result.transfer( result.end(), my_nodes );
    create_localization_fields_recursive<LField+1>(traits, result);
}

void create_localization_fields( const Field::Traits& traits, Field::Fields& result )
{
    create_localization_fields_recursive<0>( traits, result );
}

std::auto_ptr<Field> Field::construct( const Field::Traits& traits )
{
    return std::auto_ptr<Field>( new ChildrenField(traits, 0) );
}

template Field::Ptr create_localization_field<0>(int,int,bool);
template Field::Ptr create_localization_field<1>(int,int,bool);
template Field::Ptr create_localization_field<2>(int,int,bool);

}
}

namespace boost {
namespace units {

template <>
std::string name_string( const quantity<si::dimensionless, float>& ) {
    return "dimensionless";
}

}
}