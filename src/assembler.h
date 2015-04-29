#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cctype>
#include <stdint.h>
//#include <cassert>
#include "opcodes.h"

#define DEF_LEX_FUNCTION(name, condition)       \
    bool name(std::string& s = ignore_str) { \
        s.clear();                              \
        char c;                                 \
        while ((c = this->src_file.peek()) != EOF) {  \
            if ((condition)) {                  \
                char c = this->src_file.get();  \
                s.push_back(c);                 \
            } else {                            \
                break;                          \
            }                                   \
        }                                       \
        return s.size() > 0;                    \
    }

std::string int_to_string(int x);
void _assert(bool expr, const char* expr_str, int line_number, const char* filename);
#define mos_assert(expr) _assert((expr), ""#expr, __LINE__, __FILE__)

class AssemblerError : public std::runtime_error {
public:
    AssemblerError(const std::string& msg) : std::runtime_error(msg) { }
};

class Assembler {
    static std::string ignore_str;
public:
    /* Throws std::invalid_argument on failing to open the file.
     * Throw AssemblerError on syntax or other errors during assembly.
     */
    Assembler(std::istream& stream) : src_file(stream) {
        this->assemble();
        this->resolve_labels();

        ignore_str.reserve(1024);

        // for convenience
        std::sort(this->relative_addresses.begin(), this->relative_addresses.end());
    }
private:

    //DEF_LEX_FUNCTION(read_alpha, isalpha(c))
    DEF_LEX_FUNCTION(read_dec_digit, isdigit(c))
    DEF_LEX_FUNCTION(read_alphanumeric, isalnum(c))
    DEF_LEX_FUNCTION(read_xdigit, isxdigit(c))
    DEF_LEX_FUNCTION(read_whitespace, isspace(c) && c != '\n' && c != '\r')
    DEF_LEX_FUNCTION(read_newline, c == '\n' || c == '\r')

    void add_label(const std::string& _label) {
        const char* label = _label.c_str();
        if (this->labels.find(label) == this->labels.end()) {
            this->labels[label] = (uint16_t) this->code.size();
//            std::cout << "Added label: " << label << "(" << this->labels[label] << ")" << std::endl;
            mos_assert(this->labels.find(label) != this->labels.end());
        } else {
            throw AssemblerError(
                std::string("Label '").append(label).append("' is defined twice"));
        }
    }

    /* Starting from the instruction at index i find the next instruction
     * matching the mnemonic. Returns -1 on failing to find an instruction.
     */
    int find_instruction(const std::string& mnemonic, int addr_mode = -1, int i = 0) {
        for (; i < OPS_SIZE; ++i) {
            if (OPS[i].has_name(mnemonic)
                    && (addr_mode < 0 || OPS[i].address_mode == addr_mode)) {
                return i;
            }
        }
        return -1;
    }

    void error_if_bad_opcode(int opcode, const std::string& instruction, int addr_mode = -1) {
        if (opcode < 0) {
            std::string msg("Failed to find op_code for instruction: ");
            msg.append(instruction);
            if (addr_mode >= 0) {
                msg.append(" and address mode ")
                   .append(addr_mode_to_string((AddressMode) addr_mode));
            }
            throw AssemblerError(msg);
        }
    }

    void assemble() {
        while (this->assemble_next_line());
    }

    /* read either a hex value $[0-9a-fA-F]+ or a decimal value [0-9]+ */
    bool read_number(std::string& s) {
        std::string tmp;
        s.clear();
        if (this->src_file.peek() == '$') {
            this->src_file.get();
            if (!this->read_xdigit(tmp)) {
                throw AssemblerError("Expected number after '$'");
            }
            s.append("0x").append(tmp);
        } else {
            this->read_dec_digit(s);
        }
        return s.size() > 0;
    }

