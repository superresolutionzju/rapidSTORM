#include "debug.h"
#include "NaiveFitter.h"
#include "ModelledFitter.h"
#include "guf/psf/StandardFunction.h"
#include "guf/psf/free_form.h"
#include "guf/psf/fixed_form.h"
#include "guf/psf/expressions.h"
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <dStorm/traits/optics.h>
#include <dStorm/engine/JobInfo.h>
#include <dStorm/engine/InputTraits.h>
#include <nonlinfit/Bind.h>

namespace dStorm {
namespace guf {

template <int Kernels, typename Assignment, typename Lambda>
inline NaiveFitter::Ptr 
create2( const Config& c, const dStorm::engine::JobInfo& i ) 
{ 
    typedef typename PSF::StandardFunction< 
        nonlinfit::Bind< Lambda ,Assignment> ,Kernels>
        ::type F;
    return std::auto_ptr<NaiveFitter>( new ModelledFitter<F>(c,i) );
}

template <>
inline NaiveFitter::Ptr
create2<2,PSF::FreeForm,PSF::No3D>( const Config& c, const dStorm::engine::JobInfo& i )
{
    throw std::runtime_error("Two kernels and free-sigma fitting can't be combined, sorry");
}


template <int Kernels>
NaiveFitter::Ptr
NaiveFitter::create( 
    const Config& c, 
    const dStorm::engine::JobInfo& i )
{
    const threed_info::DepthInfo* d = i.traits.optics(0).depth_info(Direction_X).get();
    if ( c.free_sigmas() && d->provides_3d_info() )
        throw std::runtime_error("Free-sigma fitting is limited to 2D");
    else if ( c.free_sigmas() )
        return create2<Kernels,PSF::FreeForm,PSF::No3D>(c,i);
    else if ( d->provides_3d_info() )
        return create2<Kernels,PSF::FixedForm,PSF::Spline3D>( c, i );
    else
        return create2<Kernels,PSF::FixedForm,PSF::No3D>(c,i);
}

template NaiveFitter::Ptr NaiveFitter::create<1>( const Config& c, const dStorm::engine::JobInfo& i );
template NaiveFitter::Ptr NaiveFitter::create<2>( const Config& c, const dStorm::engine::JobInfo& i );

}
}
