#include "coordinate.h"
#include "coordinate_config.h"
#include "../Config.h"
#include <simparm/ChoiceEntry_Impl.hh>

namespace dStorm {
namespace viewer {
namespace colour_schemes {

CoordinateConfig::CoordinateConfig() 
: ColourScheme("ByCoordinate", "Vary hue with coordinate value"),
  choice("HueCoordinate", "Coordinate to vary hue with", output::binning::InteractivelyScaledToInterval, "Hue"),
  range("HueRange", "Range of hue", 0.666)
{
    choice.attach_ui( this->node );
    range.attach_ui( this->node );
}

CoordinateConfig::CoordinateConfig(const CoordinateConfig& o) 
: ColourScheme(o), choice(o.choice), range(o.range)
{
    choice.attach_ui( this->node );
    range.attach_ui( this->node );
}

void CoordinateConfig::add_listener( simparm::Listener& l ) {
    choice.add_listener( l );
    l.receive_changes_from( range.value );
}


std::auto_ptr<Backend> CoordinateConfig::make_backend( Config& config, Status& status ) const
{
    return Backend::create< Coordinate >(Coordinate(config.invert(), choice().make_user_scaled_binner(), range()), status);
}

}

template <>
std::auto_ptr<ColourScheme> ColourScheme::config_for<colour_schemes::Coordinate>()
{
    return std::auto_ptr<ColourScheme>(new colour_schemes::CoordinateConfig());
}

}
}
