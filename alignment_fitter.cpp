#include <dStorm/localization_file/reader.h>
#include <boost/variant/get.hpp>
#include <gsl/gsl_multimin.h>
#include <boost/units/Eigen/Array>
#include <iomanip>
#include <simparm/Entry_Impl.hh>
#include <simparm/IO.hh>
#include <simparm/Set.hh>
#include <simparm/TriggerEntry.hh>
#include <simparm/command_line.hh>

typedef std::list<Eigen::Vector2f, Eigen::aligned_allocator<Eigen::Vector2f> > PositionList;
typedef std::map< int, boost::array< PositionList, 2 > > ImageMap;
ImageMap images;

struct parameters {
    double sigma;
};

double badness (const gsl_vector * x, void * params) {
    Eigen::Matrix2f rotation = Eigen::Matrix2f::Identity();
    Eigen::Vector2f translation = Eigen::Vector2f::Zero();
    if ( x->size >= 2 ) 
        for (int r = 0; r < 2; ++r)
            translation[r] = gsl_vector_get(x, r);
    if ( x->size >= 4 )
        for (int r = 0; r < 2; ++r)
            rotation(r,r) = gsl_vector_get(x, 2+r);
    if ( x->size >= 6 )
        for (int r = 0; r < 2; ++r)
            rotation(r,1-r) = gsl_vector_get(x, 4+r);

    double score = 0;
    for ( ImageMap::const_iterator i = images.begin(); i != images.end(); ++i ) {
        for ( PositionList::const_iterator b = i->second[1].begin(); b != i->second[1].end(); ++b ) {
            Eigen::Vector2f t = rotation * *b + translation;
            for ( PositionList::const_iterator a = i->second[0].begin(); a != i->second[0].end(); ++a ) {
                score += - exp( - (*a-t).squaredNorm() / static_cast<parameters*>(params)->sigma );
            }
        }
    }
    return score;
}

int main(int argc, char *argv[]) {
    simparm::FileEntry file1("File1", "File 1"), file2("File2", "File 2"), output("OutputFile", "Output file");
    simparm::Entry<double> sigma("Sigma", "Sigma", 1), 
                         shift_x("ShiftX", "Shift X in mum"),
                         shift_y("ShiftY", "Shift Y in mum"),
                         scale_x("ScaleX", "Scale factor X", 1),
                         scale_y("ScaleY", "Scale factor Y", 1),
                         shear_x("ShearX", "Shear factor in X", 0),
                         shear_y("ShearY", "Shear factor Y", 0),
                         target_volume("TargetVolume", "Target estimation volume", 1E-3),
                         cur_step("CurrentStep", "Estimation step"),
                         cur_volume("CurrentVolume", "Current estimation volume");
    simparm::Entry<bool> pre_fit("PreFit", "Pre-fit with simpler models", true);
    simparm::Entry<long> image_count("ImageCount", "Number of images to use", 100);
    simparm::TriggerEntry compute("Compute", "Compute"), twiddler("Twiddler", "Twiddler");

    simparm::Set config("Config", "Matrix generator");
    config.push_back( file1 );
    config.push_back( file2 );
    config.push_back( output );
    config.push_back( sigma );
    config.push_back( shift_x );
    config.push_back( shift_y );
    config.push_back( scale_x );
    config.push_back( scale_y );
    config.push_back( shear_x );
    config.push_back( shear_y );
    config.push_back( pre_fit );
    config.push_back( target_volume );
    config.push_back( image_count );
    config.push_back( compute );
    config.push_back( cur_step );
    config.push_back( cur_volume );
    config.push_back( twiddler );
    twiddler.userLevel = simparm::Object::Debug;
    cur_step.editable = cur_step.viewable = false;
    cur_volume.editable = cur_volume.viewable = false;
    readConfig(config, argc, argv);
    simparm::IO io(NULL, &std::cout);
    io.push_back(config);
    io.desc = "Linear alignment fitter for rapidSTORM files";

    simparm::Entry<double>* entries[6] = { &shift_x, &shift_y, &scale_x, &scale_y, &shear_x, &shear_y };

    while (twiddler.triggered() && std::cin && !io.received_quit_command()) {
      io.processCommand(std::cin);
      if ( ! compute.triggered() ) continue;
      compute.untrigger();
      try {
        dStorm::input::Traits<dStorm::localization::Record> context;
        typedef dStorm::localization_file::Reader::Source Source;
        std::auto_ptr<Source> file_source[2];
        file_source[0] = dStorm::localization_file::Reader::ChainLink::read_file( file1, context );
        file_source[1] = dStorm::localization_file::Reader::ChainLink::read_file( file2, context );

        for (int m = 0; m < 2; ++m)
            for ( dStorm::input::Source<dStorm::localization::Record>::iterator
                    i = file_source[m]->begin(); i != file_source[m]->end(); ++i ) {
                const dStorm::Localization* l = boost::get<dStorm::Localization>(&*i);
                if ( l && l->frame_number().value() >= image_count.value() ) continue;
                if ( l ) { images[ l->frame_number().value() ][m].push_back( 
                            boost::units::value( l->position() ).head<2>() * 1E6 ); }
            }

        parameters params;
        params.sigma = sigma();

        cur_step = 0;
        cur_volume = 1;
        cur_step.viewable = true;
        cur_volume.viewable = true;
        for (int pc = (pre_fit()) ? 2 : 6; pc <= 6; pc += 2) {
            cur_step = pc/2;
            gsl_vector* x = gsl_vector_alloc(pc), *first_step = gsl_vector_alloc(pc);
            gsl_multimin_fminimizer* m = gsl_multimin_fminimizer_alloc(gsl_multimin_fminimizer_nmsimplex2, pc);
            gsl_multimin_function function;
            function.f = &badness;
            function.n = pc;
            function.params = &params;

            for (int i = 0; i < pc; ++i) {
                gsl_vector_set(x, i, (*entries[i])());
                gsl_vector_set(first_step, i, (i < 2) ? 1 : 1E-3 );
            }
            gsl_multimin_fminimizer_set(m, &function, x, first_step);

            while ( gsl_multimin_fminimizer_iterate(m) == GSL_SUCCESS )  {
                if ( gsl_multimin_fminimizer_size(m) < target_volume() ) break;
                cur_volume = gsl_multimin_fminimizer_size(m);
            }
            for (int i = 0; i < pc; ++i) *entries[i] = gsl_vector_get(gsl_multimin_fminimizer_x(m), i);
        }
        cur_step.viewable = false;
        cur_volume.viewable = false;

        if ( output ) {
            output.get_output_stream() << scale_x() << " " << shear_x() << " " << shift_x() * 1E-6 << "\n" 
                                            << shear_y() << " " << scale_y() << " " << shift_y() * 1E-6 << "\n" 
                                            << "0 0 1\n";
            output.close_output_stream();
        }
      } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
      }
    }

    std::cout << "quit\n";
    return 0;
}
