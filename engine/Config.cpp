#include "debug.h"
#include "Config.h"
#include <math.h>
#include <limits>
#include <dStorm/engine/SpotFinder.h>
#include <dStorm/engine/SpotFitter.h>
#include <dStorm/engine/SpotFitterFactory.h>

#include <simparm/ChoiceEntry_Impl.hh>
#include <simparm/Entry_Impl.hh>
#include <dStorm/output/Basename.h>
#include <boost/bind/bind.hpp>

#include "config.h"

namespace dStorm {
namespace engine {

_Config::_Config()
:   Set("rapidSTORM", "rapidSTORM engine"),
    nms_x("NonMaximumSuppressionX", "Minimum spot distance in X", 3 * camera::pixel),
    nms_y("NonMaximumSuppressionY", "Minimum spot distance in Y", 3 * camera::pixel),
    maskSizeFactor("MaskSizeFactor", "Proportionality factor "
                    "for smoothing and NMS masks", 1.5),
    fitSizeFactor("FitSizeFactor", "Proportionality factor for fit "
                    "window size", 3),
    spotFindingMethod("SpotFindingMethod", "Spot finding method"),
    weights("SpotFindingWeights", "Spot finding weights"),
    spotFittingMethod("SpotFittingMethod", "Spot fitting method"),
    motivation("Motivation", "Spot search eagerness", 3),
    amplitude_threshold("AmplitudeThreshold", "Amplitude discarding threshold")
{
    DEBUG("Building dStorm Config");

    nms_x.setUserLevel(Object::Intermediate);
    nms_y.setUserLevel(Object::Intermediate);
    
    maskSizeFactor.setUserLevel(Object::Expert);
    fitSizeFactor.setUserLevel(Object::Expert);

    motivation.setHelp("Abort spot search when this many successive "
                        "bad candidates are found.");
    motivation.setUserLevel(Object::Intermediate);

    amplitude_threshold.setUserLevel(Object::Beginner);
    amplitude_threshold.setHelp("Every fit attempt with an amplitude higher "
                                "than this threshold will be considered a "
                                "localization, and those below the threshold "
                                "are discarded immediately. Compared with the "
                                "other amplitude threshold in the Viewer, this "
                                "threshold is already enforced during computation,"
                                "thus saving computation time and avoiding false "
                                "positives; however, contrary to the other threshold, "
                                "it's application is not reversible.");

    amplitude_threshold.helpID = "#AmplitudeThreshold";
    spotFindingMethod.helpID = "#Smoother";
    spotFindingMethod.userLevel = simparm::Object::Intermediate;
    spotFittingMethod.userLevel = simparm::Object::Intermediate;

    spotFindingMethod.set_auto_selection( true );

    spotFittingMethod.set_auto_selection( true );
}

_Config::~_Config() {
    /* No special stuff here. Declared in this file to avoid necessity
     * of including simparm::ChoiceEntry implementation when using
     * Config. */
}

void _Config::registerNamedEntries() {
    push_back(nms_x);
    push_back(nms_y);
    push_back(amplitude_threshold);

    push_back(maskSizeFactor);
    push_back(fitSizeFactor);
    push_back(spotFindingMethod);
    push_back( weights );
    std::for_each( spot_finder_weights.begin(), spot_finder_weights.end(),
        boost::bind( &simparm::Node::push_back, boost::ref(weights), _1 ) );
    push_back(spotFittingMethod);

    push_back(motivation);

}


Config::Config() 
{ 
    registerNamedEntries();
}

Config::Config(const Config& c) 
: _Config(c)
{ 
    registerNamedEntries();
}

void _Config::set_variables( output::Basename& bn ) const
{
    std::stringstream ss;
    if ( amplitude_threshold().is_initialized() ) 
        ss << (*amplitude_threshold()).value();
    else
        ss << "auto";
    bn.set_variable("thres", ss.str());

    spotFindingMethod().set_variables( bn );
    spotFittingMethod().set_variables( bn );
}

}
}
