#include "fracdist.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_cdf.h>

// DEBUG:
#define FRACDIST_DEBUG 1

#ifndef FRACDIST_DEBUG
#define FRACDIST_DEBUG 0
#endif
#define DBG(format, ...) do { if (FRACDIST_DEBUG) fprintf(stderr, "%s:%d:%s(): " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)

// Various shortcut macros
#define ERROR(code) (fracdist_result) { .error=code, .result=NAN }
#define SUCCESS(value) (fracdist_result) { .error=fracdist_success, .result=value }
// This do { ... } while (0) construct ensures that the macro expands to a single expression, so
// that code such as: if (...) FREE_RETURN_ERROR(...); doesn't break.  The intermediate value in the
// second is to permit the value to be in the pointer being freed.
#define FREE_RETURN_ERROR(ref, code) do { free((void*) ref); return ERROR(code); } while (0)
#define FREE_RETURN_SUCCESS(ref, value) do { double r = value; free((void*) ref); return SUCCESS(r); } while (0)

// DEBUG:

// Used internally to flag an error code to return from public functions.
static enum fracdist_error error_code_;

// Caches the quantiles calculated in the last get_quantiles call.  If get_quantiles is called with
// the same q, b, constant, and fracdist_interpolation values, we can simple return the cached
// value.
static struct {
    bool cached; // False initially; will be set to true when populated
    bool constant; unsigned int q; double b; enum fracdist_interpolation interp; // Parameters the cache was calculated for
    double cache[fracdist_p_length];
} qcache = { .cached = false };

// Updates qcache
static void qcache_store(const unsigned int q, const double b, const bool constant, const enum fracdist_interpolation interp, const double *quantiles, const size_t qlen) {
    qcache.cached = true;
    qcache.q = q;
    qcache.b = b;
    qcache.constant = constant;
    qcache.interp = interp;
    memcpy(&qcache.cache, quantiles, qlen);
}

