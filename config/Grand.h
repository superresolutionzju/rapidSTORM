#ifndef DSTORM_CARCONFIG_H
#define DSTORM_CARCONFIG_H

#include <dStorm/Config.h>
#include <dStorm/output/Config.h>
#include <dStorm/input/Link.h>
#include <memory>
#include <list>
#include <simparm/Set.hh>
#include <simparm/Menu.hh>
#include <simparm/Callback.hh>
#include <simparm/FileEntry.hh>
#include <simparm/Entry.hh>
#include <boost/ptr_container/ptr_list.hpp>

namespace dStorm {
    namespace output { class OutputSource; }

    /** Configuration that summarises all
     *  configuration items offered by the dStorm library. */
    class GrandConfig : public Config
    {
      private:
        friend class EngineChoice;
        class TreeRoot;
        class InputListener;

        simparm::Set car_config;

        std::auto_ptr<TreeRoot> outputRoot;
        std::auto_ptr<input::Link> input;
        input::Link::Connection input_listener;

        void traits_changed( boost::shared_ptr<const input::MetaInfo> );
        void create_input( std::auto_ptr<input::Link> );

      public:
        GrandConfig();
        GrandConfig(const GrandConfig &c);
        ~GrandConfig();
        GrandConfig *clone() const { return new GrandConfig(*this); }

        void registerNamedEntries( simparm::Node& at );
        void processCommand( std::istream& i ) { car_config.processCommand(i); }
        void send( simparm::Message& m ) { car_config.send(m); }
        void push_back( simparm::Node& run ) { car_config.push_back(run); }
        std::list<std::string> printValues() { return car_config.printValues(); }

        output::OutputSource& outputSource;
        output::Config& outputConfig;

        simparm::Menu helpMenu;
        simparm::Set outputBox;
        simparm::FileEntry configTarget;
        simparm::BoolEntry auto_terminate;
        /** Number of parallel computation threads to run. */
        simparm::Entry<unsigned long> pistonCount;

        void add_input( std::auto_ptr<input::Link>, InsertionPlace );
        void add_spot_finder( std::auto_ptr<engine::spot_finder::Factory> );
        void add_spot_fitter( std::auto_ptr<engine::spot_fitter::Factory> );
        void add_output( std::auto_ptr<output::OutputSource> );

        const input::MetaInfo& get_meta_info() const;
        std::auto_ptr<input::BaseSource> makeSource();

        void all_modules_loaded();
    };
}

#endif
