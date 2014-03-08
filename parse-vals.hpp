#pragma once
#include <cmath>
#include <cerrno>
#include <climits>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Calls a strtod, strtoul, etc. type function and returns 1 if parse fails.  parsed will be set to
// the parsed value after this macro's code.
#define PARSE(TYPE, strtotype, ...) \
    errno = 0;\
    char *endptr;\
    TYPE parsed = strtotype(arg, &endptr, ##__VA_ARGS__);\
    if (errno || endptr[0] != 0) {\
        return 1;\
    }

// Parses a double.  Returns 0 if the entire argument could be parsed as a double, 1 otherwise.
inline int parse_double(const char *arg, double &result) {
    PARSE(double, strtod);
    result = parsed;
    return 0;
}

// Parses an unsigned int.  0 if the entire argument could be parsed as an unsigned int, 1 otherwise.
inline int parse_uint(const char *arg, unsigned int &result) {
    PARSE(unsigned long, strtoul, 10);
    if (parsed > UINT_MAX) {
        return 1;
    }
    result = (unsigned int) parsed;
    return 0;
}

// Parses a boolean value.  Accepted values: 0, 1, (case-insensitive) t, true, f, false
inline int parse_bool(const char *arg, bool &result) {
    if (!strcmp("1", arg) || !strcasecmp("TRUE", arg) || !strcasecmp("T", arg)) {
        result = true;
        return 0;
    }
    else if (!strcmp("0", arg) || !strcasecmp("FALSE", arg) || !strcasecmp("F", arg)) {
        result = false;
        return 0;
    }
    return 1;
}

#undef PARSE

#define PARSE_Q_B_C \
        fail = parse_uint(argv[1], q);\
        if (fail || q < 1 || q > fracdist::q_length)\
            return ERROR("Invalid q value ``%s''", argv[1]);\
\
        fail = parse_double(argv[2], b);\
        if (fail || b < fracdist::bvalues.front() || b > fracdist::bvalues.back())\
            return ERROR("Invalid b value ``%s''", argv[2]);\
\
        fail = parse_bool(argv[3], constant);\
        if (fail)\
            return ERROR("Invalid constant value ``%s''", argv[3]);
