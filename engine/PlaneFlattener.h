#ifndef DSTORM_ENGINE_PLANE_JOINER_H
#define DSTORM_ENGINE_PLANE_JOINER_H

#include <dStorm/engine/Image.h>
#include <dStorm/ImageTraits.h>

namespace dStorm {
namespace engine {

class PlaneFlattener {
    traits::Optics<3> optics;
    Image2D buffer;
    Eigen::Matrix< float, 3, Eigen::Dynamic > coordinates;
  public:
    PlaneFlattener( const dStorm::engine::InputTraits& );
    const Image2D flatten_image( const engine::Image& multiplane );
};

}
}

#endif
