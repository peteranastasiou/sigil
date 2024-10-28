
#include <stdint.h>

/**
 * Pack and unpack 16 bit values to/from bytes
 */
namespace encode {

uint8_t packUint16a(uint16_t value);
uint8_t packUint16b(uint16_t value);

uint8_t packInt16a(int16_t value);
uint8_t packInt16b(int16_t value);

uint16_t unpackUint16(uint8_t a, uint8_t b);

int16_t unpackInt16(uint8_t a, uint8_t b);

}
