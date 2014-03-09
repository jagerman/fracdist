/** @file fdcrit.cpp
 * @brief Takes q value, b value, constant (1 or 0), and test levels, outputs critical values.
 *
 * If any invalid arguments are provided, a help message is written to stderr and the program exits
 * with a non-zero status.
 *
 * This is a simple wrapper around the fracdist_critical call and does not support the alternative
 * functionality available through fracdist::critical_advanced().
 */
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <fracdist/critical.hpp>
#include "parse-vals.hpp"
#include <iostream>

#define HELP help(argv[0])
#define ERROR(fmt, ...) error_with_help(fmt, argv[0], ##__VA_ARGS__)

/** Prints a help message to stderr, returns 1 (to be returned by main()). */
int help(const char *arg0) {
    fprintf(stderr, "\n"
"Usage: %s Q B C P [P ...]\n\n"
"Estimates a p-value for the test statistic(s) T.\n\n"

"Q is the q value, which must be an integer between 1 and %zd, inclusive.\n\n"

"B is the b value, which must be a double between %.3f and %.3f.\n\n"

"C indicates whether the model has a constant or not.  0, F, or FALSE indicate\n"
"no constant; 1, T, or TRUE indicate a constant.\n\n"

"P values are the test levels for which you wish to calculate a critical value.\n"
"All standard floating point values are accepted (e.g. 0.2, 4.5e-3, etc.).  At\n"
"least one test level is required, and all test levels must satisfy 0 <= P <= 1.\n\n"

"Critical values will be output one-per-line in the same order as the given\n"
"values of P\n\n",

    arg0, fracdist::q_length, fracdist::bvalues.front(), fracdist::bvalues.back());
    return 2;
}

int error_with_help(const char *errfmt, const char* arg0, ...) {
    va_list arg;
    va_start(arg, arg0);
    fprintf(stderr, "\n");
    vfprintf(stderr, errfmt, arg);
    va_end(arg);
    help(arg0);
    return 3;
}

int main(int argc, char *argv[]) {
    double b;
    std::vector<double> levels;
    size_t num_levels;
    unsigned int q;
    bool constant;
    if (argc >= 5) {
        int fail;
        num_levels = argc-4;

        PARSE_Q_B_C;

        for (size_t i = 0; i < num_levels; i++) {
            double d;
            fail = parse_double(argv[4+i], d);
            if (fail)
                return ERROR("Invalid test level ``%s''", argv[4+i]);
            if (d < 0 || d > 1)
                return ERROR("Invalid test level ``%s'': value must be between 0 and 1", argv[4+i]);
            levels.push_back(d);
        }
    }
    else {
        return ERROR("Invalid number of arguments");
    }


    for (auto &d : levels) {
        double r;
        try {
            r = fracdist::critical(d, q, b, constant);
        } catch (std::exception &e) {
            return ERROR("An error occured: %s", e.what());
        }

        if (std::isinf(r))
            printf("inf\n");
        else
            printf("%.7g\n", r);
    }
}
