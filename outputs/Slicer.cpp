#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_DSTORM_DOC_CONTEXT_H
#include <dStorm/doc/context.h>
#endif
#include "Slicer.h"
#include <sstream>
#include <stdio.h>

#include <dStorm/outputs/NullOutput.h>

namespace dStorm {
namespace output {

Basename Slicer::fn_for_slice( int i ) const
{
    Basename bn = filename;
    std::stringstream slice_ident;
    slice_ident << i;
    bn.set_variable("slice", slice_ident.str());
    return bn;
}

void Slicer::add_output_clone(int i) {
    boost::shared_ptr<Output> output;
    try {
        source->FilterSource::set_output_file_basename( 
            fn_for_slice(i) );
        output = source->FilterSource::make_output();
        if ( avoid_filenames != NULL ) 
            output->check_for_duplicate_filenames( *avoid_filenames );
    } catch ( const std::exception& e ) {
        std::cerr << "Building output for slice " << i << " failed: "
                << e.what() << "\n";
        output.reset( new dStorm::outputs::NullOutput() );
    }

    std::stringstream name, desc;
    name << "SlicerNode" << i;
    desc << "Slice " << i;
    boost::shared_ptr<simparm::Object> o 
        ( new simparm::Object(name.str(), desc.str()) );

    o->push_back( output->getNode() );
    outputs.replace( i, new Child( output, o ) );
    outputs_choice.push_back( *o );

    if ( announcement.get() != NULL )
        outputs[i]->announceStormSize(*announcement);
    if ( run_announcement.get() != NULL )
        outputs[i]->announce_run(*run_announcement);
}

Slicer::_Config::_Config() 
: simparm::Object("Slicer", "Slice localization set"),
  slice_size("SliceSize", "Size of one slice in images", 500 * camera::frame),
  slice_distance("SliceDistance", "Start new slice every n images", 100 * camera::frame),
  outputFile("BaseFileName", "File name pattern", "_$slice$")
{
    slice_size.min = 1 * camera::frame;

    slice_distance.min = 1 * camera::frame;

#ifdef HAVE_DSTORM_DOC_CONTEXT_H
    slice_size.helpID = HELP_Slicer_Size;
    slice_distance.helpID = HELP_Slicer_Dist;
    outputFile.helpID = HELP_Slicer_Pattern;
#endif
    outputFile.setHelp("$slice$ is replaced with the block name.");
}

void Slicer::_Config::registerNamedEntries()
{
    push_back( slice_size ); 
    push_back( slice_distance );
    push_back( outputFile); 
}

Slicer::Slicer(const SourceBuilder& config)
: OutputObject("Slicer", "Object Slicer"),
  slice_size( config.slice_size()  ),
  slice_distance( config.slice_distance() ),
  filename( config.outputFile.get_basename() ),
  source( static_cast<const FilterSource&>(config).clone() ),
  avoid_filenames(NULL),
  outputs_choice( "Outputs", "Outputs to slicer" )
{
    outputs.resize(1, NULL);
    add_output_clone(0);
    push_back( outputs_choice );
}

void Slicer::check_for_duplicate_filenames
            (std::set<std::string>& present_filenames)
{
    avoid_filenames = &present_filenames;
}

Output::AdditionalData
Slicer::announceStormSize(const Announcement& a)
{
    announcement.reset( new Announcement(a) );

    return outputs[0]->announceStormSize(a);
}

Output::RunRequirements
Slicer::announce_run(const RunAnnouncement& a)
{
    run_announcement.reset( new RunAnnouncement(a) );

    return outputs[0]->announce_run(a);
}

void Slicer::propagate_signal(ProgressSignal s) {
    received_signals.push_back(s);
    for (unsigned int i = 0; i < outputs.size(); i++)
        if (!outputs.is_null(i)) {
            if ( s == Engine_is_restarted )
                outputs[i].images_in_output = 0;
            outputs[i]->propagate_signal(s);
        }
}

Output::Result Slicer::receiveLocalizations(const EngineResult& er)
{
    frame_count one_frame( 1 * camera::frame );
    frame_index
        cur_image = er.forImage, 
        back_image = (cur_image >= slice_size) 
                ? (cur_image - (slice_size-one_frame))
                : 0;

    int first_slice = back_image / slice_distance;
    if ( back_image.value() % slice_distance.value()
            != 0 ) 
        first_slice += 1;
    int last_slice = cur_image / slice_distance;
    
    while ( int( outputs.size() ) <= last_slice )
        outputs.push_back( NULL );

    for (int i = first_slice; i <= last_slice; i++) {
        if ( outputs.is_null(i) ) add_output_clone(i);
        outputs[i].images_in_output += one_frame;
        outputs[i]->receiveLocalizations(er);
        if ( outputs[i].images_in_output == slice_size ) {
            outputs[i]->propagate_signal( Engine_run_succeeded );
            outputs[i]->propagate_signal( Job_finished_successfully );
            outputs[i]->propagate_signal( Prepare_destruction );
            outputs.replace(i, NULL);
        }
    }

    return KeepRunning;
}

Slicer::~Slicer() 
{
}

Slicer* Slicer::clone() const { return new Slicer(*this); }

}
}
