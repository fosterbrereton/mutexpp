/******************************************************************************/
// Mutex adaptors by Foster Brereton.
//
// Distributed under the MIT License. (See accompanying LICENSE.md or copy at
// https://opensource.org/licenses/MIT)
/******************************************************************************/

#ifndef ANALYSIS_HPP__
#define ANALYSIS_HPP__

/******************************************************************************/

#include <vector>

/******************************************************************************/

struct normal_analysis_t {
    std::size_t count_m{0};
    double      min_m{std::numeric_limits<double>::max()};
    double      max_m{std::numeric_limits<double>::min()};
    double      avg_m{0};
    double      stddev_m{0};
};

std::ostream& operator<<(std::ostream& s, const normal_analysis_t&);

// assumes data is distributed normally
normal_analysis_t normal_analysis(const std::vector<double>& normal_data);

void normal_analysis_header(std::ostream& s);

/******************************************************************************/

#endif // ANALYSIS_HPP__

/******************************************************************************/
