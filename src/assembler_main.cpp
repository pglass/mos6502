#include "assembler.h"

void print_usage(char* prog_name) {
    std::cout << "Usage: " << prog_name << " <filename>" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }

    std::cout << "Using input file: " << argv[1] << std::endl;
    try {
        std::ifstream src(argv[1]);
        if (!src.is_open()) {
            throw std::invalid_argument(
                std::string("File not found: '").append(argv[1]).append("'"));
        }

        Assembler assembler(src);

        std::cout << assembler.get_code_hex() << std::endl;

    } catch (AssemblerError& error) {
        std::cerr << "AssemblerError: " << error.what() << std::endl;
    } catch (std::invalid_argument& error) {
        std::cerr << "Invalid argument: " << error.what() << std::endl;
    } catch (std::exception& error) {
        std::cerr << "Exception: " << error.what() << std::endl;
    }

//    std::cout << "Loading source file: " << argv[1] << std::endl;
//    std::ifstream source_file(argv[1]);
//    if (!source_file.is_open()) {
//        std::cout << "file doesn't exist" << std::endl;
//        return 1;
//    }

//    std::string line;
//    for (int i = 1; std::getline(source_file, line); ++i) {
//        std::cout << i <<  ": " << line << std::endl;
//    }
}
