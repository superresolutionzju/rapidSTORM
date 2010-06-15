#ifndef DSTORM_VIEWER_BACKEND_H
#define DSTORM_VIEWER_BACKEND_H

#include <dStorm/output/Output.h>
#include <memory>
#include "Status_decl.h"
#include "Config_decl.h"

namespace dStorm {
namespace viewer {

struct Backend
{
    virtual ~Backend() {}
    virtual output::Output& getForwardOutput() = 0;

    virtual std::auto_ptr<Backend> adapt( 
        std::auto_ptr<Backend> self, Config&, Status& ) = 0;

    virtual void save_image(std::string filename, const Config&) = 0;

    virtual void set_histogram_power(float power) = 0;
    virtual void set_resolution_enhancement(float re) = 0;

};

}
}

#endif
