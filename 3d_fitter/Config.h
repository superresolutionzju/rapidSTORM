#ifndef DSTORM_ENGINE_GAUSS3DFITTERCONFIG_H
#define DSTORM_ENGINE_GAUSS3DFITTERCONFIG_H

#include "Config_decl.h"
#include <simparm/Set.hh>
#include <fitter/MarquardtConfig.h>
#include <fitter/residue_analysis/Config.h>
#include <dStorm/UnitEntries/Nanometre.h>
#include <cs_units/camera/resolution.hpp>
#include "ConstantTypes.h"

namespace dStorm {
namespace gauss_3d_fitter {

template <int Widening>
class Config
: public fitter::MarquardtConfig,
  public fitter::residue_analysis::Config
{
  public:
    Config();
    ~Config();
    void registerNamedEntries();

    dStorm::FloatNanometreEntry z_distance;
    simparm::UnitEntry<typename fitpp::Exponential3D::ConstantTypes<Widening>::ResolutionUnit,float>
        defocus_constant_x, defocus_constant_y;
};

}
}

#endif
