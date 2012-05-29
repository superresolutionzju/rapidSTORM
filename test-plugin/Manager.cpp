#include "Manager.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "debug.h"

#include <boost/foreach.hpp>
#include <simparm/Message.h>
#include "Manager.h"
#include "Window.h"
#include <dStorm/display/SharedDataSource.h>

namespace Eigen {

template <> class NumTraits<dStorm::Pixel>
    : public NumTraits<int> {};

}

namespace dStorm {
namespace text_stream_ui {

class Manager::Handle 
: public dStorm::display::WindowHandle {
    Manager& m;
    boost::shared_ptr<display::SharedDataSource> data_source;
    boost::shared_ptr<Window> window;

  public:
    Handle( 
        const display::WindowProperties& properties,
        display::DataSource& source,
        Manager& m ) 
    : m(m), data_source( new display::SharedDataSource(source) ),
      window( new Window(m, properties, data_source, m.number++) )
    {
        window->attach_ui( m.current_ui );
        boost::lock_guard< boost::mutex > lock( m.source_list_mutex );
        m.sources_queue.push_back( window );
    }
    ~Handle() {
        data_source->disconnect();
    }

    void store_current_display( dStorm::display::SaveRequest );
};

void Manager::run() { 
    dispatch_events(); 
}

std::auto_ptr<dStorm::display::WindowHandle>
    Manager::register_data_source
(
    const display::WindowProperties& props,
    dStorm::display::DataSource& handler
) {
    if ( !running ) {
        running = true;
        ui_thread = boost::thread( &Manager::run, this );
    }
    return 
        std::auto_ptr<dStorm::display::WindowHandle>( new Handle(props, handler, *this) );
}

std::ostream& operator<<(std::ostream& o, dStorm::display::Color c)
{
    return ( o << int(c.red()) << " " << int(c.green()) << " " << int(c.blue()) );
}

void Manager::dispatch_events() {
    while ( running ) {
        boost::shared_ptr<Window> s = next_source();
        if ( s ) s->update_window();
        heed_requests();
        gui_run.notify_all(); 
#ifdef HAVE_USLEEP
        usleep(100000);
#elif HAVE_WINDOWS_H
        Sleep(100);
#endif
        DEBUG("Getting mutex for event loop");
        DEBUG("Got mutex for event loop");
    }
    DEBUG("Event loop ended, heeding last save requests");
    heed_requests();
    DEBUG("Finished event loop");
}

Manager::Manager()
: running(false),
  number(0),
  master_object("DummyDisplayManagerConfig", "Dummy display manager"),
  ui_is_handled_by_wxWidgets("wxWidgetsUI", "Let wxWidgets handle the UI", true)
{
    master_object.set_user_level( simparm::Debug );
    ui_is_handled_by_wxWidgets.set_user_level( simparm::Debug );
}

Manager::~Manager()
{
    if ( running ) {
        running = false;
        ui_thread.join();
    }
}

void Manager::Handle::store_current_display( dStorm::display::SaveRequest s )
{
    DEBUG("Got store request to " << s.filename);
    m.request_action( boost::bind( &Window::save_window, window, s ) );
    DEBUG("Saved store request");
}

void Manager::attach_ui( simparm::NodeHandle at )
{
    current_ui = master_object.attach_ui( at );
    ui_is_handled_by_wxWidgets.attach_ui( current_ui );
}

void Manager::heed_requests() {
    Requests my_requests;
    {
        boost::lock_guard<boost::mutex> lock(request_mutex);
        std::swap( requests, my_requests );
    }
    BOOST_FOREACH( Requests::value_type& i, my_requests )
        i();
}

void Manager::request_action( boost::function0<void> request ) {
    if ( boost::this_thread::get_id() == ui_thread.get_id() ) {
        request();
    } else {
        boost::lock_guard<boost::mutex> lock(request_mutex);
        requests.push_back( request );
    }
}

boost::shared_ptr<Window> Manager::next_source() {
    boost::lock_guard< boost::mutex > lock( source_list_mutex );
    if ( sources_queue.empty() ) return boost::shared_ptr<Window>();
    boost::shared_ptr<Window> s = sources_queue.front();
    sources_queue.erase( sources_queue.begin() );
    sources_queue.push_back( s );
    return s;
}

void Manager::remove_window_from_event_queue( boost::shared_ptr<Window> w ) {
    boost::lock_guard< boost::mutex > lock( source_list_mutex );
    sources_queue.erase( std::remove( sources_queue.begin(), sources_queue.end(), w ), sources_queue.end() );
}

}

}
