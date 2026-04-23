#ifndef FLAGS_HPP
#define FLAGS_HPP

#include <cstdint>

class Flags {
public:
    // Bitmasks according to Intel 8080 specification
    // S Z 0 AC 0 P 1 CY
    static constexpr uint8_t MASK_CY = 0x01;
    static constexpr uint8_t MASK_SET1 = 0x02; // Bit 1 is always 1
    static constexpr uint8_t MASK_P  = 0x04;
    static constexpr uint8_t MASK_RESERVED = 0x20; // Bit 5 must be 0
    static constexpr uint8_t MASK_AC = 0x10;
    static constexpr uint8_t MASK_Z  = 0x40;
    static constexpr uint8_t MASK_S  = 0x80;

    explicit Flags(uint8_t& flagsByte) : flags_byte(flagsByte) {}

    bool isZero() const { return (flags_byte & MASK_Z) != 0; }
    void setZero(bool val) { setBit(MASK_Z, val); }

    bool isSign() const { return (flags_byte & MASK_S) != 0; }
    void setSign(bool val) { setBit(MASK_S, val); }

    bool isCarry() const { return (flags_byte & MASK_CY) != 0; }
    void setCarry(bool val) { setBit(MASK_CY, val); }

    bool isAuxiliaryCarry() const { return (flags_byte & MASK_AC) != 0; }
    void setAuxiliaryCarry(bool val) { setBit(MASK_AC, val); }

    bool isParity() const { return (flags_byte & MASK_P) != 0; }
    void setParity(bool val) { setBit(MASK_P, val); }

    uint8_t getByte() const {
        return flags_byte | MASK_SET1;
    }

    void setByte(uint8_t value) {
        // Clear bit 1 (SET1) and bit 5 (RESERVED), then restore bit 1
        flags_byte = value & ~MASK_SET1;
        flags_byte |= MASK_SET1;
        flags_byte &= ~MASK_RESERVED;
    }

private:
    uint8_t& flags_byte;

    void setBit(uint8_t mask, bool condition) {
        if (condition) {
            flags_byte |= mask;
        } else {
            flags_byte &= ~mask;
        }
        // Ensure reserved bit 5 is always 0 as per spec
        flags_byte &= ~MASK_RESERVED;
    }
};

#endif // FLAGS_HPP