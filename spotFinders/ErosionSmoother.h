#ifndef DSTORM_EROSIONSMOOTHER_H
#define DSTORM_EROSIONSMOOTHER_H

#include <dStorm/data-c++/Vector.h>
#include <dStorm/engine/Image.h>
#include <dStorm/engine/SpotFinder.h>
#include <simparm/Structure.hh>
#include <dStorm/helpers/dilation.h>

namespace dStorm {
    class ErosionSmoother : public engine::SpotFinder {
      private:
        const int mw, mh;
        struct _Config : public simparm::Object {
            void registerNamedEntries() {}
            _Config() : simparm::Object("Erosion", "Erode image") {}
        };
      public:
        typedef simparm::Structure<_Config> Config;
        typedef engine::SpotFinderBuilder<ErosionSmoother> Factory;

        ErosionSmoother (const Config&, const engine::Config &conf,
                         const engine::InputTraits::Size& size) 
            : SpotFinder(conf, size),
              mw(msx+(1-msx%2)), mh(msy+(1-msy%2))
            {}
        ~ErosionSmoother() {}

        void smooth( const engine::Image &in ) {
            rectangular_erosion( in, *smoothed, mw/2, mh/2, 0, 0);
        }
    };
}
#endif