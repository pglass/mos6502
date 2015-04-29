#include <iostream>
#include <fstream>
#include "cpu.h"
#include "mem.h"
#include "assembler.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Need a file as input" << std::endl;
        return 1;
    }

    std::cout << "Using input file: " << argv[1] << std::endl;
    try {
        std::ifstream src(argv[1]);
        if (!src.is_open()) {
            throw std::invalid_argument(
                std::string("File not found: '") + argv[1] + "'");
        }

        // assemble the input file
        Assembler assembler(src);
        std::cout << "Code: " << assembler.get_code_hex() << std::endl;

        // relocate code to the correct address
        uint16_t addr = 0x600;
        std::vector<uint8_t> code;
        assembler.relocate_code(0x600, code);
        std::cout << "Code relocated to address 0x" << std::hex << addr << std::dec
                  << ": " << Assembler::get_code_hex(code) << std::endl;

        Cpu cpu(std::cout);
        cpu.load_code(code, addr);
        cpu.emu_loop();

    } catch (std::invalid_argument& error) {
        std::cerr << error.what() << std::endl;
        return -1;
    } catch (std::exception& error) {
        std::cerr << error.what() << std::endl;
        return -1;
    }

//    cout << cpu << endl;
//    cout << "MEM_SIZE = " << Mem::MEM_SIZE << endl;
//    cout << "Read address 0x3333: " << (int) cpu.mem.read_8(0x3333) << endl;
//    cout << "Write 121 to address 0x3333" << endl;
//    cpu.mem.write_8(0x3333, 121);
//    cout << "Read address 0x3333: " << (int) cpu.mem.read_8(0x3333) << endl;

//    std::ofstream memfile("mem.txt", std::ios::trunc);
//    memfile << cpu.mem;

    return 0;
}

