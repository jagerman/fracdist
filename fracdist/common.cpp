#include <fracdist/common.hpp>
#include <boost/math/distributions/chi_squared.hpp>
#include <Eigen/Core>
#include <Eigen/LU>

using Eigen::MatrixX3d;
using Eigen::VectorXd;
using Eigen::RowVector3d;

namespace fracdist {

// Caches the quantiles calculated in the last quantiles call.  If get_quantiles is called with
// the same q, b, constant, and interpolation values, we can simple return the cached
// value.
struct {
    bool cached; // False initially; will be set to true when populated
    bool constant; unsigned int q; double b; interpolation interp; // Parameters the cache was calculated for
    std::array<double, p_length> cache;
} qcache = { .cached = false };

// Updates qcache
static void qcache_store(const unsigned int &q, const double &b, const bool &constant, const interpolation &interp, const std::array<double, p_length> &quantiles) {
    qcache.cached = true;
    qcache.q = q;
    qcache.b = b;
    qcache.constant = constant;
    qcache.interp = interp;
    qcache.cache = quantiles;
}

// See description in fracdist/common.hpp
const std::array<double, p_length> quantiles(const unsigned int &q, const double &b, const bool &constant, const interpolation &interp) {
    // Will be allocated and store the result (unless an error occurs)
    std::array<double, p_length> result;

    if (qcache.cached &&
            qcache.q == q && qcache.b == b && qcache.constant == constant && qcache.interp == interp) {
        result = qcache.cache;
        return result;
    }

    if (q < 1 || q > q_length)
        throw std::out_of_range(ostringstream() << "q value (" << q << ") invalid: q must between 1 and " << q_length);
    const double bmin = bvalues.front(), bmax = bvalues.back();
    if (b < bmin || b > bmax)
        throw std::out_of_range(ostringstream() << "b value (" << b << ") invalid: b must be between " << bmin << " and " << bmax);

    // Set bmap to an alias into the q-specific b arrays
    const std::array<const std::array<double, p_length>, b_length> &bmap = constant ? q_const[q-1] : q_noconst[q-1];

    // We need to calculate weights if using the JGMMON14 method
    bool need_weights = (interp == interpolation::JGMMON14 || interp == interpolation::exact_or_JGMMON14);
    // Linear and the exact_or_JGMMON14 methods let us return right away if we have an exact b value
    bool exact_good = (interp == interpolation::exact_or_JGMMON14 || interp == interpolation::linear);

    // Will store the first b index greater than desired b (for linear interpolation)
    size_t first_gt = -1;

    // Will store the weights used for JGMMON14 method
    std::array<double, b_length> bweights;
    // The first and last b indices having non-zero weights for JGMMON14 method
    size_t bfirst = -1, blast = -1;

    for (size_t i = 0; i < b_length; i++) {
        // First check for an exact match (for appropriate interpolation modes)
        if (exact_good && bvalues[i] == b) {
            // Exact match: simply return a copy of the quantiles
            result = bmap[i];
            qcache_store(q, b, constant, interp, result);
            return result;
        }
        if (interp == interpolation::linear) {
            if (bvalues[i] > b) {
                first_gt = i;
                break;
            }
        }
        else if (need_weights) {
            double w = 1.0 - 5.0*fabs(bvalues[i] - b);
            if (w > 1e-12) {
                // Found a positive weight; store it.
                bweights[i] = w;
                if (bfirst == (size_t) -1) bfirst = i;
                blast = i;
            }
            else if (blast != (size_t) -1) {
                // This weight is non-positive, but the previous weight was positive, so we're done.
                break;
            }
        }
    }

    if (interp == interpolation::linear) {
        if (first_gt == 0 || first_gt == (size_t) -1) // Neither of these should be possible, but be defensive
            throw std::out_of_range(ostringstream() << "b value (" << b << ") invalid: b must be between " << bmin << " and " << bmax);

        // The weight to put on first_gt-1 (1 minus this is the weight for first_gt):
        const double w0 = (bvalues[first_gt] - b) / (bvalues[first_gt] - bvalues[first_gt-1]);
        const double w1 = 1 - w0;

        for (size_t i = 0; i < p_length; i++) {
            result[i] = w0 * bmap[first_gt-1][i] + w1 * bmap[first_gt][i];
        }
        qcache_store(q, b, constant, interp, result);
        return result;
    }
    else if (need_weights) {
        // We can't compute the regression if we don't have at least three values:
        if (blast - bfirst < 2)
            throw std::runtime_error(ostringstream() << "b value (" << b << ") unsupported: not enough data points for quadratic approximation");

        // This follows MacKinnon and Nielsen (2014) which calculated quantiles using a fitted quadratic
        // of nearby points.
        //
        // The weight (calculated above) is:
        //     1 - 5 abs(bhave - bwant)
        // and points with a non-positive weight (negative or less than 1e-12) are excluded.  For
        // each quantile value for b values with positive weights, we then run a weighted quadratic
        // regression on the known quantiles using the regressions:
        //     wF = w \alpha_1 + w \alpha_2 b + w \alpha_3 b^2
        // where F is the quantile value, w is the weight associated with b, and b are the b values.
        // The interpolated F' is then the fitted value from the regression evaluted at the desired
        // b.

        // The regressors don't change, so calculate the X matrix just once:
        
        MatrixX3d X(blast-bfirst+1, 3);
        for (size_t i = bfirst; i <= blast; i++) {
            X(i-bfirst, 0) = bweights[i];
            X(i-bfirst, 1) = bweights[i] * bvalues[i];
            X(i-bfirst, 2) = bweights[i] * bvalues[i] * bvalues[i];
        }

        RowVector3d wantx;
        wantx(0) = 1.0;
        wantx(1) = b;
        wantx(2) = b*b;
        auto Xt = X.transpose();

        // We want to get the fitted value for wantx, in other words, wantx * beta.  Expanding beta,
        // we get wantx * (X^T X)^-1 X^T y.  The only thing actually as we go through the data is y,
        // so we can just precompute most of the above.  fitter here will actually just be a vector.
        auto fitter = (wantx * (Xt * X).inverse() * Xt).eval();

        VectorXd y(blast-bfirst+1);

        // The regressand changes for each quantile value, which means we rerun the regression for
        // each of the 221 quantile values using the same X matrix, but different weighted y values.
        for (size_t i = 0; i < p_length; i++) {
            for (size_t j = bfirst; j <= blast; j++) {
                y(j-bfirst) = bweights[j] * bmap[j][i];
            }

            result[i] = fitter * y;
        }

        qcache_store(q, b, constant, interp, result);
        return result;
    }

    throw std::runtime_error("Internal error (BUG): unhandled interpolation");
}

// See description in common.hpp
std::pair<size_t, size_t> find_bracket(const size_t &center, const size_t &max, const size_t &size) {
    std::pair<size_t, size_t> bracket(0, 0);
    size_t left_half = size / 2; // NB: integer division
    if (center < left_half) bracket.first = 0; // We're too close to the beginning, so truncate
    else bracket.first = center - left_half;

    bracket.second = std::min(center - left_half + size - 1, max);
    return bracket;
}

unsigned int chisq_inv_cache_q = 0;
std::array<double, p_length> chisq_inv_cache;
boost::math::chi_squared_distribution<double> chisq_dist(1);
/* Returns the inverse chi squared cdf at `pvalues[pval_index]' with q^2 degrees of freedom.  The
 * value is cached (so long as q doesn't change) so that subsequent calls for the same value are
 * very fast.
 */
double chisq_inv_p_i(const size_t &pval_index, const unsigned int &q) {
    if (!q || q != chisq_inv_cache_q) {
        // Reset the cache to -1 (which we ignore)
        chisq_inv_cache.fill(-1);
        chisq_inv_cache_q = q;
        chisq_dist = boost::math::chi_squared_distribution<double>(q*q);
    }
    else if (chisq_inv_cache[pval_index] >= 0 ) {
        return chisq_inv_cache[pval_index];
    }

    return chisq_inv_cache[pval_index] = quantile(chisq_dist, pvalues[pval_index]);
}

}

