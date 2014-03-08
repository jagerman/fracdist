#include <fracdist/critical.hpp>
#include <boost/math/distributions/chi_squared.hpp>
#include <boost/format.hpp>
#include <Eigen/Core>
#include <Eigen/LU>

using namespace Eigen;
namespace fracdist {

double critical(const double &test_level, const unsigned int &q, const double &b, const bool &constant) {
    return critical_advanced(test_level, q, b, constant, interpolation::JGMMON14, 9);
}

double critical_advanced(double test_level, const unsigned int &q, const double &b, const bool &constant,
        const interpolation &interp_mode, const unsigned int &approx_points) {

    // Take 1 minus the level to make it comparable to our stored p-values
    test_level = 1 - test_level;

    if (test_level < 0 || test_level > 1)
        throw std::out_of_range((boost::format("test level (%1%) invalid: must be between 0 and 1") % test_level).str());
    // The critical values for test levels of 0 or 1 are trivial: 0 or infinity.
    if (test_level == 0) return 0.0;
    if (test_level == 1) return INFINITY;

    // First get the set of quantiles to use (this also checks and q and b are valid):
    auto quant = quantiles(q, b, constant, interp_mode);

    // If we're asked for a smaller or larger p value than our data limits, return the limit value
    if (test_level <= pvalues.front()) return quant.front();
    if (test_level >= pvalues.back()) return quant.back();

    // First find the location with a pvalue closest to the requested one
    size_t min_at = find_closest(test_level, pvalues);

    // Now figure out a set of `approx_points' consecutive points centered on the closest value
    auto ap = find_bracket(min_at, p_length-1, approx_points);

    if (ap.second - ap.first < 2)
        throw std::runtime_error((boost::format("approx_points (%1%) too small: not enough data points for quadratic approximation") % approx_points).str());

    // Now we're going to estimate the regression:
    // 
    //     quantile_i = \beta_1 + \beta_2 chisqinv_i + \beta_3 chisqinv_i^2
    //
    // using the points surrounding the requested pvalue, where chisqinv_i is the inverse cdf at
    // p=pvalue[i] of a chi-squared distribution with q^2 df
    //
    // The fitted value using the inverse chi squared at our desired pvalue then gives us a our
    // estimated critical value.

    MatrixX3d X(ap.second - ap.first + 1, 3);
    VectorXd y(ap.second - ap.first + 1);
    for (size_t i = ap.first; i <= ap.second; i++) {
        double chisqinv = chisq_inv_p_i(i, q);
        X(i-ap.first, 0) = 1.0;
        X(i-ap.first, 1) = chisqinv;
        X(i-ap.first, 2) = chisqinv*chisqinv;

        y(i-ap.first) = quant[i];
    }

    double chisqinv_actual = quantile(boost::math::chi_squared_distribution<double>(q*q), test_level);
    RowVector3d data;
    data(0) = 1.0;
    data(1) = chisqinv_actual;
    data(2) = chisqinv_actual*chisqinv_actual;

    // Get the fitted value from the regression using the inverse of our actual test level
    auto Xt = X.transpose();
    double fitted = data * (((Xt * X).inverse()) * Xt * y);

    // Negative critical values are impossible; if we somehow got a negative prediction, truncate it
    if (fitted < 0) fitted = 0;
    return fitted;
}

}
