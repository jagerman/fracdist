/** @file fdpval.c
 * @brief Takes test stat, q value, b value, and constant (1 or 0), outputs a p-value.
 *
 * If any invalid arguments are provided, a help message is written to stderr and the program exits
 * with a non-zero status.
 *
 * This is a simple wrapper around the fracdist_pvalue call and does not support the alternative
 * functionality available through fracdist_pvalue_advanced().
 */
#include <stdio.h>
#include <string.h>
#include "fracdist.h"

#define PRINT_ERROR(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#define HELP help(argv[0])

/** Prints a help message to stderr, returns 1 (to be returned by main()). */
int help(char *arg0) {
    fprintf(stderr,
"Usage: %s T Q B C\n\n"
"Estimates a p-value for the test statistic T.\n\n"

"T is the test statistic for which you wish the value.  All standard floating\n"
"point values are accepted (e.g. 1.2, 4.5e-3, etc.).\n\n"

"Q is the q value, which must be an integer between 1 and %d, inclusive.\n\n"

"B is the b value, which must be a double between %.3f and %.3f.\n\n"

"C must be 1, if the model is estimated with a constant, or 0 if the model has no constant.\n\n",
    arg0, fracdist_q_length, fracdist_bvalues[0], fracdist_bvalues[fracdist_b_length-1]);
    return 2;
}

int main(int argc, char *argv[]) {
    double test, b;
    unsigned int q;
    unsigned char constant;
    if (argc == 5) {
        int pos;
        int scanned = sscanf(argv[1], "%lg%n", &test, &pos);
        if (scanned == EOF || pos < strlen(argv[1])) {
            PRINT_ERROR("Invalid test statistic ``%s''", argv[1]);
            return HELP;
        }
        scanned = sscanf(argv[2], "%u%n", &q, &pos);
        if (scanned == EOF || pos < strlen(argv[2]) || q < 1 || q > fracdist_q_length) {
            PRINT_ERROR("Invalid q value ``%s''", argv[2]);
            return HELP;
        }
        scanned = sscanf(argv[3], "%lg%n", &b, &pos);
        if (scanned == EOF || pos < strlen(argv[3]) || b < fracdist_b_min || b > fracdist_b_max) {
            PRINT_ERROR("Invalid b value ``%s''", argv[3]);
            return HELP;
        }
        if (strcmp("0", argv[4]) == 0) {
            constant = 0;
        }
        else if (strcmp("1", argv[4]) == 0) {
            constant = 1;
        }
        else {
            PRINT_ERROR("Invalid constant value ``%s''", argv[4]);
            return HELP;
        }
    }
    else {
        return HELP;
    }


    fracdist_result r = fracdist_pvalue(test, q, b, constant);
    if (!r.error) {
        printf("%.6f\n", r.result);
    }
    else {
        // We shouldn't really get here as the argument checking above should have caught these
        PRINT_ERROR(
            r.error == fracdist_error_bvalue ? "Invalid b value" :
            r.error == fracdist_error_qvalue ? "Invalid q value" :
            "An unknown error occured");
        return HELP;
    }
}
