#pragma once
#include <stdbool.h>
#include "fracdist-data.h"

/** @file fracdist.h
 * \brief Header for public fracdist interface methods.
 */

/** Enum for the different error codes possible in a fracdist_result */
enum fracdist_error {
    /** "No error" code, i.e. for a successful result.  This is explictly 0 (and thus false) so that
     * `if (res.error)' can be used to check for any type of error.
     */
    fracdist_success = 0,
    /** Error for invalid `b' value (i.e. outside [0.51, 2.0]) */
    fracdist_error_bvalue,
    /** Error for invalid `q' value (i.e. outside [1, 12]) */
    fracdist_error_qvalue,
    /** Error for invalid interpolation value */
    fracdist_error_interpolation,
    /** Error for invalid number of approximation points */
    fracdist_error_approx_points,
    /** Error code for some unknown error (if this occurs, there's a bug in the program) */
    fracdist_error_unknown
};

/** Returns when returning a double.  .error will be 0 if the calculation was successful, to a
 * function-specific error code others.  If no error occurred, .result holds the resulting value.
 */
typedef struct {
    enum fracdist_error error;
    double result;
} fracdist_result;

/** An enum giving the different quantile interpolation types supported by fracdist_pvalue_advanced
 * and fracdist_crit_advanced.
 */
enum fracdist_interpolation {
    /** Quadratic fitting of nearby points as described in MacKinnon and Nielsen (2014).  This
     * always uses quadratic approximation across nearby b values, even when the requested b value
     * is one of the ones in the data file.  This interpolation method gives smoother curves across
     * `b' values than the other two methods, but is slightly less accurate at known `b'
     * values (0.51, 0.55, 0.6, 0.65, ..., 1.95, 2.0).
     */
    fracdist_interpolate_JGMMON14,
    /** Like JGMMON14, but when a b value is requested that exactly matches a b value in the
     * quantile data, the exact data quantiles are used.  Otherwise, interpolation occurs as in
     * JGMMON14.  This has the advantage of offering more precise values for known `b' values, but
     * the disadvantage that there are discontinuities in the calculated quantiles at the known `b'
     * values.
     */
    fracdist_interpolate_exact_or_JGMMON14,
    /** Linear interpolation between bracketing quantiles.  If, for example, `b=0.69` is provided
     * but the data only has `b=0.65' and `b=0.7', the resulting quantiles will be the weighted sum
     * of \f$0.2q_{b=0.65} + 0.8q_{b=0.7}\f$ of the two quantiles.  Like exact_or_JGMMON14, this
     * returns exactly the data's quantiles for an exact match of `b' value.  Unlike
     * exact_or_JGMMON14, this method has no discontinuities for changes in `b' (but doesn't have
     * kinks at each known `b' value).
     */
    fracdist_interpolate_linear
};

/** Calculates a p-value for a given test statistic, q value, b value, and whether the model
 * contains a constant (0 for no constant, anything else for constant).
 *
 * This uses MacKinnon and Nielsen (2004) interpolation for quantile approximation and 9-point
 * P-value approximation.  To do something else, call fracdist_pvalue_advanced instead.  This method
 * is exactly equivalent to calling:
 * 
 *     fracdist_pvalue_advanced(test_stat, q, b, constant, fracdist_interpolate_JGMMON14, 9)
 *
 * Returns the following error codes:
 * - 0 (= fracdist_success): no error (.result holds the desired pvalue)
 * - fracdist_error_bvalue: b value is not supported (i.e. outside [0.51, 2])
 * - fracdist_error_qvalue: q value is not supported (i.e. outside [1, 12])
 */
fracdist_result fracdist_pvalue(const double test_stat, const unsigned int q, const double b, const bool constant);

/** Like fracdist_pvalue, but requires an interpolation mode and number of P-value approximation
 * points.  `approx_points' must be at least 3 (and depending on the test_stat and parameters, might
 * need to be at least 5).
 *
 * Note that for values near the limit of the data (i.e. with pvalues close to 0 or 1), fewer points
 * will be used in the approximation (as only points out the the data limits can be used).
 *
 * \sa fracdist_pvalue
 *
 * In addition to the codes mentioned by fracdist_pvalue, this can return error codes:
 * - fracdist_error_interpolation: fracdist_interpolation is set to something invalid/unsupported
 * - fracdist_error_approx_points - invalid number of approximation points.  This will be triggered
 *   if there are fewer than 3 admissable points (which will happen with `approx_points=5' for
 *   test_stats closest to those associated with limit p-values (0.0001 and 0.9999).  Thus, while
 *   `approx_points' of 3 or 4 may work for some `test_stat' values, 5 is the minimum value that
 *   never results in this error.
 */
fracdist_result fracdist_pvalue_advanced(const double test_stat, const unsigned int q, const double b, const bool constant,
        const enum fracdist_interpolation interp_mode, const unsigned int approx_points);

/** Calculates a critical value for a given level of the test.  Takes the level, q value, b value,
 * and whether the model contains a constant (0 for no constant, anything else for constant).
 *
 * Returns the following error codes:
 * - 0 (= fracdist_success): no error (.result holds the desired pvalue)
 * - fracdist_error_bvalue: b value is not supported (i.e. outside [0.51, 2])
 * - fracdist_error_qvalue: q value is not supported (i.e. outside [1, 12])
 */
fracdist_result fracdist_critical(const double test_level, const unsigned int q, const double b, const bool constant);

/** Like fracdist_critical, but also takes an interpolation mode and number of P-value approximation
 * points.  `approx_points' must be at least 3 (and depending on the test_stat and parameters, might
 * need to be at least 5).
 *
 * Note that for values near the limit of the data (i.e. with pvalues close to 0 or 1), fewer points
 * will be used in the approximation (as only points out the the data limits can be used).
 *
 * \sa fracdist_critical
 *
 * In addition to the codes mentioned by fracdist_critical, this can return error codes:
 * - fracdist_error_interpolation: fracdist_interpolation is set to something invalid/unsupported
 * - fracdist_error_approx_points - invalid number of approximation points.  This will be triggered
 *   if there are fewer than 3 admissable points (which will happen with `approx_points=5' for p
 *   values close to the limits of the data (0.0001 and 0.9999).  Thus, while `approx_points' of 3
 *   or 4 may work for some `test_stat' values, 5 is the minimum value that never results in this
 *   error.
 */
fracdist_result fracdist_critical_advanced(double test_level, const unsigned int q, const double b, const bool constant,
        const enum fracdist_interpolation interp_mode, const unsigned int approx_points);

/** Takes q, b, constant, and interpolation mode values and calculates the quantiles for the given
 * values.  If anything is invalid, error_code_ is set to the appropriate error code and a null
 * pointer is returned.  Otherwise, returns a array pointer of length fracdist_p_length containing
 * the quantiles.  The pointer must be passed to free() once no longer necessary!
 *
 * The result of the previous call is cached so that calling fracdist_get_quantiles a second time
 * with the same q, b, constant, and interp values will not reperform the necessary calculations.
 *
 * This function is mainly used for internal use by the other functions in this file, but may be
 * useful for other purposes.
 */
const double* fracdist_get_quantiles(const unsigned int q, const double b, const bool constant, const enum fracdist_interpolation interp);

