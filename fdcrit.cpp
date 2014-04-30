/** @file fdcrit.cpp
 * @brief Takes q value, b value, constant (1 or 0), and test levels, outputs critical values.
 *
 * If any invalid arguments are provided, a help message is written to stderr and the program exits
 * with a non-zero status.
 *
 * This is a simple wrapper around the fracdist_critical call and does not support the alternative
 * functionality available through fracdist::critical_advanced().
 */
#include <fracdist/critical.hpp>
#include "cli-common.hpp"

/** Prints a help message to stderr, returns 1 (to be returned by main()). */
int help(const char *arg0) {
    fprintf(stderr, "\n"
"Usage: %s Q B C P [P ...] [--linear|-l]\n\n"
"Estimates a p-value for the test statistic(s) T.\n\n"

"Q is the q value, which must be an integer between 1 and %zd, inclusive.\n\n"

"B is the b value, which must be a double between %.3f and %.3f.\n\n"

"C indicates whether the model has a constant or not.  0, F, or FALSE indicate\n"
"no constant; 1, T, or TRUE indicate a constant.\n\n"

"P values are the test levels for which you wish to calculate a critical value.\n"
"All standard floating point values are accepted (e.g. 0.2, 4.5e-3, etc.).  At\n"
"least one test level is required, and all test levels must satisfy 0 <= P <= 1.\n\n"

"Critical values will be output one-per-line in the same order as the given\n"
"values of P\n\n"

"If the optional --linear (or -l) argument is given, linear interpolation of the\n"
"two closest dataset B values is used and exact values are used for exact B\n"
"value matches.  The default, when --linear is not given, uses a quadratic\n"
"approximation of nearby B values (even when the value of B exactly matches the\n"
"data set).\n\n",

    arg0, fracdist::q_length, fracdist::bvalues.front(), fracdist::bvalues.back());
    print_version("fdcrit");
    return 2;
}

int main(int argc, char *argv[]) {
    double b;
    std::list<double> levels;
    unsigned int q;
    bool constant;

    std::list<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }

    if (arg_match(args, {"--help", "-h", "-?"}))
        return help(argv[0]);
    else if (arg_match(args, {"--version", "-v"}))
        return print_version("fdcrit");

    bool linear_interp = arg_remove(args, {"--linear", "-l"});

    if (args.size() >= 4) {
        bool success;

        PARSE_Q_B_C;

        while (not args.empty()) {
            std::string arg = args.front();
            args.pop_front();
            double d;
            success = parse_double(arg.c_str(), d);
            if (not success)
                RETURN_ERROR("Invalid test level ``%s''", arg.c_str());
            if (d < 0 || d > 1)
                RETURN_ERROR("Invalid test level ``%s'': value must be between 0 and 1", arg.c_str());
            levels.push_back(d);
        }
    }

    // No arguments; output the help.
    else if (args.empty()) {
        return help(argv[0]);
    }
    else {
        RETURN_ERROR("Invalid arguments");
    }

    for (auto &d : levels) {
        double r;
        try {
            r = fracdist::critical_advanced(d, q, b, constant,
                    linear_interp ? fracdist::interpolation::linear : fracdist::interpolation::JGMMON14, 9);
        } catch (std::exception &e) {
            RETURN_ERROR("An error occured: %s", e.what());
        }

        if (std::isinf(r))
            printf("inf\n");
        else
            printf("%.7g\n", r);
    }
}
