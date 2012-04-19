#ifndef DSTORM_THREED_INFO_SPLINE_H
#define DSTORM_THREED_INFO_SPLINE_H

#include <vector>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/shared_array.hpp>
#include <boost/optional/optional.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/length.hpp>
#include <dStorm/Direction.h>
#include <gsl/gsl_interp.h>

#include <Eigen/Core>

#include <dStorm/Localization_decl.h>
#include "types.h"
#include "DepthInfo.h"

namespace dStorm {
namespace threed_info {

using boost::units::quantity;
namespace si = boost::units::si;

class SplineFactory;

class Spline3D : public DepthInfo {
public:
    Spline3D( const SplineFactory& );

private:
    const int N;
    friend class SplineFactory;
    struct Point {
        ZPosition z;
        Sigma sigma;

    public:
        Point( ZPosition z, Sigma sigma ) : z(z), sigma(sigma) {}
        static double to_x( ZPosition z ) { return z / (1E-6f * si::meter); }
        double x() const { return to_x(z); }
        double y() const { return sigma / (1E-6f * si::meter); }
        static ZPosition from_x( double x ) { return float(x * 1E-6) * si::meter; }
        static Sigma from_y( double y ) { return float(y * 1E-6) * si::meter; }
    };
    const std::vector<Point> points;
    const ZPosition h;
    Eigen::MatrixXd coeffs;
    ZPosition equifocal_plane__;

    std::string config_name_() const { return "Spline3D"; }
    Sigma get_sigma_( ZPosition z ) const;
    SigmaDerivative get_sigma_deriv_( ZPosition z ) const;
    ZRange z_range_() const;
    ZPosition equifocal_plane_() const { return equifocal_plane__; }
    std::ostream& print_( std::ostream& o ) const 
        { return o << "spline 3D information"; }
    bool provides_3d_info_() const { return true; }
};

class SplineFactory {
public:
    SplineFactory( const std::string& file, Direction dir );

    static SplineFactory Mock( Direction d );

private:
    SplineFactory() {}
    friend class Spline3D;
    typedef std::vector< Spline3D::Point > Points;
    Points points;
};

}
}

#endif
