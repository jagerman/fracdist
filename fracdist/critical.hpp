#pragma once
#include <fracdist/common.hpp>

namespace fracdist {

/** Calculates a critical value for a given level of the test.  Takes the level, q value, b value,
 * and whether the model contains a constant (0 for no constant, anything else for constant).
 *
 * \throws std::out_of_range for an invalid b or q value, or for a test level outside [0, 1].
 */
double critical(const double &test_level, const unsigned int &q, const double &b, const bool &constant);

/** Like critical(), but also takes an interpolation mode and number of P-value approximation
 * points.  `approx_points' must be at least 3 (and depending on the test_stat and parameters, might
 * need to be at least 5).
 *
 * Note that for values near the limit of the data (i.e. with pvalues close to 0 or 1), fewer points
 * will be used in the approximation (as only points out the the data limits can be used).
 *
 * \sa critical()
 *
 * \throws std::out_of_range for an invalid b or q value, or for a test level outside [0, 1].
 * \throws std::runtime_error if approx_points is too small to perform the required quadratic
 * approximation (in other words, fewer than 3 points).  This will happen with `approx_points < 5'
 * for test_stats closest to those associated with limit p-values (0.0001 and 0.9999).  Thus, while
 * `approx_points' of 3 or 4 may work for some `test_stat' values, 5 is the minimum value that never
 * results in this error.
 */
double critical_advanced(double test_level, const unsigned int &q, const double &b, const bool &constant,
        const interpolation &interp_mode, const unsigned int &approx_points);

}