    /* If s starts with "0x", parse a hex number. Otherwise, parse a decimal
     * number. */
    uint32_t parse_number(const std::string& s) {
        std::stringstream ss;
        uint32_t x;
        if (s.length() > 2 && s.substr(0, 2) == "0x") {
            ss << s.substr(2);
            ss >> std::hex >> x;
        } else {
            ss << s;
            ss >> x;
        }
        return x;
    }

    AddressMode mode_for_instr_w_label(const std::string& instruction) {
        const std::string& ilow = OpInfo::to_lower(instruction);
        if (ilow == "bcc"
            || ilow == "bcs"
            || ilow == "beq"
            || ilow == "bne"
            || ilow == "bmi"
            || ilow == "bpl"
            || ilow == "bvs"
            || ilow == "bvc") {
            return REL;
        }
        return ABS;
    }

    uint8_t compute_8bit_offset(int32_t base, int32_t dest) {
        int32_t diff = dest - base;
        if (diff < -128 || 127 < diff) {
            throw AssemblerError(std::string("Relative jump to ")
                                 + int_to_string(dest)
                                 + " is too far (> 8-bits).");
        }

        return diff & 0xff;
    }

    bool assemble_next_line() {
        int c;

        // read some text which is either an instruction or a label name
        while (this->read_whitespace() || this->read_newline());
        std::string text;
        if (!this->read_alphanumeric(text)) {
            return false;
        }
        this->read_whitespace();


        // a colon means we have a label definition
        if ((c = this->src_file.peek()) && c == ':') {
            this->add_label(text);
            this->src_file.get();
            this->read_whitespace();
            if (!this->read_newline()) {
                throw AssemblerError(std::string("Label '") + text
                                     + "' should appear on its own line");
            }
            return true;
        }

        // otherwise we have an instruction and need to parse its argument
        const std::string instruction = text;
        c = this->src_file.peek();

        // check for an immediate value
        if (c == '#') {
            this->src_file.get();
            // read either hex or a decimal value
            if (this->read_number(text)) {
                uint32_t value = this->parse_number(text);
                if (value > 0xff) {
                    throw AssemblerError(std::string("Immediate value ") + text
                                         + " is too large ("
                                         + int_to_string(value)
                                         + "). Immediate are only 8-bits");
                } else {
                    uint8_t argument = value & 0xff;
                    this->push_immediate_opcodes(instruction, argument);
                }
            } else {
                throw AssemblerError("Expected number after '#'");
            }

        // instruction with no arguments
        } else if (c == '\n' || c == '\r' || c < 0) {
            this->push_implied_opcodes(instruction);
        // accumulator register
        } else if (c == 'A') {  // should we allow lower case here?
            this->src_file.get();
        // a hex or decimal value
        } else if (c == '$' || isdigit(c)) {
            if (this->read_number(text)) {
                uint32_t value = this->parse_number(text);
                // addressing mode is determined by the value of the argument
                // values larger than 0xff are outside the zero page
                if (value > 0xffff) {
                    throw AssemblerError(std::string("Value ").append(text)
                                         .append(" is too large."));
                } else if (value > 0xff) {
                    this->push_absolute_opcodes(instruction, value & 0xffff);
                } else {
                    this->push_zero_page_opcodes(instruction, value & 0xff);
                }
            } else {
                throw AssemblerError("Expected a number");
            }
        // a label
        } else if (isalpha(c)) {
            this->read_alphanumeric(text);
            const std::string label = text;
//            std::cout << "saw label '" << label << "' as argument" << std::endl;
            /* PROBLEM: Our labels are actually indices into the code vector.
             * They are actually relative code addresses, but if we convert them
             * immediately to opcodes, the labels' address will be incorrect unless
             * if we load the code any where other than address zero.
             *
             * Solutions:
             *      - Provide the location to load the code in advance.
             *      - Create some an intermediate representation that distinguishes
             *      between relative and absolute addresses.
             *
             */

            AddressMode addr_mode = this->mode_for_instr_w_label(instruction);

            // lookup the address the label represents
            if (this->labels.find(label) != this->labels.end()) {
//                std::cout << "found label '" << label << "'" << std::endl;
                uint16_t address = this->labels[label];
                if (addr_mode == ABS) {
                    this->push_absolute_opcodes(instruction, address, true);
                } else if (addr_mode == REL) {
                    this->push_relative_opcodes(instruction, address);
                }
            } else {
                // the label hasn't been seen, but we can fill its address in later
                this->push_unresolved_label_opcodes(instruction, label, addr_mode);
            }
        } else {
            throw AssemblerError("BUG: While assembling a line");
        }
        return true;
    }

