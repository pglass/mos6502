#ifndef ASSEMBLER_FIXTURES_H
#define ASSEMBLER_FIXTURES_H
#include "gtest/gtest.h"
#include "cpu.h"

class AbstractAssemblyFixture : public testing::Test {
public:
    void assert_cpu_equal(const Cpu& cpu, uint8_t A, uint8_t X, uint8_t Y,
                          uint8_t S, uint16_t PC, uint8_t P);
protected:
    virtual void SetUp() = 0;
    std::stringstream codetext;
};

/* Use in header file */
#define DECL_CODE_FIXTURE(class_name)                       \
    class class_name : public AbstractAssemblyFixture {     \
    protected:                                              \
        virtual void SetUp();                               \
    }

/* Use in source file */
#define IMPL_CODE_FIXTURE(class_name, code_str) \
    void class_name::SetUp() { codetext << code_str; }

DECL_CODE_FIXTURE(AssemblyCodeWithLabel);
DECL_CODE_FIXTURE(AssemblyWithForwardDeclaredLabel);

#endif // ASSEMBLER_FIXTURES_H
