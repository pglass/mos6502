#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>
#include <cstring>
#include <string>
#include <algorithm>

enum AddressMode {
    ACC,    // accumulator
    IMM,    // immediate
    ZP,     // zero page
    ZPX,    // zero page, x
    ZPY,    // zero page, y
    ABS,    // absolute
    ABSX,   // absolute, x
    ABSY,   // absolute, y
    IND,    // indirect (only for JMP)
    INDX,   // indirect, x
    INDY,   // indirect, y
    REL,    // relative
    IMP,    // implied
};

const char* addr_mode_to_string(AddressMode mode);

class OpInfo {
    static const size_t NAME_LEN = 4;
public:
    static std::string to_lower(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            result.push_back(tolower(s[i]));
        }
        return result;
    }

    /* Default constructor. This will return an object for
     * which op_info.is_null() is true
     */
    OpInfo() : n_bytes(0), n_cycles(0), address_mode((AddressMode) -1) {
        this->set_name("nil");
    }

    OpInfo(const char n[NAME_LEN], int bytes, int cycles, AddressMode mode)
        : n_bytes(bytes), n_cycles(cycles), address_mode(mode) {
        this->set_name(n);
    }

    inline bool is_null() const { return this->has_name("nil"); }

    bool has_name(const std::string& n) const {
        return to_lower(n) == to_lower(this->name);
    }

private:
    void set_name(const char n[NAME_LEN]) {
        std::strncpy(this->name, n, NAME_LEN);
    }

public:
    char name[NAME_LEN];
    const int n_bytes;
    const int n_cycles;
    const AddressMode address_mode;
};


