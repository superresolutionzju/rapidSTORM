#include "engine/Localizations.h"
#include <cassert>
#include <string.h>
#include <fstream>
#include <foreach.h>

#include "Output.h"
#include "engine/Image.h"
#include <data-c++/Vector.h>
#include <data-c++/VectorList.h>
#include <cc++/thread.h>
#include <vector>
#include <map>

using namespace std;
using namespace data_cpp;

namespace dStorm {

Localizations::Localizations(int w, int h, int n) : w(w), h(h), n(n) {
}

Localizations::Localizations(const Localizations& l)
: VectorList<Localization>(l), w(l.w), h(l.h), n(l.n) {}

Localizations::~Localizations() {
    STATUS("Destructed fit list");
}

std::ostream&
operator<<(std::ostream &o, const Localization& loc)
{
    return
        o << loc.x() << " " << loc.y() << " " << loc.getImageNumber() 
            << " " << loc.getStrength() << " " << loc.parabolicity() 
            << "\n";
}

}
