#ifndef DSTORM_LOCALIZATIONCOUNTER_H
#define DSTORM_LOCALIZATIONCOUNTER_H

#include <dStorm/output/Output.h>
#include <simparm/NumericEntry.hh>
#include <iostream>
#include <memory>
#include <dStorm/output/OutputBuilder.h>
#include <dStorm/units/frame_count.h>

namespace dStorm {
namespace output {
    class LocalizationCounter : public OutputObject {
      private:
        ost::Mutex mutex;
      	int count;
        frame_count last_config_update, config_increment;
        simparm::UnsignedLongEntry update;
        std::ostream* printAtStop;

        /** Copy constructor not implemented. */
        LocalizationCounter(const LocalizationCounter&);
        /** Assignment not implemented. */
        LocalizationCounter& operator=(const LocalizationCounter&);

      public:
        struct Config : public simparm::Object { 
            Config(); 
            bool can_work_with(Capabilities) { return true; }
        };
        typedef OutputBuilder<LocalizationCounter> Source;

        LocalizationCounter(const Config &);
        LocalizationCounter* clone() const 
            { throw std::runtime_error("LC::clone Not implemented."); }

        AdditionalData announceStormSize(const Announcement &a) {
            ost::MutexLock lock(mutex);

            update.setUserLevel(simparm::Entry::Beginner);
            push_back(update);
            config_increment = 10 * cs_units::camera::frame;

            count = 0; 
            return AdditionalData();
        }
        Result receiveLocalizations(const EngineResult& er) {
            ost::MutexLock lock(mutex);
            count += er.number; 
            if ( last_config_update + config_increment < er.forImage )
            {
                update = count;
                last_config_update = er.forImage;
            }
            return KeepRunning; 
        }
        void propagate_signal(ProgressSignal s) {
            ost::MutexLock lock(mutex);
            if ( s == Engine_is_restarted ) {
                count = 0; 
                update = 0; 
                last_config_update = 0;
            } else if ( s == Engine_run_succeeded ) {
                update = count;
            } else if ( s == Job_finished_successfully ) {
                if (!this->isActive()) std::cout << count << "\n"; 
            }
        }

    };
}
}
#endif
