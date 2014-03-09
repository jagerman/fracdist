#pragma once
#include <array>
#include <fracdist/data.hpp>
#include <cmath>
#include <sstream>

/** @file fracdist/common.hpp
 * @brief Header file for various common fracdist functionality.
 */

/** Namespace for all fracdist library code. */
namespace fracdist {

/** An enum giving the different quantile interpolation types supported by pvalue_advanced() and
 * critical_advanced().
 *
 * \todo add a spline interpolation mode
 */
enum class interpolation {
    /** Quadratic fitting of nearby points as described in MacKinnon and Nielsen (2014).  This
     * always uses quadratic approximation across nearby b values, even when the requested b value
     * is one of the ones in the data file.  This interpolation method gives smoother curves across
     * `b` values than the other two methods, but is slightly less accurate at known `b`
     * values \f$(0.51, 0.55, 0.6, 0.65, \ldots, 1.95, 2.0)\f$.
     */
    JGMMON14,
    /** Like interpolation::JGMMON14, but when a b value is requested that exactly matches a b value in the
     * quantile data, the exact data quantiles are used.  Otherwise, interpolation occurs as in
     * interpolation::JGMMON14.  This has the advantage of offering more precise values for known `b` values, but
     * the disadvantage that there are discontinuities in the calculated quantiles at the known `b`
     * values.
     */
    exact_or_JGMMON14,
    /** Linear interpolation between bracketing quantiles.  If, for example, `b=0.69` is provided
     * but the data only has quantiles \f$q_{0.65}\f$ and \f$q_{0.7}\f$ (for \f$b=0.65\f$ and
     * \f$b=0.7\f$), the resulting quantiles will be the weighted sum of \f$0.2q_{0.65} +
     * 0.8q_{0.7}\f$ of the two quantiles.  Like interpolation::exact_or_JGMMON14, this returns exactly the data's
     * quantiles for an exact match of `b` value.  Unlike interpolation::exact_or_JGMMON14, this method has no
     * discontinuities for changes in `b` (but does have kinks at each known `b` value).
     */
    linear
};

/** Takes \f$q\f$, \f$b\f$, constant, and interpolation mode values and calculates the quantiles for
 * the given set of values.  If any of the values is invalid, throws an exception.
 *
 * The result of the previous call is cached so that calling quantiles() a second time with the same
 * q, b, constant, and interp values will not re-perform the necessary calculations.
 *
 * This function is mainly used for internal use by the other functions in this file, but may be
 * useful for other purposes.
 *
 * \throws std::out_of_range for an invalid b, q value
 * \throws std::runtime_error if approx_points is there are no enough data points to estimate a
 * quadratic approximation.  This shouldn't happen, normally, as the data and weights ensure that
 * there will always be enough data points.
 */
const std::array<double, p_length> quantiles(const unsigned int &q, const double &b, const bool &constant, const interpolation &interp);

/** Takes a value and array and returns the index of the array value closest to the given value.
 * In the event of a tie, the lower index is returned.
 */
template <class Container,
         typename = typename std::enable_if<std::is_same<double, typename Container::value_type>::value>::type>
size_t find_closest(const double &value, const Container &array) {
    double min_diff = fabs(value - array[0]);
    size_t min_at = 0;
    for (size_t i = 1; i < array.size(); i++) {
        double diff = fabs(value - array[i]);
        if (diff < min_diff) {
            min_diff = diff;
            min_at = i;
        }
    }
    return min_at;
}

/** Finds a bracket of size at most `size` of indices centered (if possible) on the given index.  If the
 * given index is too close to 0 or `max`, the first and last values are truncated to the end points
 * (and a bracket smaller than `n` results).
 *
 * Returns an std::pair<size_t, size_t> with `pair.first` being the first bracket element,
 * `pair.second` being the last bracket element.  \f$pair_{second}-pair_{first}+1 \leq size\f$ is
 * guaranteed; the weak inequality results from end-point truncation.
 */
std::pair<size_t, size_t> find_bracket(const size_t &center, const size_t &max, const size_t &size);

/** Returns the inverse chi squared cdf at `pvalues[pval_index]` with \f$q^2\f$ degrees of freedom.
 * The value is cached (so long as the same `q` is used) so that subsequent calls for the same value
 * are very fast.
 */
double chisq_inv_p_i(const size_t &pval_index, const unsigned int &q);

/** Wrapper object around std::ostringstream that overrides the `<<` operator and is castable to a
 * std::string.  Thie is primarily intended for quickly building strings via a construction such as:
 *
 * `somethingrequiringastring(fdstringstream() << "value: " << var);`
 *
 * is valid without requiring the caller to store an intermediate ostringstream object.
 */
class ostringstream : public std::ostringstream {
    public:
        /// Constructs a new empty std::ostringstream wrapper
        ostringstream() : std::ostringstream() {}
        /// Forwards anything shifted onto this object to std::ostringstream
        template <typename T> ostringstream& operator<<(const T &v) { std::ostringstream::operator<<(v); return *this; }
        /// Can be cast implicitly to a std::string whenever required.
        operator std::string() const { return str(); }
};


}
