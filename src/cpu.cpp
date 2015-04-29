#include "cpu.h"
#include "opcodes.h"

/* To ensure we don't forget a break in the big switch statement */
#define CASE(val, code) \
    case (val): { code; } break

#define DEBUG_EMU true

template <typename T>
inline void debug(const T& val) {
#ifdef DEBUG_EMU
    std::cout << val << std::endl;
#endif
}

void Cpu::load_code(const std::vector<uint8_t> &code, address_t addr) {
    size_t max_addr = code.size() + addr;
    if (max_addr >= 0xFFFA) {
        throw "code doesn't fit in memory";
    } else {
        std::copy(code.begin(), code.end(), &this->mem.data[addr]);
        this->PC.write(addr);
    }
}

void Cpu::emu_loop() {
    int n_steps = 0;
    int total_cycles = 0;
    int n_cycles;

    this->out << std::endl << "Step: " << n_steps << std::endl
              << *this << std::endl;

    do {
        n_steps += 1;
        this->out << std::endl << "Step: " << n_steps << std::endl;
        n_cycles = this->emu_step();
        total_cycles += n_cycles;

        if (this->P.has_breakpoint()) {
            // TODO: not the normal behavior -- see Cpu::brk().
            this->out << "BRK seen -- terminating" << std::endl;
            break;
        }

        this->out << "Took " << n_cycles << " cycles" << std::endl
                  << *this << std::endl;
    } while (n_cycles >= 0);
}

/* Emulate a single instruction and return the number of cycles */
int Cpu::emu_step() {

    // TODO: check interrupts?

    uint8_t next_op = this->next_code_byte();
    const OpInfo& op_info = OPS[next_op];
    if (op_info.is_null()) {
        this->out << "Unsupport op_code: 0x" << std::hex << next_op
                  << std::dec << std::endl;
        return -1;
    } else {
        this->out << "Instruction: " << op_info.name << std::endl;

    }

    int n_cycles = op_info.n_cycles;
//    this->PC.add(op_info.n_bytes - 1);

    /* Extra cycle addition:
     *    http://users.telenet.be/kim1-6502/6502/hwman.html#AA
     */
    int extra_cycles = 0;

    /* If a page boundary is crossed in ABS, ABSY, or INDY
     * addressing modes for these instructions, add one cycle.
     *  ADC          CMP          EOR          LDY
     *  AND          CPX          LDA          ORA
     *  BIT          CPY          LDX          SBC
     */
    if ((this->peek_two_code_bytes() & 0x0F) == 0x0F
        && (op_info.address_mode == ABSX
            || op_info.address_mode == ABSY
            || op_info.address_mode == INDY)
        && (op_info.has_name("adc")
            || op_info.has_name("and")
            || op_info.has_name("bit")
            || op_info.has_name("cmp")
            || op_info.has_name("cpx")
            || op_info.has_name("cpy")
            || op_info.has_name("eor")
            || op_info.has_name("lda")
            || op_info.has_name("ldx")
            || op_info.has_name("ldy")
            || op_info.has_name("ora")
            || op_info.has_name("sbc"))) {
        extra_cycles = 1;
    }

    // TODO: extra cycle addition
//    uint16_t addr = 0x00;
    /* If a branch is not taken and a page boundary is crossed, add two cycles.
     * Else if the branch is not taken, add one cycle.
     * Else add zero cycles.
     */
    bool branch_taken = true;
    uint16_t prior_pc = this->PC.read();
    uint16_t addr;
    switch (next_op) {
        /** Break **/
        CASE(0x00, this->i_brk());

        /** Store instructions */
        CASE(0x85, this->i_sta(this->next_code_byte()));        // STA (zp)
        CASE(0x8E, this->i_stx(this->next_two_code_bytes()));   // STX (abs)

        /** Load instructions **/
        CASE(0xA2, this->i_ldx(this->next_code_byte()));  // LDX (imm)
        CASE(0xA9, this->i_lda(this->next_code_byte()));  // LDA (imm)

        /** Increment/decrement instructions **/
        CASE(0xCA, this->i_dex());

        /** Branch instructions **/
        CASE(0x10, branch_taken = this->i_bpl(this->next_code_byte()));
        CASE(0x30, branch_taken = this->i_bmi(this->next_code_byte()));
        CASE(0x50, branch_taken = this->i_bvc(this->next_code_byte()));
        CASE(0x70, branch_taken = this->i_bvs(this->next_code_byte()));
        CASE(0x90, branch_taken = this->i_bcc(this->next_code_byte()));
        CASE(0xB0, branch_taken = this->i_bcs(this->next_code_byte()));
        CASE(0xD0, branch_taken = this->i_bne(this->next_code_byte()));
        CASE(0xF0, branch_taken = this->i_beq(this->next_code_byte()));

        /** Comparison instructions **/
        CASE(0xE0, this->i_cpx(this->next_code_byte()));  // CPX (imm)
        CASE(0xC9, this->i_cmp(this->next_code_byte()));  // CMP (imm)

        /** ORA instruction **/
        CASE(0x01,  // ORA (indx)
             addr = from_base_offset(this->next_code_byte(), this->X.read());
             this->i_ora_ind(addr);
        );
        CASE(0x05,  // ORA (zp)
             this->i_ora_mem(this->next_code_byte()));
        CASE(0x09,  // ORA (imm)
             this->i_ora(this->next_code_byte()));
        CASE(0x0D,  // ORA (abs)
             this->i_ora_mem(this->next_two_code_bytes()));
        CASE(0x11,  // ORA (indy)
             addr = this->mem.read_16(this->next_code_byte());
             this->i_ora_mem(from_base_offset(addr, this->Y.read()));
        );
        CASE(0x15,  // ORA (zpx)
             addr = from_base_offset(this->next_code_byte(), this->X.read());
             this->i_ora_mem(addr);
        );
        CASE(0x19,  // ORA (absx)
             addr = from_base_offset(this->next_two_code_bytes(), this->X.read());
             this->i_ora_mem(addr);
        );
        CASE(0x1D,  // ORA (absy)
             addr = from_base_offset(this->next_two_code_bytes(), this->Y.read());
             this->i_ora_mem(addr);
        );

        default:
            std::cerr << "Unimplemented opcode 0x" << std::hex << (int) next_op
                      << std::dec << std::endl
                      << "  -- instruction " << op_info.name << std::endl;
            return -1;
    }

    /* If a branch is not taken and a page boundary is crossed, add two cycles.
     * Else if the branch is not taken, add one cycle.
     * Else add zero cycles.
     */
    if (!branch_taken && (op_info.has_name("bpl")
                          || op_info.has_name("bmi")
                          || op_info.has_name("bvc")
                          || op_info.has_name("bvs")
                          || op_info.has_name("bcc")
                          || op_info.has_name("bcs")
                          || op_info.has_name("bne")
                          || op_info.has_name("beq"))) {
        uint16_t current_pc = this->PC.read();
        if ((prior_pc & 0xFF00) != (current_pc & 0xFF00)) {
            extra_cycles = 2;
        } else {
            extra_cycles = 1;
        }
    }

    if (extra_cycles != 0) {
        this->out << "-- Added " << extra_cycles << " extra cycles";
    }

    return n_cycles + extra_cycles;
}

