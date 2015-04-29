#ifndef CPU_H
#define CPU_H

#include <iostream>
#include <stdio.h>
#include <vector>
#include "reg.h"
#include "mem.h"
#include "nullstream.h"

inline int16_t _add_signed(int16_t a, int16_t b, int16_t c) {
    return a + b + c;
}

inline address_t from_base_offset(const address_t base,
                                  const address_t offset) {
    return base + offset;
}

class Cpu {
public:
    static const address_t STACK_BOTTOM = 0x0100;

    /* All messages are printed to the given out_stream. By default, output is
     * disabled. Pass in std::cout or std::cerr or an ofstream to put the
     * output where you want.
     */
    Cpu(std::ostream& out_stream = NULLSTREAM) : out(out_stream) {
        this->S.write(0xFF);
    }

    friend std::ostream& operator<<(std::ostream& o, const Cpu& cpu) {
        return o
            << "A: " << cpu.A
            << " X: " << cpu.X
            << " Y: " << cpu.Y
            << "\nS: " << cpu.S
            << " PC: " << cpu.PC
            << "\nP: " << cpu.P
               ;
    }

    /* Load the given code at the given address */
    void load_code(const std::vector<uint8_t>& code, address_t addr = 0x0600);

    /* Return the byte of code at the PC, and increment the PC */
    uint8_t next_code_byte() {
        uint8_t result = this->mem.read_8(this->PC.read());
        this->PC.add(1);
        return result;
    }

    uint16_t next_two_code_bytes() {
        uint16_t lo = this->next_code_byte();
        uint16_t hi = this->next_code_byte();
        return ((hi << 8) & 0xff00) | (lo & 0xff);
    }

    uint8_t peek_code_byte() {
        return this->mem.read_8(this->PC.read());
    }

    uint8_t peek_two_code_bytes() const {
        uint16_t lo = this->mem.read_8(this->PC.read());
        uint16_t hi = this->mem.read_8(this->PC.read() + 1);
        return ((hi << 8) & 0xff00) | (lo & 0xff);
    }

    void emu_loop();
    int emu_step();

    /** STACK OPERATIONS **/
    inline address_t _get_stack_top() {
        return STACK_BOTTOM + this->S.read();
    }

    inline void _adjust_stack_pointer(int amount) {
        this->S.write(this->_get_stack_top() + amount);
    }

    /* Push the given value to the top of the stack */
    void push_8(const uint8_t val);
    void push_16(const uint16_t val);

    /* Pop and return a value from the stack */
    uint8_t pop_8();
    uint16_t pop_16();

    inline void push_register_8(const Reg_8& reg) { this->push_8(reg.read()); }
    inline void push_register_16(const Reg_16& reg) { this->push_16(reg.read()); }

    inline void pop_register_8(Reg_8& reg) { reg.write(this->pop_8()); }
    inline void pop_register_16(Reg_16& reg) { reg.write(this->pop_16()); }

    void _set_zero_and_neg_flags(uint8_t val);
    void _set_addition_carry_flag(uint16_t sum);
    void _set_subtraction_carry_flag(int8_t diff);

    /* Load operations:
     *      LDA, LDX, LDY - load a value into A, X, or Y
     * These operations set the zero and negative flags based
     * on the value loaded into memory.
     */
    void i_lda(const uint8_t val);
    void i_ldx(const uint8_t val);
    void i_ldy(const uint8_t val);

    inline void i_lda_mem(const address_t addr) { this->i_lda(this->mem.read_8(addr)); }
    inline void i_ldx_mem(const address_t addr) { this->i_ldx(this->mem.read_8(addr)); }
    inline void i_ldy_mem(const address_t addr) { this->i_ldy(this->mem.read_8(addr)); }

    /* Store operations:
     *      STA - store A in memory
     *      STX - store X in memory
     *      STY - store Y in memory
     */
    inline void i_sta(address_t addr) { this->mem.write_8(addr, this->A.read()); }
    inline void i_stx(address_t addr) { this->mem.write_8(addr, this->X.read()); }
    inline void i_sty(address_t addr) { this->mem.write_8(addr, this->Y.read()); }

