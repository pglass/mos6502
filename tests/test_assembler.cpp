#include <sstream>
#include "gtest/gtest.h"
#include "assembler.h"
#include "assembler_fixtures.h"


TEST_F(AssemblyCodeWithLabel, Assembly) {
    Assembler assembler(codetext);
    ASSERT_EQ("a208ca8e0002e003d0f88e01020000", assembler.get_code_hex());
}


TEST_F(AssemblyCodeWithLabel, Relocation_BaseAddrZero) {
    Assembler assembler(codetext);
    std::vector<uint8_t> relocated_code;
    assembler.relocate_code(0x0000, relocated_code);
    ASSERT_EQ(assembler.code.size(), relocated_code.size());
    ASSERT_EQ("a208ca8e0002e003d0f88e01020000",
              Assembler::get_code_hex(relocated_code));
}

TEST_F(AssemblyCodeWithLabel, Relocation_BaseAddr1234) {
    Assembler assembler(codetext);
    std::vector<uint8_t> relocated_code;
    assembler.relocate_code(0x1234, relocated_code);
    ASSERT_EQ(assembler.code.size(), relocated_code.size());
    // there should be no change after the relocation, because there are no
    // absolute references to the decrement label.
    ASSERT_EQ("a208ca8e0002e003d0f88e01020000",
              Assembler::get_code_hex(relocated_code));
}

TEST_F(AssemblyWithForwardDeclaredLabel, Assembly) {
    Assembler assembler(codetext);
    ASSERT_EQ("a901c902d00285220000", assembler.get_code_hex());
}

