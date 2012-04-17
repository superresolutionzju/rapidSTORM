#include <boost/units/Eigen/Core>
#include "look_up_sigma_diff.h"
#include <boost/variant/apply_visitor.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/logic/tribool.hpp>

namespace dStorm {
namespace threed_info {

using namespace boost::units;

SigmaDiffLookup::SigmaDiffLookup( 
        const DepthInfo& minuend,
        Direction minuend_dimension,
        const DepthInfo& subtrahend,
        Direction subtrahend_dimension,
        ZPosition precision 
) : a( minuend ), b(subtrahend),
    a_dim(minuend_dimension), b_dim( subtrahend_dimension ),
    precision(precision)
{
    init();
}

void SigmaDiffLookup::init() {
    ZRange z_range = get_z_range();
    for ( ZPosition z = lower(z_range); z <= upper(z_range); z += precision ) {
        Diff diff;
        diff.z = z;
        diff.diff = get_sigma_diff( z );
        diffs.push_back(diff);
    }

    if ( ! diffs.empty() ) {
        crop_to_longest_monotonic_sequence();

        if ( diffs.front().diff > diffs.back().diff )
            std::reverse( diffs.begin(), diffs.end() );
        for ( std::vector<Diff>::const_iterator i = diffs.begin()+1; i != diffs.end(); ++i ) {
            if ( i->diff < (i-1)->diff )
                throw std::runtime_error("The sigma difference curve is not monotonic");
        }
    }
}

void SigmaDiffLookup::crop_to_longest_monotonic_sequence() {
    std::vector<Diff>::iterator longest_sequence_begin = diffs.begin(), longest_sequence_end = diffs.begin();
    std::vector<Diff>::iterator current_region_begin = diffs.begin();
    boost::logic::tribool increasing = boost::logic::indeterminate;

    for ( std::vector<Diff>::iterator i = diffs.begin() + 1; i != diffs.end(); ++i ) {
        bool increased = (i->diff > (i-1)->diff);
        if ( indeterminate(increasing) ) {
            increasing = increased;
        } else if ( increasing == increased ) {
            ; /* Can elongate sequence. Do nothing. */
        } else {
            if ( (i - current_region_begin) > (longest_sequence_end - longest_sequence_begin) ) {
                longest_sequence_begin = current_region_begin;
                longest_sequence_end = i;
            }
            current_region_begin = i-1;
            increasing = increased;
        }
    }
    if ( diffs.end() - current_region_begin > longest_sequence_end - longest_sequence_begin ) {
        longest_sequence_begin = current_region_begin;
        longest_sequence_end = diffs.end();
    }
    if ( longest_sequence_begin != diffs.begin() || longest_sequence_end != diffs.end() )
        std::vector<Diff>( longest_sequence_begin, longest_sequence_end ).swap( diffs );
}

Sigma SigmaDiffLookup::get_sigma_diff( ZPosition z ) const {
    return a.get_sigma( a_dim, z ) - b.get_sigma( b_dim, z );
}

ZRange SigmaDiffLookup::get_z_range() const { return a.z_range() & b.z_range(); }

ZPosition SigmaDiffLookup::operator()( Sigma x, Sigma y ) const
{
    struct sigma_compare : public std::binary_function<bool,Diff,Diff> {
        bool operator()( const Diff& a, const Diff& b ) const
            { return a.diff < b.diff; }
    };
    Diff diff;
    diff.diff = x - y;
    std::vector<Diff>::const_iterator smallest_greater_element
        = std::lower_bound( diffs.begin(), diffs.end(), diff, std::ptr_fun(&Diff::smaller_sigma_diff) );
    if ( smallest_greater_element == diffs.end() )
        return diffs.back().z;
    else
        return smallest_greater_element->z;
}

}
}