    /* Transfer operations:
     *      TAX - transfer A to X
     *      TAY - transfer A to Y
     *      TXA - transfer X to A
     *      TYA - transfer Y to A
     *      TSX - transfer S to X
     *      TXS - transfer X to S (affects no flags)
     * These set the zero and negative flags as appropriate, except for TXS)
     */
    void _do_transfer(const Reg_8& src, Reg_8& dst);
    inline void i_tax() { this->_do_transfer(this->A, this->X); }
    inline void i_tay() { this->_do_transfer(this->A, this->Y); }
    inline void i_txa() { this->_do_transfer(this->X, this->A); }
    inline void i_tya() { this->_do_transfer(this->Y, this->A); }
    inline void i_tsx() { this->_do_transfer(this->S, this->X); }
    /* changing the stack register doesn't affect flags */
    inline void i_txs() { this->S.write(this->X.read()); }

    /* Stack operations:
     *      PHA - push A to the stack
     *      PHP - push P to the stack
     *      PLA - pop the stack into A (set neg and zero flags)
     *      PLP - pop the stack into P
     */
    inline void i_pha() { this->push_register_8(this->A); }
    inline void i_php() { this->push_register_8(this->P); }
    inline void i_plp() { this->pop_register_8(this->P); }

    inline void i_pla() {
        this->pop_register_8(this->A);
        this->_set_zero_and_neg_flags(this->A.read());
    }

    /* Logical operations:
     *      AND - and together A and value in memory into A
     *      EOR - xor together A and a value in memory into A
     *      ORA - or together A and a value in memory into A
     *      BIT - and together A and a value in memory, but discard the result.
     * These all set the zero and negative flags. BIT also sets the overflow flag.
     */
    void i_ora(const uint8_t val);
    void i_eor(const uint8_t val);
    void i_and(const uint8_t val);

    inline void i_ora_mem(const address_t addr) { this->i_ora(this->mem.read_8(addr)); }
    inline void i_eor_mem(const address_t addr) { this->i_eor(this->mem.read_8(addr)); }
    inline void i_and_mem(const address_t addr) { this->i_and(this->mem.read_8(addr)); }

    inline void i_ora_ind(const address_t addr) { this->i_ora_mem(this->mem.read_16(addr)); }
    inline void i_eor_ind(const address_t addr) { this->i_eor_mem(this->mem.read_16(addr)); }
    inline void i_and_ind(const address_t addr) { this->i_and_mem(this->mem.read_16(addr)); }

    void i_bit(const int8_t val);

    /* Arithmetic
     *      ADC - add with carry
     *      SBC - subtract with carry
     *      CMP - compare a value to A
     *      CPX - compare a value to X
     *      CPY - compare a value to Y
     */
    void i_adc(const int8_t val);
    void i_sbc(const int8_t val);
    void _do_compare(const int8_t a, const int8_t b);
    inline void i_cmp(const int8_t val) { this->_do_compare(this->A.read(), val); }
    inline void i_cpx(const int8_t val) { this->_do_compare(this->X.read(), val); }
    inline void i_cpy(const int8_t val) { this->_do_compare(this->Y.read(), val); }

    /* Shifts
     *      ASL - arithmetic shift left of A or a value in memory
     *      LSR - logical shift right of A or a value in memory
     *      ROL - rotate left of A or a value in memory
     *      ROR - rotate right of A or a value in memory
     * These update the negative and zero flags.
     * Shifts shift into the carry and shift zero in to empty spots.
     * Rotates shift into the carry and shift the carry in to empty spots.
     */
    uint8_t i_asl(const uint8_t val);
    uint8_t i_lsr(const uint8_t val);
    uint8_t i_rol(const uint8_t val);
    uint8_t i_ror(const uint8_t val);

    inline void i_asl_a() { this->A.write(this->i_asl(this->A.read())); }
    inline void i_lsr_a() { this->A.write(this->i_lsr(this->A.read())); }
    inline void i_ror_a() { this->A.write(this->i_ror(this->A.read())); }
    inline void i_rol_a() { this->A.write(this->i_rol(this->A.read())); }

    inline void i_asl_mem(address_t addr) {
        this->mem.write_8(addr, this->i_asl(this->mem.read_8(addr)));
    }
    inline void i_lsr_mem(address_t addr) {
        this->mem.write_8(addr, this->i_lsr(this->mem.read_8(addr)));
    }
    inline void i_ror_mem(address_t addr) {
        this->mem.write_8(addr, this->i_ror(this->mem.read_8(addr)));
    }
    inline void i_rol_mem(address_t addr) {
        this->mem.write_8(addr, this->i_rol(this->mem.read_8(addr)));
    }

    /* Jumps and calls
     *      JMP - jump to an address
     *      JSR - jump to subroutine
     *      RTS - return from subroutine
     */

