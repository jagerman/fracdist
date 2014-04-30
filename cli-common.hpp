#pragma once
#include <cerrno>
#include <cstring>
#include <fracdist/version.hpp>
#include <unordered_set>
#include <list>
#include <string>
#include <algorithm>
#include <cctype>
#include <utility>
#include <iostream>

#define PRINT_ERROR(fmt, ...) do { fprintf(stderr, "\n" fmt "\n\n", ##__VA_ARGS__); help(argv[0]); return 3; } while(0)
#define RETURN_ERROR(fmt, ...) do { PRINT_ERROR(fmt, ##__VA_ARGS__); return 3; } while(0)

// Calls a std::stod, std::stoul, etc. type function and returns false if the parse fails.  VAR,
// which must be predeclared, will be set to the parsed value after this macro's code.
#define PARSE(VAR, STOTYPE, ...) \
    try {\
        size_t endpos;\
        VAR = STOTYPE(arg, &endpos, ##__VA_ARGS__);\
        if (endpos != arg.length()) throw std::invalid_argument("Match did not use whole string");\
    }\
    catch (...) { return false; }

// Parses a double.  Returns true if the entire argument could be parsed as a double, false otherwise.
inline bool parse_double(const std::string &arg, double &result) {
    PARSE(result, std::stod);
    return true;
}

// Parses an unsigned int.  0 if the entire argument could be parsed as an unsigned int, 1 otherwise.
inline bool parse_uint(const std::string &arg, unsigned int &result) {
    unsigned long parsed;
    PARSE(parsed, std::stoul, 10);
    if (parsed > std::numeric_limits<unsigned int>::max())
        return false;

    result = (unsigned int) parsed;
    return true;
}

#undef PARSE

inline void uc(std::string &lc) {
    std::transform(lc.begin(), lc.end(), lc.begin(), std::ptr_fun<int, int>(std::toupper));
}

// Parses a boolean value.  Accepted values: 0, 1, (case-insensitive) t, true, f, false
// Returns true on successful parse, false on failure.
inline bool parse_bool(std::string arg, bool &result) {
    uc(arg);
    if (arg == "1" || arg == "TRUE" || arg == "T") {
        result = true;
        return true;
    }
    else if (arg == "0" || arg == "FALSE" || arg == "F") {
        result = false;
        return true;
    }
    return false;
}

// Parses the Q, B, and C values and removes them from the argument list
#define PARSE_Q_B_C \
        success = parse_uint(args.front().c_str(), q);\
        if (not success or q < 1 or q > fracdist::q_length)\
            RETURN_ERROR("Invalid q value ``%s''", args.front().c_str());\
        args.pop_front();\
\
        success = parse_double(args.front().c_str(), b);\
        if (not success or b < fracdist::bvalues.front() or b > fracdist::bvalues.back())\
            RETURN_ERROR("Invalid b value ``%s''", args.front().c_str());\
        args.pop_front();\
\
        success = parse_bool(args.front().c_str(), constant);\
        if (not success)\
            RETURN_ERROR("Invalid constant value ``%s''", args.front().c_str());\
        args.pop_front();


inline int print_version(const char *program) {
    fprintf(stderr,
"%s (fracdist) %s\n"
"Copyright (C) 2014 Jason Rhinelander\n"
"License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n\n",
    program, fracdist::version_string);
    return 10;
}

/// Returns true if the given `args` contains any of the values in `find`
inline bool arg_match(const std::list<std::string> &args, const std::unordered_set<std::string> &find) {
    for (auto &arg : args) {
        if (find.count(arg))
            return true;
    }
    return false;
}

/// Like arg_match, but removes the first matched argument from `args`.
inline bool arg_remove(std::list<std::string> &args, const std::unordered_set<std::string> &remove) {
    for (auto it = args.begin(); it != args.end(); it++) {
        if (remove.count(*it)) {
            args.erase(it);
            return true;
        }
    }
    return false;
}

