#ifndef DSTORM_INPUT_RESOLUTIONSETTER_DECL_H
#define DSTORM_INPUT_RESOLUTIONSETTER_DECL_H

#include <memory>
#include <dStorm/input/chain/Link_decl.h>

class TestState;

namespace dStorm {
namespace input {

namespace resolution {

std::auto_ptr<chain::Link> makeLink();
class Config;
class ChainLink;
template <typename ForwardedType>
class Source;

void unit_test( TestState& );

}
}
}

#endif