/* Push the given value to the top of the stack */
void Cpu::push_8(const uint8_t val) {
    this->mem.write_8(this->_get_stack_top(), val);
    this->_adjust_stack_pointer(-1);
}

void Cpu::push_16(const uint16_t val) {
    // TODO: correct byte order?
    this->_adjust_stack_pointer(-1);
    this->mem.write_16(this->_get_stack_top(), val);
    this->_adjust_stack_pointer(-1);
}

/* Pop and return a value from the stack */
uint8_t Cpu::pop_8() {
    this->_adjust_stack_pointer(1);
    return this->mem.read_8(this->_get_stack_top());
}

uint16_t Cpu::pop_16() {
    this->_adjust_stack_pointer(1);
    uint16_t val = this->mem.read_16(this->_get_stack_top());
    this->_adjust_stack_pointer(1);
    return val;
}

/* The zero flag is set if val is zero.
 * The negative flag is set if the leading bit is 1.
 */
void Cpu::_set_zero_and_neg_flags(uint8_t val) {
    if (val & 0x80) {
        this->P.set_negative();
    } else {
        this->P.clear_negative();
    }

    if (val == 0) {
        this->P.set_zero();
    } else {
        this->P.clear_zero();
    }
}

void Cpu::_set_addition_carry_flag(uint16_t sum) {
    if (sum > 0xFF) {
        this->P.set_carry();
    } else {
        this->P.clear_carry();
    }
}

void Cpu::_set_subtraction_carry_flag(int8_t diff) {
    if (diff < 0) {
        this->P.clear_carry();
    } else {
        this->P.set_carry();
    }
}

void Cpu::i_lda(const uint8_t val) {
    this->A.write(val);
    this->_set_zero_and_neg_flags(val);
}

void Cpu::i_ldx(const uint8_t val) {
    this->X.write(val);
    this->_set_zero_and_neg_flags(val);
}

void Cpu::i_ldy(const uint8_t val) {
    this->Y.write(val);
    this->_set_zero_and_neg_flags(val);
}

void Cpu::_do_transfer(const Reg_8& src, Reg_8& dst) {
    dst.write(src.read());
    this->_set_zero_and_neg_flags(dst.read());
}

void Cpu::i_and(const uint8_t val) {
    this->A.write(this->A.read() & val);
    this->_set_zero_and_neg_flags(this->A.read());
}

void Cpu::i_eor(const uint8_t val) {
    this->A.write(this->A.read() ^ val);
    this->_set_zero_and_neg_flags(this->A.read());
}

void Cpu::i_ora(const uint8_t val) {
    this->A.write(this->A.read() | val);
    this->_set_zero_and_neg_flags(this->A.read());
}