    void push_implied_opcodes(const std::string instruction) {
        int op_code = this->find_instruction(instruction, IMP);
        this->error_if_bad_opcode(op_code, instruction, IMP);
        // write two bytes for BRK
        if (OPS[op_code].has_name("brk")) {
            this->code.push_back(op_code);
        }
        this->code.push_back(op_code);
    }

    void push_immediate_opcodes(const std::string& instruction, uint8_t argument) {
        int op_code = this->find_instruction(instruction, IMM);
        this->error_if_bad_opcode(op_code, instruction, IMM);
        mos_assert(OPS[op_code].n_bytes == 2);
        this->code.push_back(op_code);
        this->code.push_back(argument);
    }

    void push_absolute_opcodes(const std::string& instruction, uint16_t argument, bool label_address = false) {
        int op_code = this->find_instruction(instruction, ABS);
        this->error_if_bad_opcode(op_code, instruction, ABS);
        mos_assert(OPS[op_code].n_bytes == 3);
        this->code.push_back(op_code);
        // TODO: What order do we store 16 byte values?
        this->code.push_back((uint8_t) (argument & 0xff));
        this->code.push_back((uint8_t) ((argument >> 8) & 0xff));

        if (label_address) {
            this->relative_addresses.push_back(this->code.size() - 2);
        }
    }

    void push_relative_opcodes(const std::string& instruction, uint16_t argument) {
        int op_code = this->find_instruction(instruction, REL);
        this->error_if_bad_opcode(op_code, instruction, REL);
        mos_assert(OPS[op_code].n_bytes == 2);
        this->code.push_back(op_code);
        // For branches we will adjust the PC according to an offset argument
        // Do we increment the PC first, and then do the increment?
        // I think so. The PC should be increment immediately after each byte fetched.
        this->code.push_back(this->compute_8bit_offset(this->code.size() + 1, argument));
    }

    void push_zero_page_opcodes(const std::string& instruction, uint8_t argument) {
        int op_code = this->find_instruction(instruction, ZP);
        this->error_if_bad_opcode(op_code, instruction, ZP);
//        std::cout << instruction << std::endl;
        mos_assert(OPS[op_code].n_bytes == 2);
        this->code.push_back(op_code);
        this->code.push_back(argument);
    }

    void push_unresolved_label_opcodes(const std::string& instruction,
                                       const std::string& label,
                                       AddressMode addr_mode) {
        mos_assert(addr_mode == ABS || addr_mode == REL);
        // store information to fill in the address later
        this->unresolved_labels[this->code.size() + 1] = std::make_pair(label.c_str(), addr_mode);

        int op_code = this ->find_instruction(instruction, addr_mode);
        this->error_if_bad_opcode(op_code, instruction, addr_mode);
        this->code.push_back(op_code);
        // the address of the label is unknown, so pad with zeroes
        int i;
        for (i = 1; i < OPS[op_code].n_bytes; ++i) {
            this->code.push_back(0x00);
        }
        mos_assert(OPS[op_code].n_bytes == i);
    }

