#include "gtest/gtest.h"
#include "cpu.h"

TEST(Cpu, MemReadWrite8) {
    Cpu cpu;
    ASSERT_EQ(0, cpu.mem.read_8(0x3333));
    cpu.mem.write_8(0x3333, 121);
    ASSERT_EQ(121, cpu.mem.read_8(0x3333));
}

TEST(Cpu, MemReadWrite16) {
    Cpu cpu;
    ASSERT_EQ(0, cpu.mem.read_16(0x5555));
    cpu.mem.write_16(0x5555, 0x1234);
    // check for little endianness
    // least significant byte in the smallest address
    ASSERT_EQ(0x34, cpu.mem.read_8(0x5555));
    ASSERT_EQ(0x12, cpu.mem.read_8(0x5556));
    ASSERT_EQ(0x1234, cpu.mem.read_16(0x5555));
}



TEST(Cpu, SBC_Underflow) {
    Cpu cpu;
    ASSERT_EQ(0, cpu.P.read());
    cpu.i_sec();  // set carry instruction
    cpu.A.write(0x01);
    cpu.i_sbc(2);
    ASSERT_EQ(0xff, cpu.A.read());
    ASSERT_FALSE(cpu.P.has_carry());
    ASSERT_FALSE(cpu.P.has_zero());
    ASSERT_TRUE(cpu.P.has_negative());
    ASSERT_FALSE(cpu.P.has_overflow());
}

TEST(Cpu, SBC_NoUnderflow) {
    Cpu cpu;
    ASSERT_EQ(0, cpu.P.read());
    cpu.i_sec();  // set carry instruction
    cpu.A.write(0x01);
    cpu.i_sbc(0x01);
    ASSERT_EQ(0, cpu.A.read());
    ASSERT_TRUE(cpu.P.has_carry());
    ASSERT_TRUE(cpu.P.has_zero());
    ASSERT_FALSE(cpu.P.has_negative());
    ASSERT_FALSE(cpu.P.has_overflow());
}

TEST(Cpu, SBC_UnderflowNoCarryFlag) {
    Cpu cpu;
    ASSERT_EQ(0, cpu.P.read());
    // since carry flag is cleared, SBC will subtract 1 more
    // and, in this case, wrap around
    cpu.A.write(0x01);
    cpu.i_sbc(0x01);
    ASSERT_EQ(0xff, cpu.A.read());
    ASSERT_FALSE(cpu.P.has_carry());
    ASSERT_FALSE(cpu.P.has_zero());
    ASSERT_TRUE(cpu.P.has_negative());
    ASSERT_FALSE(cpu.P.has_overflow());
}

TEST(Cpu, SBC_NoUnderflowNoCarryFlag) {
    Cpu cpu;
    ASSERT_EQ(0, cpu.P.read());
    cpu.A.write(0x02);
    cpu.i_sbc(0x01);
    ASSERT_EQ(0, cpu.A.read());
    // No underflow, so the carry bit gets set
    ASSERT_TRUE(cpu.P.has_carry());
    ASSERT_TRUE(cpu.P.has_zero());
    ASSERT_FALSE(cpu.P.has_negative());
    ASSERT_FALSE(cpu.P.has_overflow());
}

TEST(Cpu, ADC_OverflowNoCarry) {
    Cpu cpu;
    // ensure flags are cleared
    ASSERT_EQ(0, cpu.P.read());
    cpu.A.write(0xff);
    cpu.i_adc(0x01);
    // 0xff + 0x01 overflows to zero
    ASSERT_EQ(0, cpu.A.read());
    ASSERT_TRUE(cpu.P.has_carry());
    ASSERT_TRUE(cpu.P.has_zero());
    ASSERT_FALSE(cpu.P.has_negative());
    ASSERT_FALSE(cpu.P.has_overflow());
}

TEST(Cpu, ADC_Overflow) {
    Cpu cpu;
    ASSERT_EQ(0, cpu.P.read());
    cpu.i_sec();  // set carry instruction
    cpu.A.write(0xff);
    cpu.i_adc(0x01);
    // 0xff + 0x01 + 1 (carry) overflows to one
    ASSERT_EQ(1, cpu.A.read());
    ASSERT_TRUE(cpu.P.has_carry());
    ASSERT_FALSE(cpu.P.has_zero());
    ASSERT_FALSE(cpu.P.has_negative());
    ASSERT_FALSE(cpu.P.has_overflow());
}

