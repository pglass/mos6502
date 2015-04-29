#include "assembler.h"

std::string Assembler::ignore_str("");

std::string int_to_string(int x) {
    std::stringstream ss;
    ss << x;
    return ss.str();
}

void _assert(bool expr, const char *expr_str, int line_number, const char* filename) {
    if (!expr) {
        std::cerr << "Line " << line_number << " of '" << filename << "':" << std::endl
                  << "  Assertion failure: " << expr_str << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

