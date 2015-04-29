#include <sstream>
#include "gtest/gtest.h"
#include "cpu.h"
#include "opcodes.h"
#include "assembler.h"
#include "assembler_fixtures.h"

TEST(PReg, CarryBit) {
    PReg preg;
    ASSERT_EQ(0, preg.read());
    preg.set_carry();
    ASSERT_EQ(1, preg.read());
    ASSERT_TRUE(preg.has_carry());
    ASSERT_FALSE(preg.has_zero());
    ASSERT_FALSE(preg.has_interrupt());
    ASSERT_FALSE(preg.has_bcd());
    ASSERT_FALSE(preg.has_breakpoint());
    ASSERT_FALSE(preg.has_overflow());
    ASSERT_FALSE(preg.has_negative());
    preg.clear_carry();
    ASSERT_FALSE(preg.has_carry());
    ASSERT_FALSE(preg.has_zero());
    ASSERT_FALSE(preg.has_interrupt());
    ASSERT_FALSE(preg.has_bcd());
    ASSERT_FALSE(preg.has_breakpoint());
    ASSERT_FALSE(preg.has_overflow());
    ASSERT_FALSE(preg.has_negative());
    ASSERT_EQ(0, preg.read());
}

TEST(PReg, SignBit) {
    PReg preg;
    ASSERT_EQ(0, preg.read());
    preg.set_negative();
    ASSERT_EQ(0x80, preg.read());
    ASSERT_FALSE(preg.has_carry());
    ASSERT_FALSE(preg.has_zero());
    ASSERT_FALSE(preg.has_interrupt());
    ASSERT_FALSE(preg.has_bcd());
    ASSERT_FALSE(preg.has_breakpoint());
    ASSERT_FALSE(preg.has_overflow());
    ASSERT_TRUE(preg.has_negative());
    preg.clear_negative();
    ASSERT_EQ(preg.read(), 0);
    ASSERT_FALSE(preg.has_carry());
    ASSERT_FALSE(preg.has_zero());
    ASSERT_FALSE(preg.has_interrupt());
    ASSERT_FALSE(preg.has_bcd());
    ASSERT_FALSE(preg.has_breakpoint());
    ASSERT_FALSE(preg.has_overflow());
    ASSERT_FALSE(preg.has_negative());
}

TEST(OpInfo, NameEquals) {
    OpInfo opinfo("ABC", 1, 2, ACC);
    ASSERT_FALSE(opinfo.has_name("A"));
    ASSERT_FALSE(opinfo.has_name("AB"));
    ASSERT_TRUE(opinfo.has_name("ABC"));
    ASSERT_FALSE(opinfo.has_name("ABCD"));

}

TEST(OpInfo, DefaultConstructor) {
    OpInfo opinfo;
    ASSERT_TRUE(opinfo.is_null());
}

TEST_F(AssemblyCodeWithLabel, ExecutionOutput) {
    Assembler assembler(codetext);
    Cpu cpu;
    std::vector<uint8_t> code;
    assembler.relocate_code(0x600, code);
    cpu.load_code(code);

    // checks A, X, Y, S, PC, P
    this->assert_cpu_equal(cpu, 0, 0, 0, 0xff, 0x600, 0);

    ASSERT_EQ(0, cpu.mem.read_16(0x0200));

    cpu.mem.write_16(0xfffe, 0x1234);
    cpu.emu_loop();

    // checks A, X, Y, S, PC, P
    // the final BRK pushes 3 bytes to the stack = 0xfc
    // flags breakpoint, zero, and carry should be set = 0x13
    this->assert_cpu_equal(cpu, 0, 3, 0, 0xfc, 0x1234, 0x13);

    ASSERT_EQ(0x03, cpu.mem.read_8(0x0200));
    ASSERT_EQ(0x03, cpu.mem.read_8(0x0201));
}

TEST_F(AssemblyWithForwardDeclaredLabel, ExecutionOutput) {
    Assembler assembler(codetext);
    Cpu cpu;
    std::vector<uint8_t> code;
    assembler.relocate_code(0x600, code);
    cpu.load_code(code);

    // checks A, X, Y, S, PC, P
    this->assert_cpu_equal(cpu, 0, 0, 0, 0xff, 0x600, 0);

    cpu.emu_step();
    this->assert_cpu_equal(cpu, 1, 0, 0, 0xff, 0x602, 0);

    cpu.emu_step();
    this->assert_cpu_equal(cpu, 1, 0, 0, 0xff, 0x604, 0x80);

    cpu.emu_step();
    this->assert_cpu_equal(cpu, 1, 0, 0, 0xff, 0x608, 0x80);

    cpu.emu_step();
    ASSERT_TRUE(cpu.P.has_breakpoint());
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