// See description in fracdist.h
const double* fracdist_get_quantiles(const unsigned int q, const double b, const bool constant, const enum fracdist_interpolation interp) {
    // Will be allocated and store the result (unless an error occurs)
    double *result;
    size_t result_bytes = sizeof(double)*fracdist_p_length;

    if (qcache.cached &&
            qcache.q == q && qcache.b == b && qcache.constant == constant && qcache.interp == interp) {
        result = (double*) malloc(result_bytes);
        memcpy(result, qcache.cache, result_bytes);
        return result;
    }


    error_code_ = fracdist_success;
    if (q < 1 || q > fracdist_q_length) {
        error_code_ = fracdist_error_qvalue;
    }
    else if (b < fracdist_bvalues[0] || b > fracdist_bvalues[fracdist_b_length-1]) {
        error_code_ = fracdist_error_bvalue;
    }
    else if (interp != fracdist_interpolate_JGMMON14 &&
            interp != fracdist_interpolate_linear &&
            interp != fracdist_interpolate_linear) {
        error_code_ = fracdist_error_interpolation;
    }
    if (error_code_) return NULL;

    // Set bmap to a pointer to the appropriate 2d array for the given q and constant values
    const double (*bmap)[fracdist_b_length][fracdist_p_length] = constant ? &fracdist_q_const[q-1] : &fracdist_q_noconst[q-1];


    // We need to calculate weights if using the JGMMON14 method
    bool need_weights = (interp == fracdist_interpolate_JGMMON14
            || interp == fracdist_interpolate_exact_or_JGMMON14);
    // Linear and the exact_or_JGMMON14 methods let us return right away if we have an exact b value
    bool exact_good = (interp == fracdist_interpolate_exact_or_JGMMON14
            || interp == fracdist_interpolate_linear);

    // Will store the first b index greater than desired b (for linear interpolation)
    size_t first_gt = -1;

    // Will store the weights used for JGMMON14 method
    double bweights[fracdist_b_length];
    // The first and last b indices having non-zero weights for JGMMON14 method
    size_t bfirst = -1, blast = -1;

    for (size_t i = 0; i < fracdist_b_length; i++) {
        // First check for an exact match (for appropriate interpolation modes)
        if (exact_good && fracdist_bvalues[i] == b) {
            // Exact match: simply return a copy of the quantiles
            result = (double*) malloc(result_bytes);
            memcpy(result, bmap[i], result_bytes);
            qcache_store(q, b, constant, interp, result, result_bytes);
            return result;
        }
        if (interp == fracdist_interpolate_linear) {
            if (fracdist_bvalues[i] > b) {
                first_gt = i;
                break;
            }
        }
        else if (need_weights) {
            double w = 1.0 - 5.0*fabs(fracdist_bvalues[i] - b);
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

    if (interp == fracdist_interpolate_linear) {
        if (first_gt == 0 || first_gt == (size_t) -1) { // Neither of these should be possible, but be defensive
            error_code_ = fracdist_error_bvalue;
            return NULL;
        }

        // The weight to put on first_gt-1 (1 minus this is the weight for first_gt):
        const double w0 = (fracdist_bvalues[first_gt] - b) / (fracdist_bvalues[first_gt] - fracdist_bvalues[first_gt-1]);
        const double w1 = 1 - w0;

        result = (double*) malloc(result_bytes);
        for (size_t i = 0; i < fracdist_p_length; i++) {
            result[i] = w0 * (*bmap)[first_gt-1][i] + w1 * (*bmap)[first_gt][i];
        }
        qcache_store(q, b, constant, interp, result, result_bytes);
        return result;
    }
    else if (need_weights) {
        // We can't compute the regression if we don't have at least three values:
        if (blast - bfirst < 2) {
            error_code_ = fracdist_error_bvalue;
            return NULL;
        }

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
        gsl_matrix *X = gsl_matrix_alloc(blast-bfirst+1, 3);
        for (size_t i = bfirst; i <= blast; i++) {
            gsl_matrix_set(X, i-bfirst, 0, bweights[i]);
            gsl_matrix_set(X, i-bfirst, 1, bweights[i] * fracdist_bvalues[i]);
            gsl_matrix_set(X, i-bfirst, 2, bweights[i] * gsl_pow_2(fracdist_bvalues[i]));
        }

        gsl_vector *y = gsl_vector_alloc(blast-bfirst+1);
        gsl_vector *eta = gsl_vector_alloc(3);
        gsl_matrix *cov = gsl_matrix_alloc(3, 3);
        double ssr;
        gsl_multifit_linear_workspace *work = gsl_multifit_linear_alloc(blast-bfirst+1, 3);

        // Allocate space for the result
        result = (double*) malloc(result_bytes);

        // The regressand changes for each quantile value, which means we rerun the regression for
        // each of the 221 quantile values using the same X matrix, but different weighted y values.
        for (size_t i = 0; i < fracdist_p_length; i++) {
            for (size_t j = bfirst; j <= blast; j++) {
                gsl_vector_set(y, j-bfirst, bweights[j] * (*bmap)[j][i]);
            }

            gsl_multifit_linear(X, y, eta, cov, &ssr, work);

            // The fitted value for the desired b is our interpolated quantile value
            result[i] = gsl_vector_get(eta, 0) + gsl_vector_get(eta, 1) * b + gsl_vector_get(eta, 2) * gsl_pow_2(b);
        }

        gsl_matrix_free(X);
        gsl_vector_free(y);
        gsl_vector_free(eta);
        gsl_matrix_free(cov);
        gsl_multifit_linear_free(work);

        qcache_store(q, b, constant, interp, result, result_bytes);
        return result;
    }

    error_code_ = fracdist_error_interpolation;
    return NULL;
}


/* Takes a value, double array pointer, and length of the array and returns the index of the array
 * value closest to the given value.  alen must not be 0 (but this is not checked).  In the event of
 * a tie, the lower index is returned.
 */
size_t find_closest(const double value, const double *array, const size_t alen) {
    double min_diff = fabs(value - array[0]);
    size_t min_at = 0;
    for (size_t i = 1; i < alen; i++) {
        double diff = fabs(value - array[i]);
        if (diff < min_diff) {
            min_diff = diff;
            min_at = i;
        }
    }
    return min_at;
}

typedef struct { size_t first; size_t last; } index_bracket;

/* Finds a bracket of size at most `n' of indices centered (if possible) on the given index.  If the
 * given index is too close to 0 or `max', the first and last values are truncated to the end points
 * (and a bracket smaller than `n' results).
 *
 * Returns a struct with .first being the first bracket element, .last being the last bracket
 * element.  last-first+1 <= n is guaranteed.
 */
index_bracket find_bracket(const size_t center, const size_t max, const size_t size) {
    index_bracket bracket;
    size_t left_half = size / 2; // NB: integer division
    if (center < left_half) bracket.first = 0; // We're too close to the beginning, so truncate
    else bracket.first = center - left_half;

    bracket.last = center - left_half + size - 1;
    if (bracket.last > max) bracket.last = max; // Truncate if necessary
    return bracket;
}

static unsigned int chisq_inv_cache_q = 0;
static double chisq_inv_cache[fracdist_p_length] = {0};
/* Returns the inverse chi squared cdf at `fracdist_pvalues[i]' with q^2 degrees of freedom.  The
 * value is cached (so long as q doesn't change) so that subsequent calls for the same value are
 * very fast.
 */
double chisq_inv_p_i(const size_t pval_index, const unsigned int q) {
    if (q != chisq_inv_cache_q) {
        // Reset the cache to NAN
        for (size_t i = 0; i < fracdist_p_length; i++) {
            chisq_inv_cache[i] = NAN;
        }
        chisq_inv_cache_q = q;
    }
    else if (!isnan(chisq_inv_cache[pval_index])) {
        return chisq_inv_cache[pval_index];
    }

    return chisq_inv_cache[pval_index] = gsl_cdf_chisq_Pinv(fracdist_pvalues[pval_index], q*q);
}

// See description in fracdist.h
fracdist_result fracdist_pvalue(const double test_stat, const unsigned int q, const double b, const bool constant) {
    return fracdist_pvalue_advanced(test_stat, q, b, constant, fracdist_interpolate_JGMMON14, 9);
}

// See description in fracdist.h
fracdist_result fracdist_pvalue_advanced(const double test_stat, const unsigned int q, const double b, const bool constant,
        const enum fracdist_interpolation interp_mode, const unsigned int approx_points) {

    if (test_stat < 0)
        return ERROR(fracdist_error_teststat);
    // The critical values for test stats of 0 or infinity are trivial: 1 or 0.
    if (test_stat == 0)
        return SUCCESS(1);
    if (isinf(test_stat))
        return SUCCESS(0);

    // First get the set of quantiles to use (this also checks and q and b are valid):
    const double *quantiles = fracdist_get_quantiles(q, b, constant, interp_mode);

    // Pass through errors in case one of the parameters was invalid
    if (error_code_) FREE_RETURN_ERROR(quantiles, error_code_);

    if (quantiles == NULL) // This shouldn't happen without error_code_ being set
        return ERROR(fracdist_error_unknown);

    // If asked for the p-value for a value less than half the smallest quantile we have, or more
    // than double the largest quantile we have, just give back 0 or 1.
    if (test_stat < 0.5*quantiles[0]) FREE_RETURN_SUCCESS(quantiles, 1);
    if (test_stat > 2*quantiles[fracdist_p_length-1]) FREE_RETURN_SUCCESS(quantiles, 0);

    // Otherwise we need to do some more work.

    // First find the location with a quantile closest to the requested one
    size_t min_at = find_closest(test_stat, quantiles, fracdist_p_length);

    // Now figure out a set of `approx_points' consecutive points centered on the closest value
    index_bracket ap = find_bracket(min_at, fracdist_p_length-1, approx_points);

    if (ap.last - ap.first < 2) FREE_RETURN_ERROR(quantiles, fracdist_error_approx_points);

    // Now we're going to run the regression:
    // 
    //     chisqinv_i = \beta_1 + \beta_2 quantile_i + \beta_3 quantile_i^2
    //
    // where chisqinv_i is the inverse cdf at p=pvalue[i] of a chi-squared distribution with q^2 df
    //
    // The fitted value then gives us a fitted chi-squared value for which we can get a pvalue.

    gsl_matrix *X = gsl_matrix_alloc(ap.last - ap.first + 1, 3);
    gsl_vector *y = gsl_vector_alloc(ap.last - ap.first + 1);
    for (size_t i = ap.first; i <= ap.last; i++) {
        gsl_matrix_set(X, i-ap.first, 0, 1.0);
        gsl_matrix_set(X, i-ap.first, 1, quantiles[i]);
        gsl_matrix_set(X, i-ap.first, 2, gsl_pow_2(quantiles[i]));
        gsl_vector_set(y, i-ap.first, chisq_inv_p_i(i, q));
    }

    // Allocate the variables we need for the regression
    gsl_vector *beta = gsl_vector_alloc(3);
    gsl_matrix *cov = gsl_matrix_alloc(3, 3);
    double ssr;
    gsl_multifit_linear_workspace *work = gsl_multifit_linear_alloc(X->size1, X->size2);
    gsl_multifit_linear(X, y, beta, cov, &ssr, work);

    // Get the fitted value from the regression using our actual test statistic
    double fitted = gsl_vector_get(beta, 0) + gsl_vector_get(beta, 1) * test_stat + gsl_vector_get(beta, 2) * gsl_pow_2(test_stat);

    // Free regression variables
    gsl_multifit_linear_free(work);
    gsl_matrix_free(cov);
    gsl_vector_free(beta);
    gsl_matrix_free(X);
    gsl_vector_free(y);

    // A negative isn't valid, so if we predicted one anyway, truncate it at 0 (which corresponds to
    // a pvalue of 1).
    if (fitted < 0) FREE_RETURN_SUCCESS(quantiles, 1);

    // NB: chisq_Q gets the *upper-tail* chisq cdf (chisq_P gives the lower tail)
    FREE_RETURN_SUCCESS(quantiles, gsl_cdf_chisq_Q(fitted, q*q));
}

fracdist_result fracdist_critical(const double test_level, const unsigned int q, const double b, const bool constant) {
    return fracdist_critical_advanced(test_level, q, b, constant, fracdist_interpolate_JGMMON14, 9);
}

fracdist_result fracdist_critical_advanced(double test_level, const unsigned int q, const double b, const bool constant,
        const enum fracdist_interpolation interp_mode, const unsigned int approx_points) {

    // Take 1 minus the level to make it comparable to our stored p-values
    test_level = 1 - test_level;

    if (test_level < 0 || test_level > 1)
        return ERROR(fracdist_error_pvalue);
    // The critical values for test levels of 0 or 1 are trivial: 0 or infinity.
    if (test_level == 0)
        return SUCCESS(0);
    if (test_level == 1)
        return SUCCESS(INFINITY);

    // First get the set of quantiles to use (this also checks and q and b are valid):
    const double *quantiles = fracdist_get_quantiles(q, b, constant, interp_mode);

    // Pass through errors in case one of the parameters was invalid
    if (error_code_) FREE_RETURN_ERROR(quantiles, error_code_);

    if (quantiles == NULL) // This shouldn't happen without error_code_ being set
        FREE_RETURN_ERROR(quantiles, fracdist_error_unknown);

    // If we're asked for a smaller or larger p value than our data limits, return the limit value
    if (test_level <= fracdist_pvalues[0])
        FREE_RETURN_SUCCESS(quantiles, quantiles[0]);
    if (test_level >= fracdist_pvalues[fracdist_p_length-1])
        FREE_RETURN_SUCCESS(quantiles, quantiles[fracdist_p_length-1]);

    // First find the location with a pvalue closest to the requested one
    size_t min_at = find_closest(test_level, fracdist_pvalues, fracdist_p_length);

    // Now figure out a set of `approx_points' consecutive points centered on the closest value
    index_bracket ap = find_bracket(min_at, fracdist_p_length-1, approx_points);

    if (ap.last - ap.first < 2) FREE_RETURN_ERROR(quantiles, fracdist_error_approx_points);

    // Now we're going to estimate the regression:
    // 
    //     quantile_i = \beta_1 + \beta_2 chisqinv_i + \beta_3 chisqinv_i^2
    //
    // using the points surrounding the requested pvalue, where chisqinv_i is the inverse cdf at
    // p=pvalue[i] of a chi-squared distribution with q^2 df
    //
    // The fitted value using the inverse chi squared at our desired pvalue then gives us a our
    // estimated critical value.

    gsl_matrix *X = gsl_matrix_alloc(ap.last - ap.first + 1, 3);
    gsl_vector *y = gsl_vector_alloc(ap.last - ap.first + 1);
    for (size_t i = ap.first; i <= ap.last; i++) {
        double chisqinv = chisq_inv_p_i(i, q);
        gsl_matrix_set(X, i-ap.first, 0, 1.0);
        gsl_matrix_set(X, i-ap.first, 1, chisqinv);
        gsl_matrix_set(X, i-ap.first, 2, gsl_pow_2(chisqinv));

        gsl_vector_set(y, i-ap.first, quantiles[i]);
    }

    // Allocate the variables we need for the regression
    gsl_vector *beta = gsl_vector_alloc(3);
    gsl_matrix *cov = gsl_matrix_alloc(3, 3);
    double ssr;
    gsl_multifit_linear_workspace *work = gsl_multifit_linear_alloc(X->size1, X->size2);
    gsl_multifit_linear(X, y, beta, cov, &ssr, work);

    // Get the fitted value from the regression using the inverse of our actual test level
    double chisqinv_actual = gsl_cdf_chisq_Pinv(test_level, q*q);
    double fitted = gsl_vector_get(beta, 0) + gsl_vector_get(beta, 1) * chisqinv_actual + gsl_vector_get(beta, 2) * gsl_pow_2(chisqinv_actual);

    // Free regression variables
    gsl_multifit_linear_free(work);
    gsl_matrix_free(cov);
    gsl_vector_free(beta);
    gsl_matrix_free(X);
    gsl_vector_free(y);

    // Negative critical values are impossible; if we somehow got a negative prediction, truncate it
    if (fitted < 0) fitted = 0;
    FREE_RETURN_SUCCESS(quantiles, fitted);
}
