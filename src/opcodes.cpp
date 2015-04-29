#include "opcodes.h"

const char* addr_mode_to_string(AddressMode mode) {
    switch (mode) {
        case ACC: return "ACC";
        case IMM: return "IMM";
        case ZP: return "ZP";
        case ZPX: return "ZPX";
        case ZPY: return "ZPY";
        case ABS: return "ABS";
        case ABSX: return "ABSX";
        case ABSY: return "ABSY";
        case IND: return "IND";
        case INDX: return "INDX";
        case INDY: return "INDY";
        case REL: return "REL";
        case IMP: return "IMP";
        default: return "UNKNOWN";
    }
}
