#include "Temperature.h"
#include "SDK.h"
#include <string.h>
#include "Config.h"
#include "TemperatureMonitor.h"
#include "System.h"

using namespace simparm;
using namespace SDK;

namespace AndorCamera {

using namespace States;
using namespace Phases;

_Temperature::_Temperature() :
  requiredTemperature("TargetTemperature",
                      "Required CCD temperature for acquisition", -65),
  realTemperature("ActualTemperature","Actual CCD temperature value"),
  doCool("Cooling", "Cool CCD", false)
{
    doCool.setUserLevel(Entry::Beginner);
    doCool.viewable = false;

    requiredTemperature.setHelp("Cool the camera to this value before "
                                "starting acquisitions.");
    requiredTemperature.setUserLevel(Entry::Beginner);
    requiredTemperature.setMax(30);
    requiredTemperature.setMin(-100);

    realTemperature.setHelp("This is the actual CCD temperature.");
    realTemperature.setUserLevel(Entry::Beginner);
    realTemperature.setEditable(false);
    realTemperature.viewable = false;
}

Temperature::Temperature(StateMachine& sm, Config &conf)
: Object("Temperature", "Temperature"),
  sm(sm),
  targetTemperature(conf.targetTemperature),
  am_cooling(false)
{
    registerNamedEntries();
}
  
Temperature::Temperature(const Temperature&c)
: Node(c), Object(c),
  _Temperature(c),
  Listener(),
  Node::Callback(),
  sm(c.sm),
  targetTemperature(c.targetTemperature),
  am_cooling(c.am_cooling)
{
    registerNamedEntries();
}

Temperature::~Temperature() 
{
}

void Temperature::registerNamedEntries() 
{
    receive_changes_from( doCool.value );
    receive_changes_from( targetTemperature );

    push_back( targetTemperature );
    push_back( requiredTemperature );
    push_back( realTemperature );
    push_back( doCool );
}

void Temperature::operator()(Node &src, 
    Node::Callback::Cause c, Node *)
 
{
    try {
        if ( c == ValueChanged && &src == &doCool.value )
        {
            static ost::Mutex event_mutex;
            ost::MutexLock lock( event_mutex );
            if ( doCool() ) {
                targetTemperature.editable = false;
                SDK::SetTemperature( targetTemperature() );
                #ifdef NO_COOLER
                std::cerr << "Would cool if I were allowed to\n";
                #else
                CoolerON();
                #endif
                STATUS("Creating temperature monitor");
            } else {
                #ifdef NO_COOLER
                std::cerr << "Would stop cooling if I were allowed to\n";
                #else
                CoolerOFF();
                #endif
                targetTemperature.editable = true;
            }
        }
        else if ( c == ValueChanged && &src == &targetTemperature ) {
            requiredTemperature.setMin( targetTemperature() + 1 );
        }
    } catch (const std::exception&e ) {
        std::cerr << e.what() << "\n";
    }
}

void Temperature::cool() 
{
    /* Make sure the cooling switch is pressed. If not pressed already,
     * nothing happens. */
    doCool = true;

    /* We need to check for the existence of the \c temperatureMonitor
    * here because finalize() stops that process, not stopCooling().
    * Thus, it might still be active if we went through the states
    * Inquired -> Initialized -> Cooled -> Initialized . */

    #ifndef NO_COOLER
    sm.status = "Waiting to reach required acquisition temperature";
    std::pair<bool, float> tstate;
    do {
        tstate = GetTemperatureF();
        System::sleep(100);
    } while ( tstate.first == false &&
              tstate.second > requiredTemperature() );
    sm.status = "Reached required temperature"; 
    #endif
}

void Temperature::warm() 
{
    doCool = false;

    std::pair<bool, float> tstate;
    do {
        tstate = GetTemperatureF();
        realTemperature = tstate.second;
        System::sleep(100);
    } while ( tstate.second < -20 );
    monitor.reset(NULL);
}

void Temperature::controlStateChanged( Phase phase, State from, State to) 

{
    STATUS("Temperature for phase " << phase << " of " << from << " to "
           << to);

    if ( phase == Beginning && to != Initialized ) {
        doCool.editable = false;
    }
    if ( phase == Ending && to == Initialized ) {
        doCool.editable = true;
    }

    if ( (phase == Review && to == Acquiring) ) {
        STATUS("Destructing temperature monitor");
        requiredTemperature.editable = false;
        monitor.reset( NULL );
        STATUS("Destructed temperature monitor");
    }
    if ( (phase == Prepare && from == Acquiring) ||
         (phase == Review && from == Disconnected) ) 
    {
        STATUS("Reconstructing temperature monitor");
        monitor.reset( new TemperatureMonitor(realTemperature) );
        if ( from == Acquiring )
            requiredTemperature.editable = true;
        STATUS("Reconstructing temperature monitor");
    }

    if ( phase == Review && from == Disconnected ) 
    {
        /* Read temperature range */
        std::pair<int,int> temp_range;
        temp_range = GetTemperatureRange();
        targetTemperature.setMin(temp_range.first);
        targetTemperature.setMax(temp_range.second);
        requiredTemperature.setMax(temp_range.second);

        doCool.viewable = true;
    } else if ( phase == Prepare && to == Disconnected ) {
        doCool.viewable = false;
    }

    if ( phase == Transition && to == Disconnected ) {
        warm();
        monitor.reset( NULL );
    } else if ( phase == Transition && to == Acquiring ) {
        cool();
    }
}

}
