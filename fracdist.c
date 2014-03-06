#include "fracdist.h"
#include <stddef.h>
#include <stdbool.h>
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

#define FDR_ERROR(code) (fracdist_result) { .error=code, .result=NAN }
#define FDR_SUCCESS(value) (fracdist_result) { .error=fracdist_success, .result=value }

// DEBUG:

// Used internally to flag an error code to return from public functions.
static enum fracdist_error error_code_;

/* Takes q and b.  If either trigger an error, error_code_ is set to the appropriate error code and a
 * null pointer is returned.  Otherwise, returns a array pointer of length fracdist_quantiles_length
 * containing the quantiles.  The pointer must be passed to free() once no longer necessary!
 *
 * The `interpolation' parameter must be one of the following values:
 */
static double* get_quantiles(unsigned int q, double b, unsigned char constant, enum fracdist_interpolation interp) {
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

    size_t quantiles_size = sizeof(double)*fracdist_p_length;

    // Allocate space to store the result
    double *result = (double*) malloc(quantiles_size);

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
            memcpy(result, bmap[i], quantiles_size);
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
                if (bfirst == -1) bfirst = i;
                blast = i;
            }
            else if (blast != -1) {
                // This weight is non-positive, but the previous weight was positive, so we're done.
                break;
            }
        }
    }

    if (interp == fracdist_interpolate_linear) {
        if (first_gt == 0 || first_gt == -1) { // Neither of these shouldn be possible, but be defensive
            error_code_ = fracdist_error_bvalue;
            return NULL;
        }

        // The weight to put on first_gt-1 (1 minus this is the weight for first_gt):
        const double w0 = (fracdist_bvalues[first_gt] - b) / (fracdist_bvalues[first_gt] - fracdist_bvalues[first_gt-1]);
        const double w1 = 1 - w0;

        for (size_t i = 0; i < fracdist_p_length; i++) {
            result[i] = w0 * (*bmap)[first_gt-1][i] + w1 * (*bmap)[first_gt][i];
        }
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
        // The regressand changes for each quantile value
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
    }
    else {
        error_code_ = fracdist_error_interpolation;
        return NULL;
    }
    return result;
}

// Returns the chi^2 inverse value for the given probability and degrees of freedom.
double inv_chisq(double p, double df) {
    return gsl_cdf_chisq_Pinv(p, df);
}

fracdist_result fracdist_pvalue(double test_stat, unsigned int q, double b, unsigned char constant) {
    return fracdist_pvalue_advanced(test_stat, q, b, constant, fracdist_interpolate_JGMMON14, 9);
}

fracdist_result fracdist_pvalue_advanced(double test_stat, unsigned int q, double b, unsigned char constant,
        enum fracdist_interpolation interp_mode, unsigned int approx_points) {
    // Check that we didn't get passed a nonsensical number of approximation points
    if (approx_points < 3 || approx_points > fracdist_p_length) return FDR_ERROR(fracdist_error_approx_points);

    // First get the set of quantiles to use (this also checks and q and b are valid):
    double *quantiles = get_quantiles(q, b, constant, interp_mode);

    // Pass through errors in case one of the parameters was invalid
    if (error_code_) return FDR_ERROR(error_code_);

    // If asked for the p-value for a value less than half the smallest quantile we have, or more
    // than double the largest quantile we have, just give back 0 or 1.
    if (test_stat < 0.5*quantiles[0]) return FDR_SUCCESS(1);
    if (test_stat > 2*quantiles[fracdist_p_length-1]) return FDR_SUCCESS(0);

    // Otherwise we need to do some more work.

    // First find the location with a quantile closest to the requested one
    double min_diff = fabs(test_stat - quantiles[0]);
    size_t min_at = 0;
    for (size_t i = 1; i < fracdist_p_length; i++) {
        double diff = fabs(test_stat - quantiles[i]);
        if (diff < min_diff) {
            min_diff = diff;
            min_at = i;
        }
    }

    // Now figure out a set of `approx_points' consecutive points centered on the closest value
    size_t ap_first, ap_last;

    int left_half = approx_points / 2; // NB: integer division
    if (min_at < left_half) ap_first = 0; // We're too close to the beginning, so truncate
    else ap_first = min_at - left_half;

    ap_last = min_at + approx_points - left_half - 1;
    if (ap_last >= fracdist_p_length) ap_last = fracdist_p_length-1; // Truncate if necessary

    if (ap_last - ap_first < 2) return FDR_ERROR(fracdist_error_approx_points);

    // Now we're going to run the regression:
    // 
    //     chisqinv_i = \beta_1 + \beta_2 quantile_i + \beta_3 quantile_i^2
    //
    // where chisqinv_i is the inverse cdf at p=pvalue[i] of a chi-squared distribution with q^2 df
    //
    // The fitted value then gives us a fitted chi-squared value for which we can get a pvalue.

    gsl_matrix *X = gsl_matrix_alloc(ap_last - ap_first + 1, 3);
    gsl_vector *y = gsl_vector_alloc(ap_last - ap_first + 1);
    for (size_t i = ap_first; i <= ap_last; i++) {
        gsl_matrix_set(X, i-ap_first, 0, 1.0);
        gsl_matrix_set(X, i-ap_first, 1, quantiles[i]);
        gsl_matrix_set(X, i-ap_first, 2, gsl_pow_2(quantiles[i]));
        gsl_vector_set(y, i-ap_first, inv_chisq(fracdist_pvalues[i], q*q));
    }

    gsl_vector *beta = gsl_vector_alloc(3);
    gsl_matrix *cov = gsl_matrix_alloc(3, 3);
    double ssr;
    gsl_multifit_linear_workspace *work = gsl_multifit_linear_alloc(X->size1, X->size2);
    gsl_multifit_linear(X, y, beta, cov, &ssr, work);

    gsl_matrix_free(X);
    gsl_vector_free(y);
    gsl_matrix_free(cov);
    gsl_multifit_linear_free(work);

    double fitted = gsl_vector_get(beta, 0) + gsl_vector_get(beta, 1) * test_stat + gsl_vector_get(beta, 2) * gsl_pow_2(test_stat);
    gsl_vector_free(beta);

    if (fitted < 0) return FDR_SUCCESS(1);

    // NB: chisq_Q is the upper-tail chisq cdf (chisq_P is the lower tail)
    return FDR_SUCCESS(gsl_cdf_chisq_Q(fitted, q*q));
}