    /* TODO? "An original 6502 has does not correctly fetch the target address
     * if the indirect vector falls on a page boundary (e.g. $xxFF where xx is
     * and value from $00 to $FF). In this case fetches the LSB from $xxFF as
     * expected but takes the MSB from $xx00. This is fixed in some later
     * chips like the 65SC02 so for compatibility always ensure the indirect
     * vector is not at the end of the page." */
    inline void i_jmp(address_t addr) { this->PC.write(addr); }

    /* push the address of the *next* instruction to the stack.
     * subtract one to support clever jump table tricks.
     * http://wiki.nesdev.com/w/index.php/RTS_Trick#About_JSR_and_RTS
     */
    inline void i_jsr(address_t addr, address_t next_addr) {
        this->push_16(next_addr - 1);
        i_jmp(addr);
    }

    inline void i_rts() { i_jmp(this->pop_16() + 1); }

    /* Increments/decrements
     *      INC, DEC - increment or decrement a memory location
     *      INX, DEX - increment or decrement register X
     *      INY, DEY - increment or decrement register Y
     * These all update the zero and negative flags.
     */
    void i_inc(address_t addr);
    void i_inx();
    void i_iny();
    void i_dec(address_t addr);
    void i_dex();
    void i_dey();

    /* Branching operations
     *      BCS - Branch if carry set
     *      BCC - Branch if carry clear
     *      BEQ - Branch if zero set
     *      BNE - Branch if zero clear
     *      BMI - Branch if negative set (if minus)
     *      BPL - Branch if negative clear (if positive)
     *      BVS - Branch if overflow set
     *      BVC - Branch if overflow clear
     * These all add an 8-bit displacement to the PC.
     *
     * Returns true if the branch is taken (e.g. returns the condition).
     */
    bool _do_branch(bool condition, int8_t displacement) {
        if (condition) {
            this->PC.write(this->PC.read() + displacement);
        }
        return condition;
    }

    inline bool i_bcs(int8_t displacement) {
        return this->_do_branch(this->P.has_carry(), displacement);
    }

    inline bool i_bcc(int8_t displacement) {
        return this->_do_branch(!this->P.has_carry(), displacement);
    }

    inline bool i_beq(int8_t displacement) {
        return this->_do_branch(this->P.has_zero(), displacement);
    }

    inline bool i_bne(int8_t displacement) {
        return this->_do_branch(!this->P.has_zero(), displacement);
    }

    inline bool i_bmi(int8_t displacement) {
        return this->_do_branch(this->P.has_negative(), displacement);
    }

    inline bool i_bpl(int8_t displacement) {
        return this->_do_branch(!this->P.has_negative(), displacement);
    }

    inline bool i_bvs(int8_t displacement) {
        return this->_do_branch(this->P.has_overflow(), displacement);
    }

    inline bool i_bvc(int8_t displacement) {
        return this->_do_branch(!this->P.has_overflow(), displacement);
    }

    /* Flag-related operations */
    inline void i_sec() { this->P.set_carry(); }
    inline void i_clc() { this->P.clear_carry(); }
    inline void i_sei() { this->P.set_interrupt(); }
    inline void i_cli() { this->P.clear_interrupt(); }
    inline void i_sed() { this->P.set_bcd(); }
    inline void i_cld() { this->P.clear_bcd(); }

    /* System Functions
     *      BRK - Force an interrupt
     *      NOP - No operation (implemented elsewhere)
     *      RTI - Return from interrupt
     */
    /* The BRK instruction forces the generation of an interrupt request.
     * The program counter and processor status are pushed on the stack then
     * the IRQ interrupt vector at $FFFE/F is loaded into the PC and the break
     * flag in the status set to one. */
    void i_brk() {
        this->push_register_16(this->PC);
        this->push_register_8(this->P);
        this->PC.write(this->mem.read_16(0xFFFE));
        this->P.set_breakpoint();
    }

    void i_rti() {
        this->pop_register_8(this->P);
        this->pop_register_16(this->PC);
    }

public:
    /*
     * 6502 Registers:
     *     A        8-bit accumulator registor
     *     X, Y     8-bit index registers
     *     P        8-bit processor status register
     *     S        8-bit stack pointer
     *     PC       16-bit program counter
     */
    Reg_8 A, X, Y, S;
    PReg P;
    Reg_16 PC;

    Mem mem;
private:
    std::ostream& out;
};

#endif // CPU_H
