#include "dStorm/input/FileMethod.h"
#include "inputs/TIFF.h"
#include "dejagnu.h"
#include <dStorm/helpers/thread.h>

int main() {
    ost::DebugStream::set( std::cerr );
    TestState state;
    dStorm::input::FileMethod::unit_test( state );
    dStorm::TIFF::ChainLink::unit_test( state );
}