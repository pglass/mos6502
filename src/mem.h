#ifndef MEM_H
#define MEM_H

#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdint.h>

typedef uint16_t address_t;

class Mem {
public:
    static const size_t MEM_SIZE = 1 << 16;

    static std::string as_hex(int value) {
        std::stringstream ss;
        ss << "0x" << std::setw(6) << std::setfill('0') << std::hex << value;
        return ss.str();
    }

    Mem() {
        memset(this->data, 0, MEM_SIZE);
    }

    inline uint8_t read_8(address_t index) const {
        return data[index];
    }

    inline uint16_t read_16(address_t index) const {
        // little endian - least significant byte in smallest address
        return ((data[index + 1] << 8) & 0xff00) | (data[index] & 0xff);
    }

    inline void write_8(address_t index, uint8_t val) {
        data[index] = val;
    }

    inline void write_16(address_t index, uint16_t val) {
        // little endian - least significant byte in smallest address
        data[index] = (uint8_t) (val & 0xFF);
        data[index + 1] = (uint8_t) (val >> 8) & 0xFF;
    }

    friend std::ostream& operator<<(std::ostream& o, const Mem& mem) {
        for (size_t i = 0; i < MEM_SIZE; ++i) {
            int val = mem.read_8(i);

            // print the address and value in hex
            o << as_hex(i) << ": " << as_hex(val)
            // print the value in decimal
              << " " <<  std::setw(6) << std::dec << val;

            // print the value as a char
            uint8_t c = mem.read_8(i);
            if (c == 0) {
                o << std::setw(8) << "'\\0'";
            } else {
                o << std::setw(6) << "'" << c << "'";
            }
            o << std::endl;
        }
        return o;
    }

public:
    uint8_t data[MEM_SIZE];
};

#endif // MEM_H
