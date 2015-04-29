#ifndef REG_H
#define REG_H

#include <stdint.h>
#include <iostream>
#include <iomanip>

template <typename T>
class Reg {
public:
    Reg() : data(T(0)) {}
    inline void write(T value) { this->data = value; }

    inline T read() const { return this->data; }

    inline void add(const T amount) { this->data += amount; }

    inline friend std::ostream& operator<<(std::ostream & o, const Reg& reg) {
        return o << "0x" << std::hex << (int) reg.data << std::dec << "(" << (int) reg.data << ")";
    }

protected:
    T data;
};

typedef Reg<uint8_t> Reg_8;
typedef Reg<uint16_t> Reg_16;

/* The P register is the flag/status register.
 * The flags are laid out (according to the wikipedia diagram) in this order:
 *   N V - B D I Z C
 * where
 *   N is the negative bit
 *   V is the overflow bit
 *   B is the breakpoint bit
 *   D is the binary coded decimal bit
 *   I is the interrupt bit
 *   Z is the zero bit
 *   C is the carry bit
 */
class PReg : public Reg_8 {
public:
    PReg() : Reg_8() {}
    inline void clear_carry() { this->clear_bit(0); }
    inline void set_carry() { this->set_bit(0); }
    inline bool has_carry() const { return this->is_set(0); }

    inline void clear_zero() { this->clear_bit(1); }
    inline void set_zero() { this->set_bit(1); }
    inline bool has_zero() const { return this->is_set(1); }

    inline void clear_interrupt() { this->clear_bit(2); }
    inline void set_interrupt() { this->set_bit(2); }
    inline bool has_interrupt() const { return this->is_set(2); }

    inline void clear_bcd() { this->clear_bit(3); }
    inline void set_bcd() { this->set_bit(3); }
    inline bool has_bcd() const { return this->is_set(3); }

    inline void clear_breakpoint() { this->clear_bit(4); }
    inline void set_breakpoint() { this->set_bit(4); }
    inline bool has_breakpoint() const { return this->is_set(4); }

    inline void clear_overflow() { this->clear_bit(6); }
    inline void set_overflow() { this->set_bit(6); }
    inline bool has_overflow() const { return this->is_set(6); }

    inline void clear_negative() { this->clear_bit(7); }
    inline void set_negative() { this->set_bit(7); }
    inline bool has_negative() const { return this->is_set(7); }

    inline friend std::ostream& operator<<(std::ostream & o, const PReg& reg) {
        for (int i = 7; i >= 0; --i) {
            if (i == 5) {
                o << '-';
                continue;
            }

            if (reg.is_set(i)) {
                o << '1';
            } else {
                o << '0';
            }
        }
        return o;
    }
private:
    inline void clear_bit(int i) { this->data &= ~(1 << i); }
    inline void set_bit(int i) { this->data |= (1 << i); }
    inline bool is_set(int i) const { return this->data & (1 << i); }
};

#endif
