
#include "encode.hpp"

namespace encode {

uint8_t packUint16a(uint16_t value) {
    return (uint8_t)((value >> 8) & 0xff);
}

uint8_t packUint16b(uint16_t value) {
    return (uint8_t)(value & 0xff);
}

uint8_t packInt16a(int16_t value) {
    return (uint8_t)((value >> 8) & 0xff);
}
uint8_t packInt16b(int16_t value) {
    return (uint8_t)(value & 0xff);
}

uint16_t unpackUint16(uint8_t a, uint8_t b) {
    return (uint16_t)((a << 8) | b);
}

int16_t unpackInt16(uint8_t a, uint8_t b) {
    return (int16_t)((a << 8) | b);
}

}
