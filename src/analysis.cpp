/******************************************************************************/
// Mutex adaptors by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

// stdc++
#include <cmath>
#include <iostream>

// application
#include "analysis.hpp"

/******************************************************************************/

void normal_analysis_header(std::ostream& s) {
    // make sure this routine accurately reflects the fields being output.
    s //<< "n,"
      << "sig3 high,"
      << "sig3 low,"
      << "avg,"
      << "min,"
      << "max,"
      << "stddev"
      << '\n'
      ;
}

/******************************************************************************/

std::ostream& operator<<(std::ostream& s, const normal_analysis_t& a) {
    auto sig3 = 3 * a.stddev_m;
    auto sig3low = a.avg_m - sig3;
    auto sig3high = a.avg_m + sig3;

    return s //<< a.count_m << ','
             << sig3high << ','
             << sig3low << ','
             << a.avg_m << ','
             << a.min_m << ','
             << a.max_m << ','
             << a.stddev_m
             ;
}

/******************************************************************************/

normal_analysis_t normal_analysis(const std::vector<double>& normal_data) {
    if (normal_data.empty())
        throw std::runtime_error("data empty.");

    normal_analysis_t result;

    result.count_m = normal_data.size();

    for (const auto& datum : normal_data) {
        if (datum < result.min_m)
            result.min_m = datum;

        if (datum > result.max_m)
            result.max_m = datum;

        result.avg_m += datum;
    }

    result.avg_m /= result.count_m;

    for (const auto& datum : normal_data) {
        double diff = datum - result.avg_m;
        result.stddev_m += diff * diff;
    }

    result.stddev_m /= result.count_m;

    result.stddev_m = std::sqrt(result.stddev_m);

    return result;
}

/******************************************************************************/
