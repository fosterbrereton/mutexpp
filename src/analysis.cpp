/******************************************************************************/
// Mutex adaptors by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

// stdc++
#include <iostream>

// application
#include "analysis.hpp"

/******************************************************************************/

std::ostream& operator<<(std::ostream& s, const normal_analysis_t& a) {
    return s << "n: " << a.count_m
             << ", min: " << a.min_m
             << ", max: " << a.max_m
             << ", avg: " << a.avg_m;
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

    return result;
}

/******************************************************************************/