TEST(Cpu, ADC_NoOverflow) {
    Cpu cpu;
    ASSERT_EQ(0, cpu.P.read());
    cpu.i_sec();  // set carry instruction
    cpu.A.write(0x7f);
    cpu.i_adc(0x01);
    // 0x7f + 0x01 + 1 = 0x81
    ASSERT_EQ(0x81, cpu.A.read());
    ASSERT_FALSE(cpu.P.has_carry());
    ASSERT_FALSE(cpu.P.has_zero());
    ASSERT_TRUE(cpu.P.has_negative());

    // interpreting inputs as signed two's complement values, we added
    // a positive to a positive (0x7f + 0x01) and got a negative (0x81)
    // so we should see the overflow flag get set
    ASSERT_TRUE(cpu.P.has_overflow());
}

TEST(Cpu, SBC_OverflowFlag_Positive) {
    Cpu cpu;
    ASSERT_EQ(0, cpu.P.read());
    cpu.i_sec();
    cpu.A.write(0x01);  // 1
    cpu.i_sbc(0x80);      // -128
    // 1 - (-128) = 1 + 128 = 129
    // But this overflows in two's complement math:
    // 0x01 - 0x80 = 0x81 = -127
    ASSERT_EQ(0x81, cpu.A.read());
    ASSERT_TRUE(cpu.P.has_overflow());
    ASSERT_TRUE(cpu.P.has_negative());
    ASSERT_FALSE(cpu.P.has_zero());
    ASSERT_FALSE(cpu.P.has_carry());
}

TEST(Cpu, SBC_OverflowFlag_Negative) {
    Cpu cpu;
    ASSERT_EQ(0, cpu.P.read());
    cpu.i_sec();
    cpu.A.write(0x01);
    cpu.i_sbc(0x02);
    // 1 - 2 = -1 is allowed to be negative, so
    // the overflow flag should be cleared
    ASSERT_EQ(0xff, cpu.A.read());
    ASSERT_FALSE(cpu.P.has_overflow());
    ASSERT_TRUE(cpu.P.has_negative());
    ASSERT_FALSE(cpu.P.has_zero());
    ASSERT_FALSE(cpu.P.has_carry());
}

/* Don't use this for the TXS instruction, since changing the stack pointer
 * doesn't set any flags */
#define TEST_TRANSFER_INSTRUCTION(tag, instruction, src, dst, val, flags) \
    TEST(Cpu, T##src##dst##_##tag) {                        \
        Cpu cpu;                                            \
        cpu.src.write(val);                                 \
        cpu.dst.write(0);                                   \
        cpu.P.write(0);                                     \
                                                            \
        ASSERT_EQ(val, cpu.src.read());                     \
        ASSERT_EQ(0, cpu.dst.read());                       \
        ASSERT_EQ(0, cpu.P.read());                         \
                                                            \
        cpu.instruction();                                  \
        ASSERT_EQ(val, cpu.src.read());                     \
        ASSERT_EQ(val, cpu.dst.read());                     \
        ASSERT_EQ(flags, cpu.P.read());                     \
    }

/* A positive value will set neither the negative or zero flags */
#define TEST_TRANSFER_POSITIVE(instr, src, dst) \
    TEST_TRANSFER_INSTRUCTION(positive, instr, src, dst, 0x65, 0x00)

/* A zero value will set the zero flag */
#define TEST_TRANSFER_ZERO(instr, src, dst) \
    TEST_TRANSFER_INSTRUCTION(zero, instr, src, dst, 0x00, 0x02)

/* A negative value will set only the negative flag (bit 7) */
#define TEST_TRANSFER_NEGATIVE(instr, src, dst) \
    TEST_TRANSFER_INSTRUCTION(negative, instr, src, dst, 0xf1, 0x80)

TEST_TRANSFER_POSITIVE(i_tax, A, X)
TEST_TRANSFER_POSITIVE(i_txa, X, A)
TEST_TRANSFER_POSITIVE(i_tay, A, Y)
TEST_TRANSFER_POSITIVE(i_tya, Y, A)
TEST_TRANSFER_POSITIVE(i_tsx, S, X)

TEST_TRANSFER_ZERO(i_tax, A, X)
TEST_TRANSFER_ZERO(i_txa, X, A)
TEST_TRANSFER_ZERO(i_tay, A, Y)
TEST_TRANSFER_ZERO(i_tya, Y, A)
TEST_TRANSFER_ZERO(i_tsx, S, X)

TEST_TRANSFER_NEGATIVE(i_tax, A, X)
TEST_TRANSFER_NEGATIVE(i_txa, X, A)
TEST_TRANSFER_NEGATIVE(i_tay, A, Y)
TEST_TRANSFER_NEGATIVE(i_tya, Y, A)
TEST_TRANSFER_NEGATIVE(i_tsx, S, X)

