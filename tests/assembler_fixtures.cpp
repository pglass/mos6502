#include "assembler_fixtures.h"

void AbstractAssemblyFixture::assert_cpu_equal(const Cpu& cpu, uint8_t A,
                                               uint8_t X, uint8_t Y, uint8_t S,
                                               uint16_t PC, uint8_t P) {
    ASSERT_EQ(A, cpu.A.read());
    ASSERT_EQ(X, cpu.X.read());
    ASSERT_EQ(Y, cpu.Y.read());
    ASSERT_EQ(S, cpu.S.read());
    ASSERT_EQ(PC, cpu.PC.read());
    ASSERT_EQ(P, cpu.P.read());
 }

/* decrement is at address 0x02 and BNE uses an offset from the PC
 * *after* the BNE's 2 bytes are grabbed, which puts the PC at 0x10:
 *       0x02 - 0x10 = -8 = 0xf8
 */
IMPL_CODE_FIXTURE(AssemblyCodeWithLabel,
    "LDX #$08\n"            // 0x00: a2 08
    "decrement:\n"          // 0x02            -- absolute, gets relocated
    "DEX\n"                 // 0x02: ca
    "STX $0200\n"           // 0x03: 8e 00 02
    "CPX #$03\n"            // 0x06: e0 03
    "BNE decrement\n"       // 0x08: d0 f8     -- relative, doesn't change
    "STX $0201\n"           // 0x10: 8e 01 02
    "BRK\n"                 // 0x12: 00 00
)

/* notequal is first used at 0x04, but we don't see notequal defined
 * until 0x08. When the assembler goes back to compute the offset for
 * the branch, it needs to first increment the PC to 0x06 (consuming both
 * bytes of the branch instruction) then set the BNE argument to 0x02 so
 * that the branch correctly points to notequal at 0x06 + 0x02 = 0x08.
 *
 * Also checks no newline at the end of this file
 */
IMPL_CODE_FIXTURE(AssemblyWithForwardDeclaredLabel,
    "LDA #$01\n"           // 0x00: a9 01
    "CMP #$02\n"           // 0x02: c9 02
    "BNE notequal\n"       // 0x04: d0 02  -- 02 not 04
    "STA $22\n"            // 0x06: 85 22
    "notequal:\n"          // 0x08
    "BRK"                  // 0x08: 00 00
)
