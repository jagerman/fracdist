#pragma once
#include <fracdist/common.hpp>

namespace fracdist {

/** Calculates a p-value for a given test statistic, q value, b value, and whether the model
 * contains a constant (0 for no constant, anything else for constant).
 *
 * This uses MacKinnon and Nielsen (2004) interpolation for quantile approximation and 9-point
 * P-value approximation.  To do something else, call pvalue_advanced() instead.  This method
 * is exactly equivalent to calling:
 * 
 *     pvalue_advanced(test_stat, q, b, constant, interpolation::JGMMON14, 9)
 *
 * \throws std::out_of_range for an invalid b or q value
 */
double pvalue(const double &test_stat, const unsigned int &q, const double &b, const bool &constant);

/** Like pvalue(), but requires an interpolation mode and number of P-value approximation
 * points.  `approx_points' must be at least 3 (and depending on the test_stat and parameters, might
 * need to be at least 5).
 *
 * Note that for values near the limit of the data (i.e. with pvalues close to 0 or 1), fewer points
 * will be used in the approximation (as only points out the the data limits can be used).
 *
 * \sa pvalue()
 *
 * \throws std::out_of_range for an invalid b, q value
 * \throws std::runtime_error if approx_points is too small to perform the required quadratic
 * approximation (in other words, fewer than 3 points).  This will happen with `approx_points < 5'
 * for test_stats closest to those associated with limit p-values (0.0001 and 0.9999).  Thus, while
 * `approx_points' of 3 or 4 may work for some `test_stat' values, 5 is the minimum value that never
 * results in this error.
 */
double pvalue_advanced(const double &test_stat, const unsigned int &q, const double &b, const bool &constant,
        const interpolation &interp_mode, const unsigned int &approx_points);

};