    void resolve_labels() {
        std::map<size_t, std::pair<std::string, AddressMode> >::const_iterator it;
        for (it = this->unresolved_labels.begin(); it != this->unresolved_labels.end(); ++it) {
            // code[i] needs the address of the label
            size_t i = it->first;
            const std::string& label = it->second.first;
            AddressMode addr_mode = it->second.second;

            if (this->labels.find(label) == this->labels.end()) {
                throw AssemblerError(std::string("Undefined label ").append(label));
            }

            uint16_t address = this->labels[label];
            if (addr_mode == ABS) {
                mos_assert(code[i] = 0x00 && code[i + 1] == 0x00);
                this->code[i] = address & 0xff;
                this->code[i + 1] = (address >> 8) & 0xff;
                // we will need to adjust this address again at either link or load time
                this->relative_addresses.push_back(i);
            } else if (addr_mode == REL) {
                mos_assert(code[i] == 0x00);
                this->code[i] = this->compute_8bit_offset(i + 1, address);
            } else {
                throw AssemblerError(std::string("BUG: Unsupported address "
                                     "mode during label resolution: ")
                                     + addr_mode_to_string(addr_mode));
            }
        }
    }


public:

    /* During assembly, the PC is assumed to start at zero. Any labels used
     * are converted to addresses under this assumption. This will add base_addr
     * to each labels address, to allow the PC to start at base_addr.
     *
     * result is cleared and the relocated code is copied into result
     * throws std::invalid_argument if base_addr pushes code passed 0xffff
     */
    void relocate_code(uint16_t base_addr, std::vector<uint8_t>& result) const {
        if (base_addr + this->code.size() > 0xffff) {
            throw std::invalid_argument("Relocation pushes code out of bounds");
        }

        result.clear();
        result.resize(this->code.size(), 0);  // ensure there is enough room
        std::copy(this->code.begin(), this->code.end(), result.begin());

        for (size_t i = 0; i < this->relative_addresses.size(); ++i) {
            size_t pc = this->relative_addresses[i];

            uint16_t lo = result[pc];
            uint16_t hi = result[pc + 1];
            uint32_t old_addr = (((hi << 8) & 0xff00) | (lo & 0xff)) & 0xffff;
            uint32_t new_addr = old_addr + base_addr;
            if (new_addr > 0xffff) {
                throw std::invalid_argument(std::string("Relocation push address ")
                                            + int_to_string(old_addr) + "out of bounds");
            } else {
                result[pc] = new_addr & 0xff;
                result[pc + 1] = (new_addr >> 8) & 0xff;
            }
        }
    }

    static std::string get_code_hex(const std::vector<uint8_t>& code) {
        std::stringstream ss;
        for (size_t i = 0; i < code.size(); ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int) code[i];
        }
        return ss.str();
    }

    inline std::string get_code_hex() const {
        return Assembler::get_code_hex(this->code);
    }

public:
    std::istream& src_file;
    std::vector<uint8_t> code;
    std::map<std::string, uint16_t> labels;

    /* If i is in relative_addresses, then code[i] and code[i+1]
     * form a relative address. When loading the code into memory
     * at location X, these addresses must have X added to them.
     *
     * This has nothing to do with the relative addresing mode
     * usd by certain instructions.
     */
    std::vector<size_t> relative_addresses;

    /* If a label is used before we know its address, it is stored
     * here. A second pass over the code vector fills in the correct
     * label addresses.
     *
     * This fundamentally stores (index, label, mode) tuples.
     * This says that code[index] needs to be filled in with the
     * address of the label. mode should be either REL or ABS.
     * If mode is REL, than an 8-bit offset to the label's address
     * is computed (error if too far away). If mode is ABS, then the
     * label's address is copied to code[index] and code[index + 1].
     *
     * This gets slightly problematic supporting both:
     *      JMP labelname
     *      BNE labelname
     * The JMP is absolute. It sets the PC to that address. The branch
     * is relative to the address of the branch instruction. It takes
     * a signed 8-bit offset and jumps forward or backward that amount.
     */
    std::map<size_t, std::pair<std::string, AddressMode> > unresolved_labels;
};

#endif // ASSEMBLER_H
