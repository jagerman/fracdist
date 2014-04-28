#include <fracdist/pvalue.hpp>
#include <boost/math/distributions/chi_squared.hpp>
#include <Eigen/Core>
#include <Eigen/Cholesky>

using Eigen::Matrix3d;
using Eigen::MatrixX3d;
using Eigen::Matrix3Xd;
using Eigen::VectorXd;
using Eigen::RowVector3d;
using Eigen::LLT;

namespace fracdist {

// See description in fracdist.h
double pvalue(const double &test_stat, const unsigned int &q, const double &b, const bool &constant) {
    return pvalue_advanced(test_stat, q, b, constant, interpolation::JGMMON14, 9);
}

// See description in fracdist.h
double pvalue_advanced(const double &test_stat, const unsigned int &q, const double &b, const bool &constant,
        const interpolation &interp_mode, const unsigned int &approx_points) {

    if (test_stat < 0)
        throw std::out_of_range(ostringstream() << "test stat (" << test_stat << ") invalid: cannot be negative");
    // The critical values for test stats of 0 or infinity are trivial: 1 or 0.
    if (test_stat == 0)
        return 1.0;
    if (std::isinf(test_stat))
        return 0.0;

    // First get the set of quantiles to use (this also checks and q and b are valid):
    auto quant = quantiles(q, b, constant, interp_mode);

    // If asked for the p-value for a value less than half the smallest quantile we have, or more
    // than double the largest quantile we have, just give back 0 or 1.
    if (test_stat < 0.5*quant.front()) return 1.0;
    if (test_stat > 2*quant.back()) return 0.0;

    // Otherwise we need to do some more work.

    // First find the location with a quantile closest to the requested one
    size_t min_at = find_closest(test_stat, quant);

    // Now figure out a set of `approx_points' consecutive points centered on the closest value
    auto ap = find_bracket(min_at, p_length-1, approx_points);

    if (ap.second - ap.first < 2)
        throw std::runtime_error(ostringstream() << "approx_points (" << approx_points << ") too small: not enough data points for quadratic approximation");

    // Now we're going to run the regression:
    // 
    //     chisqinv_i = \beta_1 + \beta_2 quantile_i + \beta_3 quantile_i^2
    //
    // where chisqinv_i is the inverse cdf at p=pvalue[i] of a chi-squared distribution with q^2 df
    //
    // The fitted value then gives us a fitted chi-squared value for which we can get a pvalue.

    MatrixX3d X(ap.second - ap.first + 1, 3);
    VectorXd y(ap.second - ap.first + 1);
    for (size_t i = ap.first; i <= ap.second; i++) {
        X(i-ap.first, 0) = 1.0;
        X(i-ap.first, 1) = quant[i];
        X(i-ap.first, 2) = quant[i] * quant[i];

        y(i-ap.first) = chisq_inv_p_i(i, q);
    }
    RowVector3d data;
    data(0) = 1.0;
    data(1) = test_stat;
    data(2) = test_stat*test_stat;

    Matrix3Xd Xt = X.transpose();
    LLT<Matrix3d> cholXtX(Xt * X);
    double fitted = data * cholXtX.solve(Xt * y);

    // A negative isn't valid, so if we predicted one anyway, truncate it at 0 (which corresponds to
    // a pvalue of 1).
    if (fitted < 0) return 1.0;

    // NB: chisq_Q gets the *upper-tail* chisq cdf (chisq_P gives the lower tail)
    boost::math::chi_squared_distribution<double> chisq(q*q);
    return cdf(complement(chisq, fitted));
}

}