/* Our array of opcode definitions */
const int OPS_SIZE = 0x100;
const OpInfo OPS[OPS_SIZE] = {
#define OP(a, b, c, d) OpInfo((a), (b), (c), (d))
#define NONE OpInfo()
    /* 0x00 - 0x0F */
    // on the BRK instruction:
    //   http://nesdev.com/the%20%27B%27%20flag%20&%20BRK%20instruction.txt
    OP("BRK", 2, 7, IMP), OP("ORA", 2, 6, INDX), NONE,                 NONE,
    NONE,                 OP("ORA", 2, 3, ZP),   OP("ASL", 2, 5, ZP),  NONE,
    OP("PHP", 1, 3, IMP), OP("ORA", 2, 2, IMM),  OP("ASL", 1, 2, ACC), NONE,
    NONE,                 OP("ORA", 3, 4, ABS),  OP("ASL", 3, 6, ABS), NONE,

    /* 0x10 - 0x1F */
    OP("BPL", 2, 2, REL), OP("ORA", 2, 5, INDY), NONE,                  NONE,
    NONE,                 OP("ORA", 2, 4, ZPX),  OP("ASL", 2, 6, ZPX),  NONE,
    OP("CLC", 1, 2, IMP), OP("ORA", 3, 4, ABSY), NONE,                  NONE,
    NONE,                 OP("ORA", 3, 4, ABSX), OP("ASL", 3, 7, ABSX), NONE,

    /* 0x20 - 0x2F */
    OP("JSR", 3, 6, ABS), OP("AND", 2, 6, INDX), NONE,                 NONE,
    OP("BIT", 2, 3, ZP),  OP("AND", 2, 3, ZP),   OP("ROL", 2, 5, ZP),  NONE,
    OP("PLP", 1, 4, IMP), OP("AND", 2, 2, IMM),  OP("ROL", 1, 2, ACC), NONE,
    OP("BIT", 3, 4, ABS), OP("AND", 3, 4, ABS),  OP("ROL", 3, 6, ABS), NONE,

    /* 0x30 - 0x3F */
    OP("BMI", 2, 2, REL), OP("AND", 2, 5, INDY), NONE,                  NONE,
    NONE,                 OP("AND", 2, 4, ZPX),  OP("ROL", 2, 6, ZPX),  NONE,
    OP("SEC", 1, 2, IMP), OP("AND", 3, 4, ABSY), NONE,                  NONE,
    NONE,                 OP("AND", 3, 4, ABSX), OP("ROL", 3, 7, ABSX), NONE,

    /* 0x40 - 0x4F */
    OP("RTI", 1, 6, IMP), OP("EOR", 2, 6, INDX), NONE,                 NONE,
    NONE,                 OP("EOR", 2, 3, ZP),   OP("LSR", 2, 5, ZP),  NONE,
    OP("PHA", 1, 3, IMP), OP("EOR", 2, 2, IMM),  OP("LSR", 1, 2, ACC), NONE,
    OP("JMP", 3, 3, ABS), OP("EOR", 3, 4, ABS),  OP("LSR", 3, 6, ABS), NONE,

    /* 0x50 - 0x5F */
    OP("BVC", 2, 2, REL), OP("EOR", 2, 5, INDY), NONE,                  NONE,
    NONE,                 OP("EOR", 2, 4, ZPX),  OP("LSR", 2, 6, ZPX),  NONE,
    OP("CLI", 1, 2, IMP), OP("EOR", 3, 4, ABSY), NONE,                  NONE,
    NONE,                 OP("EOR", 3, 4, ABSX), OP("LSR", 3, 7, ABSX), NONE,

    /* 0x60 - 0x6F */
    OP("RTS", 1, 6, IMP), OP("ADC", 2, 6, INDX), NONE,                 NONE,
    NONE,                 OP("ADC", 2, 3, ZP),   OP("ROR", 2, 5, ZP),  NONE,
    OP("PLA", 1, 4, IMP), OP("ADC", 2, 2, IMM),  OP("ROR", 1, 2, ACC), NONE,
    OP("JMP", 3, 5, IND), OP("ADC", 3, 4, ABS),  OP("ROR", 3, 6, ABS), NONE,

    /* 0x70 - 0x7F */
    OP("BVS", 2, 2, REL), OP("ADC", 2, 5, INDY), NONE,                  NONE,
    NONE,                 OP("ADC", 2, 4, ZPX),  OP("ROR", 2, 6, ZPX),  NONE,
    OP("SEI", 1, 2, IMP), OP("ADC", 3, 4, ABSY), NONE,                  NONE,
    NONE,                 OP("ADC", 3, 4, ABSX), OP("ROR", 3, 7, ABSX), NONE,

    /* 0x80 - 0x8F */
    NONE,                 OP("STA", 2, 6, INDX), NONE,                 NONE,
    OP("STY", 2, 3, ZP),  OP("STA", 2, 3, ZP),   OP("STX", 2, 3, ZP),  NONE,
    OP("DEY", 1, 2, IMP), NONE,                  OP("TXA", 1, 2, IMP), NONE,
    OP("STY", 3, 4, ABS), OP("STA", 3, 4, ABS),  OP("STX", 3, 4, ABS), NONE,

    /* 0x90 - 0x9F */
    OP("BCC", 2, 2, REL), OP("STA", 2, 6, INDY), NONE,                 NONE,
    OP("STY", 2, 4, ZPX), OP("STA", 2, 4, ZPX),  OP("STX", 2, 4, ZPY), NONE,
    OP("TYA", 1, 2, IMP), OP("STA", 3, 5, ABSY), OP("TXS", 1, 2, IMP), NONE,
    NONE,                 OP("STA", 3, 5, ABSX), NONE,                 NONE,

    /* 0xA0 - 0xAF */
    OP("LDY", 2, 2, IMM), OP("LDA", 2, 6, INDX), OP("LDX", 2, 2, IMM), NONE,
    OP("LDY", 2, 3, ZP),  OP("LDA", 2, 3, ZP),   OP("LDX", 2, 3, ZP),  NONE,
    OP("TAY", 1, 2, IMP), OP("LDA", 2, 2, IMM),  OP("TAX", 1, 2, IMP), NONE,
    OP("LDY", 3, 4, ABS), OP("LDA", 3, 4, ABS),  OP("LDX", 3, 4, ABS), NONE,

    /* 0xB0 - 0xBF */
    OP("BCS", 2, 2, REL),  OP("LDA", 2, 5, INDY), NONE,                  NONE,
    OP("LDY", 2, 4, ZPX),  OP("LDA", 2, 4, ZPX),  OP("LDX", 2, 4, ZPY),  NONE,
    OP("CLV", 1, 2, IMP),  OP("LDA", 3, 4, ABSY), OP("TSX", 1, 2, IMP),  NONE,
    OP("LDY", 3, 4, ABSX), OP("LDA", 3, 4, ABSX), OP("LDX", 3, 4, ABSY), NONE,

    /* 0xC0 - 0xCF */
    OP("CPY", 2, 2, IMM), OP("CMP", 2, 6, INDX), NONE,                 NONE,
    OP("CPY", 2, 3, ZP),  OP("CMP", 2, 3, ZP),   OP("DEC", 2, 5, ZP),  NONE,
    OP("INY", 1, 2, IMP), OP("CMP", 2, 2, IMM),  OP("DEX", 1, 2, IMP), NONE,
    OP("CPY", 3, 4, ABS), OP("CMP", 3, 4, ABS),  OP("DEC", 3, 6, ABS), NONE,

    /* 0xD0 - 0xDF */
    OP("BNE", 2, 2, REL), OP("CMP", 2, 5, INDY), NONE,                  NONE,
    NONE,                 OP("CMP", 2, 4, ZPX),  OP("DEC", 2, 6, ZPX),  NONE,
    OP("CLD", 1, 2, IMP), OP("CMP", 3, 4, ABSY), NONE,                  NONE,
    NONE,                 OP("CMP", 3, 4, ABSX), OP("DEC", 3, 7, ABSX), NONE,

    /* 0xE0 - 0xEF */
    OP("CPX", 2, 2, IMM), OP("SBC", 2, 6, INDX), NONE,                 NONE,
    OP("CPX", 2, 3, ZP),  OP("SBC", 2, 3, ZP),   OP("INC", 2, 5, ZP),  NONE,
    OP("INX", 1, 2, IMP), OP("SBC", 2, 2, IMM),  OP("NOP", 1, 2, IMP), NONE,
    OP("CPX", 3, 4, ABS), OP("SBC", 3, 4, ABS),  OP("INC", 3, 6, ABS), NONE,

    /* 0xF0 - 0xFF */
    OP("BEQ", 2, 2, REL), OP("SBC", 2, 5, INDY), NONE,                  NONE,
    NONE,                 OP("SBC", 2, 4, ZPX),  OP("INC", 2, 6, ZPX),  NONE,
    OP("SED", 1, 2, IMP), OP("SBC", 3, 4, ABSY), NONE,                  NONE,
    NONE,                 OP("SBC", 3, 4, ABSX), OP("INC", 3, 7, ABSX), NONE

#undef OP
#undef NONE
};


#endif // OPCODES_H
