#ifndef DSTORM_STATUS_H
#define DSTORM_STATUS_H

#include "Status_decl.h"
#include "Config.h"
#include <simparm/Entry.hh>
#include <simparm/FileEntry.hh>
#include <simparm/TriggerEntry.hh>
#include <simparm/Structure.hh>
#include <boost/thread/mutex.hpp>
#include <dStorm/Engine.h>

namespace dStorm {
namespace display { class Manager; }
namespace viewer {

struct Status {
    Status(const Config&);
    ~Status();

    Config config;
    simparm::TriggerEntry save;

    boost::mutex mutex;
    display::Manager *manager;
    dStorm::Engine* engine;

    virtual void adapt_to_changed_config() = 0;
    void attach_ui( simparm::Node& name );
    void add_listener( simparm::Listener& );
};

}
}

#endif
