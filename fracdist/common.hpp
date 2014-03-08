#pragma once
#include <array>
#include <fracdist/data.hpp>
#include <cmath>
#include <sstream>

namespace fracdist {

/** An enum giving the different quantile interpolation types supported by pvalue_advanced and
 * critical_advanced.
 */
enum class interpolation {
    /** Quadratic fitting of nearby points as described in MacKinnon and Nielsen (2014).  This
     * always uses quadratic approximation across nearby b values, even when the requested b value
     * is one of the ones in the data file.  This interpolation method gives smoother curves across
     * `b' values than the other two methods, but is slightly less accurate at known `b'
     * values (0.51, 0.55, 0.6, 0.65, ..., 1.95, 2.0).
     */
    JGMMON14,
    /** Like JGMMON14, but when a b value is requested that exactly matches a b value in the
     * quantile data, the exact data quantiles are used.  Otherwise, interpolation occurs as in
     * JGMMON14.  This has the advantage of offering more precise values for known `b' values, but
     * the disadvantage that there are discontinuities in the calculated quantiles at the known `b'
     * values.
     */
    exact_or_JGMMON14,
    /** Linear interpolation between bracketing quantiles.  If, for example, `b=0.69` is provided
     * but the data only has `b=0.65' and `b=0.7', the resulting quantiles will be the weighted sum
     * of \f$0.2q_{b=0.65} + 0.8q_{b=0.7}\f$ of the two quantiles.  Like exact_or_JGMMON14, this
     * returns exactly the data's quantiles for an exact match of `b' value.  Unlike
     * exact_or_JGMMON14, this method has no discontinuities for changes in `b' (but doesn't have
     * kinks at each known `b' value).
     */
    linear
};

/** Takes q, b, constant, and interpolation mode values and calculates the quantiles for the given
 * values.  If anything is invalid, throws an exception.
 *
 * The result of the previous call is cached so that calling quantiles() a second time with the same
 * q, b, constant, and interp values will not re-perform the necessary calculations.
 *
 * This function is mainly used for internal use by the other functions in this file, but may be
 * useful for other purposes.
 */
const std::array<double, p_length> quantiles(const unsigned int &q, const double &b, const bool &constant, const interpolation &interp);

// Caches the quantiles calculated in the last quantiles call.  If get_quantiles is called with
// the same q, b, constant, and interpolation values, we can simple return the cached
// value.
struct QCache {
    bool cached; // False initially; will be set to true when populated
    bool constant; unsigned int q; double b; interpolation interp; // Parameters the cache was calculated for
    std::array<double, p_length> cache;
};

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

/** Finds a bracket of size at most `size' of indices centered (if possible) on the given index.  If the
 * given index is too close to 0 or `max', the first and last values are truncated to the end points
 * (and a bracket smaller than `n' results).
 *
 * Returns an std::pair<size_t, size_t> with .first being the first bracket element, .second being
 * the last bracket element.  last-first+1 <= size is guaranteed.
 */
std::pair<size_t, size_t> find_bracket(const size_t &center, const size_t &max, const size_t &size);

/** Returns the inverse chi squared cdf at `pvalues[pval_index]' with q^2 degrees of freedom.  The
 * value is cached (so long as q doesn't change) so that subsequent calls for the same value are
 * very fast.
 */
double chisq_inv_p_i(const size_t &pval_index, const unsigned int &q);

/** Wrapper object around std::ostringstream that overrides << and is castable to a std::string so
 * that a construction like:
 *
 *     somethingrequiringastring(fdstringstream() << "a" << "b")
 *
 * is valid without requiring the caller to store an intermediate ostringstream object.
 */
class ostringstream : public std::ostringstream {
    public:
        ostringstream() : std::ostringstream() {}
        template <typename T> ostringstream& operator<<(const T &v) { std::ostringstream::operator<<(v); return *this; }
        operator std::string() const { return str(); }
};


}