void Cpu::i_bit(const int8_t val) {
    uint8_t test_val = this->A.read() & val;
    this->_set_zero_and_neg_flags(test_val);
    if (test_val & (1 << 6)) {
        this->P.set_overflow();
    } else {
        this->P.clear_overflow();
    }
}

void Cpu::i_adc(const int8_t val) {
    if (this->P.has_bcd()) {
        throw "BCD mode not implemented";
    } else {
        const bool signs_differ = (this->A.read() ^ val) & 0x80;
        const int16_t carry = this->P.has_carry() ? 1 : 0;
        const int16_t sum = _add_signed(this->A.read(), val, carry);
        this->A.write(sum & 0xFF);

        this->_set_zero_and_neg_flags(this->A.read());

        /* if the signs of the inputs were the same, and if the sum has
         * a different sign than the inputs, then set the overflow bit
         */
        if (!signs_differ && (sum & 0x80) != (val & 0x80)) {
            this->P.set_overflow();
        } else {
            this->P.clear_overflow();
        }

        this->_set_addition_carry_flag(sum);
    }
}

void Cpu::i_sbc(const int8_t val) {
    if (this->P.has_bcd()) {
        throw "BCD mode not implemented";
    } else {
        const bool signs_differ = (this->A.read() ^ val) & 0x80;
        const int16_t not_carry = this->P.has_carry() ? 0 : 1;
        const int16_t diff = _add_signed(this->A.read(), -val, -not_carry);
        this->A.write(diff & 0xFF);

        this->_set_zero_and_neg_flags(this->A.read());

        /* subtraction flips the sign of the second operand, so if the
         * signs of the inputs were different, and if the difference
         * has the same sign as the second input, then set the overflow bit
         */
        if (signs_differ && (diff & 0x80) == (val & 0x80)) {
            this->P.set_overflow();
        } else {
            this->P.clear_overflow();
        }

        this->_set_subtraction_carry_flag(diff);
    }
}

void Cpu::_do_compare(const int8_t a, const int8_t b) {
    const int16_t diff = _add_signed(a, -b, 0);
    this->_set_zero_and_neg_flags(diff & 0xFF);
    this->_set_subtraction_carry_flag(diff);
}

/* Shift val left one bit, and update the zero and negative flags.
 * Bit 7 is placed in the carry flag (since it is shifted out). */
uint8_t Cpu::i_asl(const uint8_t val) {
    const uint8_t shifted = val << 1;
    this->_set_zero_and_neg_flags(shifted);
    if (val & 0x80) {
        this->P.set_carry();
    } else {
        this->P.clear_carry();
    }
    return shifted;
}

/* Shift val to the right one bit, and update the zero (and negative) flags.
 * Bit 0 is placed in the carry flag. */
uint8_t Cpu::i_lsr(const uint8_t val) {
    const uint8_t shifted = val >> 1;
    this->_set_zero_and_neg_flags(shifted);
    if (val & 0x01) {
        this->P.set_carry();
    } else {
        this->P.clear_carry();
    }
    return shifted;
}

/* Rotate left. The carry bit is shifted in on the left side,
 * and the rightmost bit is shifted into the carry. */
uint8_t Cpu::i_rol(const uint8_t val) {
    const uint8_t carry = this->P.has_carry() ? 1 : 0;
    const uint8_t rotated = (val << 1) | (1 & carry);
    this->_set_zero_and_neg_flags(rotated);
    if (val & 0x80) {
        this->P.set_carry();
    } else {
        this->P.clear_carry();
    }
    return rotated;
}

uint8_t Cpu::i_ror(const uint8_t val) {
    const uint8_t carry = this->P.has_carry() ? 1 : 0;
    const uint8_t rotated = (val >> 1) | (0x80 & (carry << 7));
    this->_set_zero_and_neg_flags(rotated);
    if (val & 0x01) {
        this->P.set_carry();
    } else {
        this->P.clear_carry();
    }
    return rotated;
}

void Cpu::i_inc(address_t addr) {
    uint8_t val = this->mem.read_8(addr) + 1;
    this->mem.write_8(addr, val);
    this->_set_zero_and_neg_flags(val);
}

void Cpu::i_inx() {
    this->X.write(this->X.read() + 1);
    this->_set_zero_and_neg_flags(this->X.read());
}

void Cpu::i_iny() {
    this->Y.write(this->Y.read() + 1);
    this->_set_zero_and_neg_flags(this->Y.read());
}

void Cpu::i_dec(address_t addr) {
    uint8_t val = this->mem.read_8(addr) - 1;
    this->mem.write_8(addr, val);
    this->_set_zero_and_neg_flags(val);
}

void Cpu::i_dex() {
    this->X.write(this->X.read() - 1);
    this->_set_zero_and_neg_flags(this->X.read());
}

void Cpu::i_dey() {
    this->Y.write(this->Y.read() - 1);
    this->_set_zero_and_neg_flags(this->Y.read());
}
